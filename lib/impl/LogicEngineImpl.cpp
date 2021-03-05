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
#include "impl/LoggerImpl.h"
#include "internals/FileUtils.h"
#include "internals/TypeUtils.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/LogicNode.h"

#include "ramses-logic-build-config.h"
#include "generated/logicengine_gen.h"

#include "ramses-framework-api/RamsesFrameworkTypes.h"
#include "ramses-framework-api/RamsesVersion.h"
#include "ramses-client-api/SceneObject.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
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
            // TODO Violin need to report error here too! Error mechanism needs rethinking
            m_errors.add("Failed opening file " + std::string(filename) + "!");
            return nullptr;
        }

        std::string source(std::istreambuf_iterator<char>(iStream), std::istreambuf_iterator<char>{});
        return createLuaScriptInternal(source, filename, scriptName);
    }

    LuaScript* LogicEngineImpl::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        m_errors.clear();
        return createLuaScriptInternal(source, "", scriptName);
    }

    LuaScript* LogicEngineImpl::createLuaScriptInternal(std::string_view source, std::string_view filename, std::string_view scriptName)
    {
        auto scriptImpl = LuaScriptImpl::Create(m_luaState, source, scriptName, filename, m_errors);
        if (scriptImpl)
        {
            m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(scriptImpl)));
            auto script = m_scripts.back().get();
            m_logicNodeDependencies.addNode(script->m_impl);
            return script;
        }

        return nullptr;
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(std::string_view name)
    {
        m_errors.clear();
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Create(name)));
        auto ramsesBinding = m_ramsesNodeBindings.back().get();
        m_logicNodeDependencies.addNode(ramsesBinding->m_impl);
        return ramsesBinding;
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(std::string_view name)
    {
        m_errors.clear();
        m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(RamsesAppearanceBindingImpl::Create(name)));
        auto ramsesBinding = m_ramsesAppearanceBindings.back().get();
        m_logicNodeDependencies.addNode(ramsesBinding->m_impl);
        return ramsesBinding;
    }

    bool LogicEngineImpl::destroy(LogicNode& logicNode)
    {
        m_errors.clear();

        {
            auto luaScript = dynamic_cast<LuaScript*>(&logicNode);
            if (nullptr != luaScript)
            {
                return destroyInternal(*luaScript);
            }
        }

        {
            auto ramsesNodeBinding = dynamic_cast<RamsesNodeBinding*>(&logicNode);
            if (nullptr != ramsesNodeBinding)
            {
                return destroyInternal(*ramsesNodeBinding);
            }
        }

        {
            auto ramsesAppearanceBinding = dynamic_cast<RamsesAppearanceBinding*>(&logicNode);
            if (nullptr != ramsesAppearanceBinding)
            {
                return destroyInternal(*ramsesAppearanceBinding);
            }
        }

        m_errors.add(fmt::format("Tried to destroy object '{}' with unknown type", logicNode.getName()));
        return false;
    }

    bool LogicEngineImpl::destroyInternal(LuaScript& luaScript)
    {
        auto scriptIter = find_if(m_scripts.begin(), m_scripts.end(), [&](const std::unique_ptr<LuaScript>& script) {
            return script.get() == &luaScript;
        });
        if (scriptIter == m_scripts.end())
        {
            m_errors.add("Can't find script in logic engine!");
            return false;
        }

        m_logicNodeDependencies.removeNode(luaScript.m_impl);
        m_scripts.erase(scriptIter);
        return true;
    }

    bool LogicEngineImpl::destroyInternal(RamsesNodeBinding& ramsesNodeBinding)
    {
        auto nodeIter = find_if(m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), [&](const std::unique_ptr<RamsesNodeBinding>& nodeBinding)
            {
                return nodeBinding.get() == &ramsesNodeBinding;
            });

        if (nodeIter == m_ramsesNodeBindings.end())
        {
            m_errors.add("Can't find RamsesNodeBinding in logic engine!");
            return false;
        }

        m_logicNodeDependencies.removeNode(ramsesNodeBinding.m_impl);
        m_ramsesNodeBindings.erase(nodeIter);

        return true;
    }

    bool LogicEngineImpl::destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding)
    {
        auto appearanceIter = find_if(m_ramsesAppearanceBindings.begin(), m_ramsesAppearanceBindings.end(), [&](const std::unique_ptr<RamsesAppearanceBinding>& appearanceBinding) {
            return appearanceBinding.get() == &ramsesAppearanceBinding;
            });

        if (appearanceIter == m_ramsesAppearanceBindings.end())
        {
            m_errors.add("Can't find RamsesAppearanceBinding in logic engine!");
            return false;
        }

        m_logicNodeDependencies.removeNode(ramsesAppearanceBinding.m_impl);
        m_ramsesAppearanceBindings.erase(appearanceIter);

        return true;
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_logicNodeDependencies.isLinked(logicNode.m_impl);
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
                const PropertyImpl* output = m_logicNodeDependencies.getLinkedOutput(*child.m_impl);
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

        const std::optional<NodeVector> sortedNodes = m_logicNodeDependencies.getTopologicallySortedNodes();
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
                    m_errors.add(error);
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

    const std::vector<std::string>& LogicEngineImpl::getErrors() const
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

    // TODO Violin/Sven consider handling errors gracefully, e.g. don't change state when error occurs
    // Idea: collect data, and only move() in the end when everything was loaded correctly
    bool LogicEngineImpl::loadFromFile(std::string_view filename, ramses::Scene* scene)
    {
        m_errors.clear();
        m_scripts.clear();
        m_ramsesNodeBindings.clear();
        m_ramsesAppearanceBindings.clear();
        m_logicNodeDependencies = {};

        std::optional<std::vector<char>> maybeBytesFromFile = FileUtils::LoadBinary(std::string(filename));
        if (!maybeBytesFromFile)
        {
            m_errors.add(fmt::format("Failed to load file '{}'", filename));
            return false;
        }

        const auto logicEngine = rlogic_serialization::GetLogicEngine((*maybeBytesFromFile).data());

        if (nullptr == logicEngine || nullptr == logicEngine->ramsesVersion() || nullptr == logicEngine->rlogicVersion())
        {
            m_errors.add(fmt::format("File '{}' doesn't contain logic engine data with readable version specifiers", filename));
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading file '{}'! Expected Ramses version {}.x.x but found {} in file",
                filename, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()));
            return false;
        }

        const auto& rlogicVersion = *logicEngine->rlogicVersion();
        if (!CheckLogicVersionFromFile(rlogicVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading file '{}'! Expected version {}.{}.x but found {} in file",
                filename, g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR,
                rlogicVersion.v_string()->string_view()));
            return false;
        }

        assert(nullptr != logicEngine->luascripts());
        assert(nullptr != logicEngine->ramsesnodebindings());
        assert(nullptr != logicEngine->ramsesappearancebindings());

        const auto luascripts = logicEngine->luascripts();
        assert(nullptr != luascripts);

        if (luascripts->size() != 0)
        {
            m_scripts.reserve(luascripts->size());
        }

        for (auto script : *luascripts)
        {
            // script can't be null. Already handled by flatbuffers
            assert(nullptr != script);
            auto newScript = LuaScriptImpl::Create(m_luaState, script, m_errors);
            if (nullptr != newScript)
            {
                m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(newScript)));
                m_logicNodeDependencies.addNode(m_scripts.back()->m_impl);
            }
        }

        const auto ramsesnodebindings = logicEngine->ramsesnodebindings();
        assert(nullptr != ramsesnodebindings);

        if (ramsesnodebindings->size() != 0)
        {
            m_ramsesNodeBindings.reserve(ramsesnodebindings->size());
        }

        for (auto rNodeBinding : *ramsesnodebindings)
        {
            assert(nullptr != rNodeBinding);
            ramses::Node*           ramsesNode = nullptr;
            ramses::sceneObjectId_t objectId(rNodeBinding->ramsesNode());

            if (objectId.isValid())
            {
                auto optional_ramsesNode = findRamsesNodeInScene(rNodeBinding->logicnode(), scene, objectId);
                if (!optional_ramsesNode)
                {
                    return false;
                }
                ramsesNode = *optional_ramsesNode;
            }
            auto newBindingImpl = RamsesNodeBindingImpl::Create(*rNodeBinding, ramsesNode);
            assert(nullptr != newBindingImpl);
            m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::move(newBindingImpl)));
            m_logicNodeDependencies.addNode(m_ramsesNodeBindings.back()->m_impl);
        }

        const auto ramsesappearancebindings = logicEngine->ramsesappearancebindings();

        if (ramsesappearancebindings->size() != 0)
        {
            m_ramsesAppearanceBindings.reserve(ramsesappearancebindings->size());
        }

        for (auto rAppearanceBinding : *ramsesappearancebindings)
        {
            assert(nullptr != rAppearanceBinding);

            ramses::Appearance*     ramsesAppearance = nullptr;
            ramses::sceneObjectId_t objectId(rAppearanceBinding->ramsesAppearance());

            if (objectId.isValid())
            {
                auto optional_ramsesAppearance = findRamsesAppearanceInScene(rAppearanceBinding->logicnode(), scene, objectId);
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

            m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::move(newBinding)));
            m_logicNodeDependencies.addNode(m_ramsesAppearanceBindings.back()->m_impl);
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
                assert(rLink->sourceLogicNodeId() < m_scripts.size());
                srcLogicNode = m_scripts[rLink->sourceLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesNodeBinding:
            case rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding:
                assert(false && "Bindings can't be the source node of a link!");
            }
            assert(srcLogicNode != nullptr);

            const LogicNode* tgtLogicNode = nullptr;
            switch (rLink->targetLogicNodeType())
            {
            case rlogic_serialization::ELogicNodeType::Script:
                assert(rLink->targetLogicNodeId() < m_scripts.size());
                tgtLogicNode = m_scripts[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesNodeBinding:
                assert(rLink->targetLogicNodeId() < m_ramsesNodeBindings.size());
                tgtLogicNode = m_ramsesNodeBindings[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding:
                assert(rLink->targetLogicNodeId() < m_ramsesAppearanceBindings.size());
                tgtLogicNode = m_ramsesAppearanceBindings[rLink->targetLogicNodeId()].get();
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

            // TODO Violin/Sven/Tobias discuss how we can simulate failures here, and on other places like this
            // where an error can only be triggered with binary file changes or incompatible versions
            const bool success = link(*sourceProperty, *targetProperty);
            if (!success)
            {
                m_errors.add(
                    fmt::format("Fatal error during loading from file! Could not link property '{}' (from LogicNode {}) to property '{}' (from LogicNode {})!",
                        sourceProperty->getName(),
                        srcLogicNode->getName(),
                        targetProperty->getName(),
                        tgtLogicNode->getName()
                    ));
                return false;
            }
        }

        return true;
    }

    ramses::SceneObject* LogicEngineImpl::findRamsesSceneObjectInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        if (nullptr == scene)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized ramses logic object '{}' points to a Ramses object (id: {}) , but no Ramses scene was provided to resolve the Ramses object!",
                            logicNode->name()->string_view().data(),
                            objectId.getValue()
                    ));
            return nullptr;
        }

        ramses::SceneObject* sceneObject = scene->findObjectById(objectId);

        if (nullptr == sceneObject)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized ramses logic object {} points to a Ramses object (id: {}) which couldn't be found in the provided scene!",
                            logicNode->name()->string_view().data(),
                            objectId.getValue()));
            return nullptr;
        }

        return sceneObject;
    }

    bool LogicEngineImpl::allBindingsReferToSameRamsesScene()
    {
        // Optional because it's OK that no Ramses object is referenced at all (and thus no ramses scene)
        std::optional<ramses::sceneId_t> sceneId;

        for (const auto& binding : m_ramsesNodeBindings)
        {
            const ramses::Node* node = binding->m_nodeBinding->getRamsesNode();
            if (node)
            {
                const ramses::sceneId_t nodeSceneId = node->getSceneId();
                if (!sceneId)
                {
                    sceneId = nodeSceneId;
                }

                if (*sceneId != nodeSceneId)
                {
                    m_errors.add(fmt::format("Ramses node '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                        node->getName(), nodeSceneId.getValue(), sceneId->getValue()));
                    return false;
                }
            }
        }

        for (const auto& binding : m_ramsesAppearanceBindings)
        {
            const ramses::Appearance* appearance = binding->m_appearanceBinding->getRamsesAppearance();
            if (appearance)
            {
                const ramses::sceneId_t appearanceSceneId = appearance->getSceneId();
                if (!sceneId)
                {
                    sceneId = appearanceSceneId;
                }

                if (*sceneId != appearanceSceneId)
                {
                    m_errors.add(fmt::format("Ramses appearance '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                        appearance->getName(), appearanceSceneId.getValue(), sceneId->getValue()));
                    return false;
                }
            }
        }

        return true;
    }

    std::optional<ramses::Node*> LogicEngineImpl::findRamsesNodeInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        ramses::SceneObject*    sceneObject = findRamsesSceneObjectInScene(logicNode, scene, objectId);
        ramses::Node* ramsesNode = nullptr;

        if (nullptr == sceneObject)
        {
            // Very hard to test. Shall we change it to an assert?
            return std::nullopt;
        }

        ramsesNode = ramses::RamsesUtils::TryConvert<ramses::Node>(*sceneObject);

        if (nullptr == ramsesNode)
        {
            // Can't be unit tested directly unfortunately because there is no way to force ramses to recreate an object with the same ID
            // But in order to be safe, we have to check that the cast to Node worked here
            m_errors.add("Fatal error during loading from file! Node binding points to a Ramses scene object which is not of type 'Node'!");
        }

        return std::make_optional(ramsesNode);
    }

    std::optional<ramses::Appearance*> LogicEngineImpl::findRamsesAppearanceInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId)
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNode, scene, objectId);
        ramses::Appearance* ramsesAppearance  = nullptr;

        if (nullptr == sceneObject)
        {
            // Very hard to test. Shall we change it to an assert?
            return std::nullopt;
        }

        ramsesAppearance = ramses::RamsesUtils::TryConvert<ramses::Appearance>(*sceneObject);

        if (nullptr == ramsesAppearance)
        {
            // Can't be unit tested directly unfortunately because there is no way to force ramses to recreate an object with the same ID
            // But in order to be safe, we have to check that the cast to Node worked here
            m_errors.add("Fatal error during loading from file! Appearance binding points to a Ramses scene object which is not of type 'Appearance'!");
        }

        return std::make_optional(ramsesAppearance);
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
        for (const auto& script : m_scripts)
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
        for (const auto& nodeBinding : m_ramsesNodeBindings)
        {
            if (nodeBinding->m_impl.get().isDirty())
            {
                return true;
            }
        }

        for (const auto& appBinding : m_ramsesAppearanceBindings)
        {
            if (appBinding->m_impl.get().isDirty())
            {
                return true;
            }
        }

        return false;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename)
    {
        m_errors.clear();

        if (!allBindingsReferToSameRamsesScene())
        {
            m_errors.add("Can't save a logic engine to file while it has references to more than one Ramses scene!");
            return false;
        }

        if (bindingsDirty())
        {
            LOG_WARN("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!");
        }

        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(m_scripts.size());

        std::transform(m_scripts.begin(), m_scripts.end(), std::back_inserter(luascripts), [&builder](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) {
            return it->m_script->serialize(builder);
        });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(m_ramsesNodeBindings.size());

        std::transform(m_ramsesNodeBindings.begin(),
                       m_ramsesNodeBindings.end(),
                       std::back_inserter(ramsesnodebindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) { return it->m_nodeBinding->serialize(builder); });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(m_ramsesAppearanceBindings.size());

        std::transform(m_ramsesAppearanceBindings.begin(),
                       m_ramsesAppearanceBindings.end(),
                       std::back_inserter(ramsesappearancebindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesAppearanceBinding>>::value_type& it) { return it->m_appearanceBinding->serialize(builder);
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
        for (uint32_t scriptIndex = 0; scriptIndex < m_scripts.size(); ++scriptIndex)
        {
            const PropertyImpl* inputs = m_scripts[scriptIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs]  = PropertyLocation{rlogic_serialization::ELogicNodeType::Script, scriptIndex, {}};
            collectPropertyChildrenMetadata(*inputs);

            const PropertyImpl* outputs = m_scripts[scriptIndex]->getOutputs()->m_impl.get();
            propertyLocation[outputs] = PropertyLocation{rlogic_serialization::ELogicNodeType::Script, scriptIndex, {}};
            collectPropertyChildrenMetadata(*outputs);

        }

        for (uint32_t nbIndex = 0; nbIndex < m_ramsesNodeBindings.size(); ++nbIndex)
        {
            const PropertyImpl* inputs = m_ramsesNodeBindings[nbIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesNodeBinding, nbIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        for (uint32_t abIndex = 0; abIndex < m_ramsesAppearanceBindings.size(); ++abIndex)
        {
            const PropertyImpl* inputs = m_ramsesAppearanceBindings[abIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding, abIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        // Refuse save() if logic graph has loops
        if (!m_logicNodeDependencies.getTopologicallySortedNodes())
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling saveToFile()!");
            return false;
        }

        const LinksMap& allLinks = m_logicNodeDependencies.getLinks();

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
            builder.CreateVector(links)
        );

        builder.Finish(logicEngine);

        return FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize());
    }

    ScriptsContainer& LogicEngineImpl::getScripts()
    {
        return m_scripts;
    }

    NodeBindingsContainer& LogicEngineImpl::getNodeBindings()
    {
        return m_ramsesNodeBindings;
    }

    AppearanceBindingsContainer& LogicEngineImpl::getAppearanceBindings()
    {
        return m_ramsesAppearanceBindings;
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_logicNodeDependencies.link(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_logicNodeDependencies.unlink(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    const LogicNodeDependencies& LogicEngineImpl::getLogicNodeDependencies() const
    {
        return m_logicNodeDependencies;
    }

}
