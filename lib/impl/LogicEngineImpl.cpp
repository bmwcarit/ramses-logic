//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicEngineImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"

#include "impl/LoggerImpl.h"
#include "internals/FileUtils.h"
#include "internals/TypeUtils.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/LogicNode.h"

#include "ramses-logic-build-config.h"
#include "generated/logicengine_gen.h"

#include "ramses-framework-api/RamsesFrameworkTypes.h"
#include "ramses-framework-api/RamsesVersion.h"
#include "ramses-client-api/SceneObject.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"
#include "ramses-utils.h"

#include <string>
#include <fstream>
#include <streambuf>

#include "fmt/format.h"

namespace rlogic::internal
{
    LuaScript* LogicEngineImpl::createLuaScriptFromFile(std::string_view filename, std::string_view scriptName)
    {
        m_errors.clear();

        std::ifstream iStream;
        // ifstream has no support for string_view
        iStream.open(std::string(filename), std::ios_base::in);
        if (iStream.fail())
        {
            m_errors.add("Failed opening file " + std::string(filename) + "!");
            return nullptr;
        }

        std::string source(std::istreambuf_iterator<char>(iStream), std::istreambuf_iterator<char>{});
        return m_apiObjects.createLuaScript(m_luaState, source, filename, scriptName, m_errors);
    }

    LuaScript* LogicEngineImpl::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        m_errors.clear();
        return m_apiObjects.createLuaScript(m_luaState, source, "", scriptName, m_errors);
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesNodeBinding(name);
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesAppearanceBinding(name);
    }

    RamsesCameraBinding* LogicEngineImpl::createRamsesCameraBinding(std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesCameraBinding(name);
    }

    bool LogicEngineImpl::destroy(LogicNode& logicNode)
    {
        m_errors.clear();
        return m_apiObjects.destroy(logicNode, m_errors);
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_apiObjects.getLogicNodeDependencies().isLinked(logicNode.m_impl);
    }

    void LogicEngineImpl::updateLinksRecursive(Property& inputProperty)
    {
        // TODO Violin consider providing property iterator - seems it's needed quite often, would make access more convenient also for users
        const auto inputCount = inputProperty.getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            Property& child = *inputProperty.getChild(i);

            if (TypeUtils::CanHaveChildren(child.getType()))
            {
                updateLinksRecursive(child);
            }
            else
            {
                const PropertyImpl* output = m_apiObjects.getLogicNodeDependencies().getLinkedOutput(*child.m_impl);
                if (nullptr != output)
                {
                    child.m_impl->setInternal(*output);
                }
            }
        }
    }

    bool LogicEngineImpl::update(bool disableDirtyTracking)
    {
        m_errors.clear();
        LOG_DEBUG("Begin update");

        const std::optional<NodeVector> sortedNodes = m_apiObjects.getLogicNodeDependencies().getTopologicallySortedNodes();
        if (!sortedNodes)
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling update()!");
            return false;
        }

        for (LogicNodeImpl* logicNode : *sortedNodes)
        {
            if (!updateLogicNodeInternal(*logicNode, disableDirtyTracking))
            {
                return false;
            }
        }

        return true;
    }

    bool LogicEngineImpl::updateLogicNodeInternal(LogicNodeImpl& node, bool disableDirtyTracking)
    {
        updateLinksRecursive(*node.getInputs());
        if (disableDirtyTracking || node.isDirty())
        {
            LOG_DEBUG("Updating LogicNode '{}'", node.getName());
            if (!node.update())
            {
                const auto& errors = node.getErrors();
                for (const auto& error : errors)
                {
                    m_errors.add(error, *m_apiObjects.getApiObject(node));
                }
                return false;
            }
        }
        else
        {
            LOG_DEBUG("Skip update of LogicNode '{}' because no input has changed since the last update", node.getName());
        }
        node.setDirty(false);
        return true;
    }

    const std::vector<ErrorData>& LogicEngineImpl::getErrors() const
    {
        return m_errors.getErrors();
    }

    bool LogicEngineImpl::CheckLogicVersionFromFile(const rlogic_serialization::Version& version)
    {
        // TODO Violin/Tobias/Sven should discuss to what do we couple the serialization
        // compatibility check and when do we mandate version bump
        // For now, to be safe, any non-patch version change results in incompatibility
        return
            version.v_major() == g_PROJECT_VERSION_MAJOR &&
            version.v_minor() == g_PROJECT_VERSION_MINOR;
    }

    bool LogicEngineImpl::CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion)
    {
        // Only major version changes result in file incompatibilities
        return static_cast<int>(ramsesVersion.v_major()) == ramses::GetRamsesVersion().major;
    }

    bool LogicEngineImpl::loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* scene, bool enableMemoryVerification)
    {
        return loadFromByteData(rawBuffer, bufferSize, scene, enableMemoryVerification, fmt::format("data buffer '{}' (size: {})", rawBuffer, bufferSize));
    }

    bool LogicEngineImpl::loadFromFile(std::string_view filename, ramses::Scene* scene, bool enableMemoryVerification)
    {
        std::optional<std::vector<char>> maybeBytesFromFile = FileUtils::LoadBinary(std::string(filename));
        if (!maybeBytesFromFile)
        {
            m_errors.add(fmt::format("Failed to load file '{}'", filename));
            return false;
        }

        const size_t fileSize = (*maybeBytesFromFile).size();
        return loadFromByteData((*maybeBytesFromFile).data(), fileSize, scene, enableMemoryVerification, fmt::format("file '{}' (size: {})", filename, fileSize));
    }


    // TODO Violin consider handling errors gracefully, e.g. don't change state when error occurs
    // Idea: collect data, and only move() in the end when everything was loaded correctly
    bool LogicEngineImpl::loadFromByteData(const void* byteData, size_t byteSize, ramses::Scene* scene, bool enableMemoryVerification, const std::string& dataSourceDescription)
    {
        m_errors.clear();

        if (enableMemoryVerification)
        {
            flatbuffers::Verifier bufferVerifier(static_cast<const uint8_t*>(byteData), byteSize);
            const bool bufferOK = rlogic_serialization::VerifyLogicEngineBuffer(bufferVerifier);

            if (!bufferOK)
            {
                m_errors.add(fmt::format("{} contains corrupted data!", dataSourceDescription));
                return false;
            }
        }

        // Fill containers with data and check for errors before replacing existing data
        // TODO Violin also use fresh Lua environment, so that we don't pollute current one when loading failed
        ScriptsContainer scripts;
        NodeBindingsContainer ramsesNodeBindings;
        AppearanceBindingsContainer ramsesAppearanceBindings;
        CameraBindingsContainer ramsesCameraBindings;
        LogicNodeDependencies logicNodeDependencies;

        const auto logicEngine = rlogic_serialization::GetLogicEngine(byteData);

        if (nullptr == logicEngine || nullptr == logicEngine->ramsesVersion() || nullptr == logicEngine->rlogicVersion())
        {
            m_errors.add(fmt::format("{} doesn't contain logic engine data with readable version specifiers", dataSourceDescription));
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected Ramses version {}.x.x but found {}",
                dataSourceDescription, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()));
            return false;
        }

        const auto& rlogicVersion = *logicEngine->rlogicVersion();
        if (!CheckLogicVersionFromFile(rlogicVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected version {}.{}.x but found {}",
                dataSourceDescription, g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR,
                rlogicVersion.v_string()->string_view()));
            return false;
        }

        assert(nullptr != logicEngine->luascripts());
        assert(nullptr != logicEngine->ramsesnodebindings());
        assert(nullptr != logicEngine->ramsesappearancebindings());
        assert(nullptr != logicEngine->ramsescamerabindings());

        const auto luascripts = logicEngine->luascripts();
        assert(nullptr != luascripts);

        if (luascripts->size() != 0)
        {
            scripts.reserve(luascripts->size());
        }

        for (auto script : *luascripts)
        {
            // script can't be null. Already handled by flatbuffers
            assert(nullptr != script);
            auto newScript = LuaScriptImpl::Create(m_luaState, script, m_errors);
            if (nullptr != newScript)
            {
                scripts.emplace_back(std::make_unique<LuaScript>(std::move(newScript)));
                logicNodeDependencies.addNode(scripts.back()->m_impl);
            }
        }

        const auto ramsesnodebindings = logicEngine->ramsesnodebindings();
        assert(nullptr != ramsesnodebindings);

        if (ramsesnodebindings->size() != 0)
        {
            ramsesNodeBindings.reserve(ramsesnodebindings->size());
        }

        for (auto rNodeBinding : *ramsesnodebindings)
        {
            assert(nullptr != rNodeBinding);
            ramses::Node*           ramsesNode = nullptr;
            ramses::sceneObjectId_t objectId(rNodeBinding->ramsesNode());

            if (objectId.isValid())
            {
                auto optional_ramsesNode = findRamsesNodeInScene(rNodeBinding->logicnode()->name()->string_view(), scene, objectId);
                if (!optional_ramsesNode)
                {
                    return false;
                }
                ramsesNode = *optional_ramsesNode;
            }
            auto newBindingImpl = RamsesNodeBindingImpl::Create(*rNodeBinding, ramsesNode);
            assert(nullptr != newBindingImpl);
            ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::move(newBindingImpl)));
            logicNodeDependencies.addNode(ramsesNodeBindings.back()->m_impl);
        }

        const auto ramsesappearancebindings = logicEngine->ramsesappearancebindings();

        if (ramsesappearancebindings->size() != 0)
        {
            ramsesAppearanceBindings.reserve(ramsesappearancebindings->size());
        }

        for (auto rAppearanceBinding : *ramsesappearancebindings)
        {
            assert(nullptr != rAppearanceBinding);

            ramses::Appearance*     ramsesAppearance = nullptr;
            ramses::sceneObjectId_t objectId(rAppearanceBinding->ramsesAppearance());

            if (objectId.isValid())
            {
                auto optional_ramsesAppearance = findRamsesAppearanceInScene(rAppearanceBinding->logicnode()->name()->string_view(), scene, objectId);
                if(!optional_ramsesAppearance)
                {
                    return false;
                }
                ramsesAppearance = *optional_ramsesAppearance;
            }

            auto newBinding = RamsesAppearanceBindingImpl::Create(*rAppearanceBinding, ramsesAppearance, m_errors);
            if (nullptr == newBinding)
            {
                return false;
            }

            ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::move(newBinding)));
            logicNodeDependencies.addNode(ramsesAppearanceBindings.back()->m_impl);
        }

        const auto ramsescamerabindings = logicEngine->ramsescamerabindings();

        if (ramsescamerabindings->size() != 0)
        {
            ramsesCameraBindings.reserve(ramsescamerabindings->size());
        }

        for (auto rCameraBinding : *ramsescamerabindings)
        {
            assert(nullptr != rCameraBinding);

            ramses::Camera*     ramsesCamera = nullptr;
            ramses::sceneObjectId_t objectId(rCameraBinding->ramsesCamera());

            if (objectId.isValid())
            {
                auto optional_ramsesCamera = findRamsesCameraInScene(rCameraBinding->logicnode()->name()->string_view(), scene, objectId);
                if (!optional_ramsesCamera)
                {
                    return false;
                }
                ramsesCamera = *optional_ramsesCamera;
            }

            auto newBinding = RamsesCameraBindingImpl::Create(*rCameraBinding, ramsesCamera);
            if (nullptr == newBinding)
            {
                return false;
            }

            ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::move(newBinding)));
            logicNodeDependencies.addNode(ramsesCameraBindings.back()->m_impl);
        }

        assert(logicEngine->links());
        const auto& links = *logicEngine->links();

        // TODO Violin open discussion if we want to handle errors differently here
        for (const auto rLink : links)
        {
            assert(nullptr != rLink);

            const LogicNode* srcLogicNode = nullptr;
            switch (rLink->sourceLogicNodeType())
            {
            case rlogic_serialization::ELogicNodeType::Script:
                assert(rLink->sourceLogicNodeId() < scripts.size());
                srcLogicNode = scripts[rLink->sourceLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesNodeBinding:
            case rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding:
            case rlogic_serialization::ELogicNodeType::RamsesCameraBinding:
                assert(false && "Bindings can't be the source node of a link!");
            }
            assert(srcLogicNode != nullptr);

            const LogicNode* tgtLogicNode = nullptr;
            switch (rLink->targetLogicNodeType())
            {
            case rlogic_serialization::ELogicNodeType::Script:
                assert(rLink->targetLogicNodeId() < scripts.size());
                tgtLogicNode = scripts[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesNodeBinding:
                assert(rLink->targetLogicNodeId() < ramsesNodeBindings.size());
                tgtLogicNode = ramsesNodeBindings[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding:
                assert(rLink->targetLogicNodeId() < ramsesAppearanceBindings.size());
                tgtLogicNode = ramsesAppearanceBindings[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesCameraBinding:
                assert(rLink->targetLogicNodeId() < ramsesCameraBindings.size());
                tgtLogicNode = ramsesCameraBindings[rLink->targetLogicNodeId()].get();
                break;
            }
            assert(tgtLogicNode != nullptr);
            assert(tgtLogicNode != srcLogicNode);

            const auto* indicesSrc = rLink->sourcePropertyNestedIndex();
            assert(indicesSrc != nullptr);
            const Property* sourceProperty = srcLogicNode->getOutputs();
            assert(sourceProperty);
            for (auto idx : *indicesSrc)
            {
                sourceProperty = sourceProperty->getChild(idx);
                assert(sourceProperty);
            }

            const auto* indicesTgt = rLink->targetPropertyNestedIndex();
            assert(indicesTgt != nullptr);
            const Property* targetProperty = tgtLogicNode->getInputs();
            assert(targetProperty != nullptr);
            for (auto idx : *indicesTgt)
            {
                targetProperty = targetProperty->getChild(idx);
                assert(targetProperty != nullptr);
            }

            const bool success = logicNodeDependencies.link(*sourceProperty->m_impl, *targetProperty->m_impl, m_errors);
            if (!success)
            {
                m_errors.add(
                    fmt::format("Fatal error during loading from {}! Could not link property '{}' (from LogicNode {}) to property '{}' (from LogicNode {})!",
                        dataSourceDescription,
                        sourceProperty->getName(),
                        srcLogicNode->getName(),
                        targetProperty->getName(),
                        tgtLogicNode->getName()
                    ));
                return false;
            }
        }

        m_apiObjects = ApiObjects(std::move(scripts), std::move(ramsesNodeBindings), std::move(ramsesAppearanceBindings), std::move(ramsesCameraBindings), std::move(logicNodeDependencies));

        return true;
    }

    ramses::SceneObject* LogicEngineImpl::findRamsesSceneObjectInScene(std::string_view logicNodeName, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        if (nullptr == scene)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized ramses logic object '{}' points to a Ramses object (id: {}) , but no Ramses scene was provided to resolve the Ramses object!",
                            logicNodeName,
                            objectId.getValue()
                    ));
            return nullptr;
        }

        ramses::SceneObject* sceneObject = scene->findObjectById(objectId);

        if (nullptr == sceneObject)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized ramses logic object {} points to a Ramses object (id: {}) which couldn't be found in the provided scene!",
                            logicNodeName,
                            objectId.getValue()));
            return nullptr;
        }

        return sceneObject;
    }

    std::optional<ramses::Node*> LogicEngineImpl::findRamsesNodeInScene(std::string_view logicNodeName, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        ramses::SceneObject*    sceneObject = findRamsesSceneObjectInScene(logicNodeName, scene, objectId);
        ramses::Node* ramsesNode = nullptr;

        if (nullptr == sceneObject)
        {
            // TODO Violin unit test this and remove code duplication
            return std::nullopt;
        }

        ramsesNode = ramses::RamsesUtils::TryConvert<ramses::Node>(*sceneObject);

        if (nullptr == ramsesNode)
        {
            // TODO Violin unit test this and remove code duplication
            m_errors.add("Fatal error during loading from file! Node binding points to a Ramses scene object which is not of type 'Node'!");
            return std::nullopt;
        }

        return std::make_optional(ramsesNode);
    }

    std::optional<ramses::Appearance*> LogicEngineImpl::findRamsesAppearanceInScene(std::string_view logicNodeName, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNodeName, scene, objectId);
        ramses::Appearance* ramsesAppearance  = nullptr;

        if (nullptr == sceneObject)
        {
            // TODO Violin unit test this and remove code duplication
            return std::nullopt;
        }

        ramsesAppearance = ramses::RamsesUtils::TryConvert<ramses::Appearance>(*sceneObject);

        if (nullptr == ramsesAppearance)
        {
            // TODO Violin unit test this and remove code duplication
            m_errors.add("Fatal error during loading from file! Appearance binding points to a Ramses scene object which is not of type 'Appearance'!");
            return std::nullopt;
        }

        return std::make_optional(ramsesAppearance);
    }

    std::optional<ramses::Camera*> LogicEngineImpl::findRamsesCameraInScene(std::string_view logicNodeName, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNodeName, scene, objectId);
        ramses::Camera* ramsesCamera = nullptr;

        if (nullptr == sceneObject)
        {
            // TODO Violin unit test this and remove code duplication
            return std::nullopt;
        }

        ramsesCamera = ramses::RamsesUtils::TryConvert<ramses::Camera>(*sceneObject);

        if (nullptr == ramsesCamera)
        {
            // TODO Violin unit test this and remove code duplication
            m_errors.add("Fatal error during loading from file! Camera binding points to a Ramses scene object which is not of type 'Camera'!");
            return std::nullopt;
        }

        return std::make_optional(ramsesCamera);
    }

    // TODO Violin this needs more testing (both internal and user-side). Concretely, ensure that:
    // - if anything is dirty, this will always warn
    // - the logic engine is never dirty immediately after update()
    // - some specific cases that need special attention:
    //      - what is the dirty state after unlink?
    //      - what is the dirty state when setting a binding property (with same value?)
    bool LogicEngineImpl::isDirty() const
    {
        // TODO Violin the ugliness of this code shows the problems with the current dirty handling implementation
        // Refactor the dirty handling, and this code will disappear

        // TODO Violin improve internal management of logic nodes so that we don't have to loop over three
        // different containers below which all call a method on LogicNode

        // Scripts dirty?
        for (const auto& script : m_apiObjects.getScripts())
        {
            if (script->m_impl.get().isDirty())
            {
                return true;
            }
        }

        if (bindingsDirty())
        {
            return true;
        }

        return false;
    }

    bool LogicEngineImpl::bindingsDirty() const
    {
        for (const auto& nodeBinding : m_apiObjects.getNodeBindings())
        {
            if (nodeBinding->m_impl.get().isDirty())
            {
                return true;
            }
        }

        for (const auto& appBinding : m_apiObjects.getAppearanceBindings())
        {
            if (appBinding->m_impl.get().isDirty())
            {
                return true;
            }
        }

        for (const auto& camBinding : m_apiObjects.getCameraBindings())
        {
            if (camBinding->m_impl.get().isDirty())
            {
                return true;
            }
        }

        return false;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename)
    {
        m_errors.clear();

        if (!m_apiObjects.checkBindingsReferToSameRamsesScene(m_errors))
        {
            m_errors.add("Can't save a logic engine to file while it has references to more than one Ramses scene!");
            return false;
        }

        if (bindingsDirty())
        {
            LOG_WARN("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!");
        }

        const ScriptsContainer& scripts = m_apiObjects.getScripts();

        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(scripts.size());

        std::transform(scripts.begin(), scripts.end(), std::back_inserter(luascripts), [&builder](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) {
            return it->m_script->serialize(builder);
        });

        const NodeBindingsContainer& nodeBindings = m_apiObjects.getNodeBindings();
        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(nodeBindings.size());

        std::transform(nodeBindings.begin(),
                       nodeBindings.end(),
                       std::back_inserter(ramsesnodebindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) { return it->m_nodeBinding->serialize(builder); });

        const AppearanceBindingsContainer& appearanceBindings = m_apiObjects.getAppearanceBindings();
        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(appearanceBindings.size());

        std::transform(appearanceBindings.begin(),
                       appearanceBindings.end(),
                       std::back_inserter(ramsesappearancebindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesAppearanceBinding>>::value_type& it) { return it->m_appearanceBinding->serialize(builder);
        });

        const CameraBindingsContainer& cameraBindings = m_apiObjects.getCameraBindings();
        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> ramsescamerabindings;
        ramsescamerabindings.reserve(cameraBindings.size());

        std::transform(cameraBindings.begin(),
                       cameraBindings.end(),
                       std::back_inserter(ramsescamerabindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesCameraBinding>>::value_type& it) { return it->m_cameraBinding->serialize(builder);
        });

        // TODO Violin this code needs some redesign... Re-visit once we catch all errors and rewrite it
        // Ideas: keep at least some of the relationship infos (we need them for error checking anyway)
        // and don't traverse everything recursively to collect the data here
        // Or: add and use object IDs, like in ramses

        // TODO Violin this code can be designed better if the data related to links is less spread
        // Currently, the data is spread between the logic engine impls class (holds full info on nodes and properties)
        // and the LogicNodeConnector (holds link information). To serialize the links, both are required

        struct PropertyLocation
        {
            rlogic_serialization::ELogicNodeType parentLogicNodeType = rlogic_serialization::ELogicNodeType::Script;
            uint32_t owningLogicNodeIndex = 0;
            std::vector<uint32_t> cascadedChildIndex;
        };

        std::unordered_map<const PropertyImpl*, PropertyLocation> propertyLocation;

        std::function<void(const PropertyImpl&)> collectPropertyChildrenMetadata =
            [&propertyLocation, &collectPropertyChildrenMetadata]
        (const PropertyImpl& parentProperty)
        {
            const PropertyLocation parentMetadata = propertyLocation[&parentProperty];

            for (uint32_t childIdx = 0; childIdx < parentProperty.getChildCount(); ++childIdx)
            {
                const PropertyImpl& childProperty = *parentProperty.getChild(childIdx)->m_impl;
                PropertyLocation childMetadata = parentMetadata;
                assert(propertyLocation.end() == propertyLocation.find(&childProperty));
                childMetadata.cascadedChildIndex.push_back(childIdx);

                propertyLocation[&childProperty] = childMetadata;
                if (childProperty.getChildCount() != 0)
                {
                    collectPropertyChildrenMetadata(childProperty);
                }
            }
        };

        // TODO Violin consider moving this code to LogicNodeImpl after links fully implemented
        for (uint32_t scriptIndex = 0; scriptIndex < scripts.size(); ++scriptIndex)
        {
            const PropertyImpl* inputs = scripts[scriptIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs]  = PropertyLocation{rlogic_serialization::ELogicNodeType::Script, scriptIndex, {}};
            collectPropertyChildrenMetadata(*inputs);

            const PropertyImpl* outputs = scripts[scriptIndex]->getOutputs()->m_impl.get();
            propertyLocation[outputs] = PropertyLocation{rlogic_serialization::ELogicNodeType::Script, scriptIndex, {}};
            collectPropertyChildrenMetadata(*outputs);

        }

        for (uint32_t nbIndex = 0; nbIndex < nodeBindings.size(); ++nbIndex)
        {
            const PropertyImpl* inputs = nodeBindings[nbIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesNodeBinding, nbIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        for (uint32_t abIndex = 0; abIndex < appearanceBindings.size(); ++abIndex)
        {
            const PropertyImpl* inputs = appearanceBindings[abIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding, abIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        for (uint32_t cbIndex = 0; cbIndex < cameraBindings.size(); ++cbIndex)
        {
            const PropertyImpl* inputs = cameraBindings[cbIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesCameraBinding, cbIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }
        LogicNodeDependencies& nodeDependencies = m_apiObjects.getLogicNodeDependencies();

        // Refuse save() if logic graph has loops
        if (!nodeDependencies.getTopologicallySortedNodes())
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling saveToFile()!");
            return false;
        }

        const LinksMap& allLinks = nodeDependencies.getLinks();

        std::vector<flatbuffers::Offset<rlogic_serialization::Link>> links;
        links.reserve(allLinks.size());

        for (const auto& link : allLinks)
        {
            const PropertyLocation& srcProperty = propertyLocation[link.second];
            const PropertyLocation& targetProperty = propertyLocation[link.first];

            auto offset = rlogic_serialization::CreateLink(builder,
                srcProperty.parentLogicNodeType,
                targetProperty.parentLogicNodeType,
                srcProperty.owningLogicNodeIndex,
                targetProperty.owningLogicNodeIndex,
                builder.CreateVector(srcProperty.cascadedChildIndex),
                builder.CreateVector(targetProperty.cascadedChildIndex));
            links.emplace_back(offset);
        }

        ramses::RamsesVersion ramsesVersion = ramses::GetRamsesVersion();

        auto ramsesVersionOffset = rlogic_serialization::CreateVersion(builder,
            ramsesVersion.major,
            ramsesVersion.minor,
            ramsesVersion.patch,
            builder.CreateString(ramsesVersion.string));

        auto ramsesLogicVersionOffset = rlogic_serialization::CreateVersion(builder,
            g_PROJECT_VERSION_MAJOR,
            g_PROJECT_VERSION_MINOR,
            g_PROJECT_VERSION_PATCH,
            builder.CreateString(g_PROJECT_VERSION));

        auto logicEngine = rlogic_serialization::CreateLogicEngine(
            builder,
            ramsesVersionOffset,
            ramsesLogicVersionOffset,
            builder.CreateVector(luascripts),
            builder.CreateVector(ramsesnodebindings),
            builder.CreateVector(ramsesappearancebindings),
            builder.CreateVector(ramsescamerabindings),
            builder.CreateVector(links)
        );

        builder.Finish(logicEngine);

        if (!FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize()))
        {
            m_errors.add(fmt::format("Failed to save content to path '{}'!", filename));
            return false;
        }

        return true;
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects.getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects.getLogicNodeDependencies().unlink(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    ApiObjects& LogicEngineImpl::getApiObjects()
    {
        return m_apiObjects;
    }

    const ApiObjects& LogicEngineImpl::getApiObjects() const
    {
        return m_apiObjects;
    }
}
