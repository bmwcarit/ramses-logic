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
#include "ramses-utils.h"

#include <string>
#include <fstream>
#include <streambuf>

#include "flatbuffers/util.h"
#include "fmt/format.h"
#include "impl/LoggerImpl.h"

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
            setupLogicNodeInternal(*script);
            return script;
        }

        return nullptr;
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(std::string_view name)
    {
        m_errors.clear();
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Create(name)));
        auto ramsesBinding = m_ramsesNodeBindings.back().get();
        m_ramsesBindings.emplace_back(ramsesBinding);
        setupLogicNodeInternal(*ramsesBinding);
        return ramsesBinding;
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(std::string_view name)
    {
        m_errors.clear();
        m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(RamsesAppearanceBindingImpl::Create(name)));
        auto ramsesBinding = m_ramsesAppearanceBindings.back().get();
        m_ramsesBindings.emplace_back(ramsesBinding);
        setupLogicNodeInternal(*ramsesBinding);
        return ramsesBinding;
    }

    void LogicEngineImpl::setupLogicNodeInternal(LogicNode& logicNode)
    {
        m_disconnectedNodes.insert(&logicNode.m_impl.get());
        m_logicNodes.insert(&logicNode.m_impl.get());
    }

    bool LogicEngineImpl::destroy(LogicNode& logicNode)
    {
        m_disconnectedNodes.erase(&logicNode.m_impl.get());
        m_logicNodes.erase(&logicNode.m_impl.get());
        m_errors.clear();

        unlinkAll(logicNode);

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

        m_logicNodeGraph.removeLinksForNode((*scriptIter)->m_impl);
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

        m_ramsesBindings.erase(std::find(m_ramsesBindings.begin(), m_ramsesBindings.end(), nodeIter->get()));
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

        m_ramsesBindings.erase(std::find(m_ramsesBindings.begin(), m_ramsesBindings.end(), appearanceIter->get()));
        m_ramsesAppearanceBindings.erase(appearanceIter);

        return true;
    }

    void LogicEngineImpl::unlinkAll(LogicNode& logicNode)
    {
        std::unordered_set<const LogicNodeImpl*> possibleDisconnectedNodes;
        {
            const auto inputs = logicNode.getInputs();
            if (inputs != nullptr)
            {
                const auto linkedLogicNodes = getAllLinkedLogicNodesOfInput(*inputs->m_impl);
                possibleDisconnectedNodes.insert(linkedLogicNodes.begin(), linkedLogicNodes.end());
            }
        }

        {
            auto outputs = logicNode.getOutputs();
            if (outputs != nullptr)
            {
                const auto linkedLogicNodes = getAllLinkedLogicNodesOfOutput(*outputs->m_impl);
                possibleDisconnectedNodes.insert(linkedLogicNodes.begin(), linkedLogicNodes.end());
            }
        }

        m_logicNodeConnector.unlinkAll(logicNode.m_impl);

        for (const auto possibleDisconnectedNode : possibleDisconnectedNodes)
        {
            if (!m_logicNodeConnector.isLinked(*possibleDisconnectedNode))
            {
                m_disconnectedNodes.insert(const_cast<LogicNodeImpl*>(possibleDisconnectedNode)); // NOLINT(cppcoreguidelines-pro-type-const-cast) We do this to keep all Parameters in API const
            }
        }
    }


    std::unordered_set<const LogicNodeImpl*> LogicEngineImpl::getAllLinkedLogicNodesOfInput(PropertyImpl& property)
    {
        std::unordered_set<const LogicNodeImpl*> linkedLogicNodes;
        {
            const auto inputCount = property.getChildCount();

            for (size_t i = 0; i < inputCount; ++i)
            {
                const auto childProperty = property.getChild(i);
                const auto linkedProperty = m_logicNodeConnector.getLinkedOutput(*childProperty->m_impl);
                if (nullptr != linkedProperty)
                {
                    linkedLogicNodes.insert(&linkedProperty->getLogicNode());
                }
                auto childProperties = getAllLinkedLogicNodesOfInput(*childProperty->m_impl);
                linkedLogicNodes.insert(childProperties.begin(), childProperties.end());
            }
        }
        return linkedLogicNodes;
    }

    std::unordered_set<const LogicNodeImpl*> LogicEngineImpl::getAllLinkedLogicNodesOfOutput(PropertyImpl& property)
    {
        std::unordered_set<const LogicNodeImpl*> linkedLogicNodes;
        {
            const auto outputCount = property.getChildCount();

            for (size_t i = 0; i < outputCount; ++i)
            {
                const auto childProperty = property.getChild(i);
                const auto linkedProperty = m_logicNodeConnector.getLinkedInput(*childProperty->m_impl);
                if (nullptr != linkedProperty)
                {
                    linkedLogicNodes.insert(&linkedProperty->getLogicNode());
                }
                auto childProperties = getAllLinkedLogicNodesOfOutput(*childProperty->m_impl);
                linkedLogicNodes.insert(childProperties.begin(), childProperties.end());
            }
        }
        return linkedLogicNodes;
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_disconnectedNodes.find(&logicNode.m_impl.get()) == m_disconnectedNodes.end();
    }

    void LogicEngineImpl::updateLinksRecursive(Property& inputProperty)
    {
        // TODO Violin consider providing property iterator - seems it's needed quite often, would make access more convenient also for users
        const auto inputCount = inputProperty.getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            Property& child = *inputProperty.getChild(i);

            // TODO Violin provide type utils to check for complex types - this doesn't scale well
            if (child.getType() == EPropertyType::Struct || child.getType() == EPropertyType::Array)
            {
                updateLinksRecursive(child);
            }
            else
            {
                const PropertyImpl* output = m_logicNodeConnector.getLinkedOutput(*child.m_impl);
                if (nullptr != output)
                {
                    child.m_impl->set(*output);
                }
            }
        }
    }

    bool LogicEngineImpl::update(bool disableDirtyTracking)
    {
        m_errors.clear();
        LOG_DEBUG("Begin update");

        for (auto unlinkedNode : m_disconnectedNodes)
        {
            if (disableDirtyTracking || unlinkedNode->isDirty())
            {
                LOG_DEBUG("Updating LogicNode '{}'", unlinkedNode->getName());
                if (!unlinkedNode->update())
                {
                    const auto& errors = unlinkedNode->getErrors();
                    for (const auto& error : errors)
                    {
                        m_errors.add(error);
                    }
                    return false;
                }
            }
            else
            {
                LOG_DEBUG("Skip update of LogicNode '{}' because no input has changed since the last update", unlinkedNode->getName());
            }
            unlinkedNode->setDirty(false);
        }

        for (auto logicNode : m_logicNodeGraph)
        {
            updateLinksRecursive(*logicNode->getInputs());

            if (disableDirtyTracking || logicNode->isDirty())
            {
                LOG_DEBUG("Updating LogicNode '{}'", logicNode->getName());
                if (!logicNode->update())
                {
                    const auto& errors = logicNode->getErrors();
                    for (const auto& error : errors)
                    {
                        m_errors.add(error);
                    }
                    return false;
                }
            }
            else
            {
                LOG_DEBUG("Skip update of LogicNode '{}' because no input has changed since the last update", logicNode->getName());
            }
            logicNode->setDirty(false);
        }

        for (auto& ramsesBinding : m_ramsesBindings)
        {
            updateLinksRecursive(*ramsesBinding->getInputs());
            if (disableDirtyTracking || ramsesBinding->m_impl.get().isDirty())
            {
                LOG_DEBUG("Updating RamsesBinding '{}'", ramsesBinding->getName());
                bool success = ramsesBinding->m_impl.get().update();
                assert(success && "Bindings update can never fail!");
                (void)success;
            }
            else
            {
                LOG_DEBUG("Skip update of RamsesBinding '{}' because no input has changed since the last update", ramsesBinding->getName());
            }
            ramsesBinding->m_impl.get().setDirty(false);
        }
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
        m_ramsesBindings.clear();
        m_ramsesNodeBindings.clear();
        m_ramsesAppearanceBindings.clear();
        m_disconnectedNodes.clear();
        m_logicNodes.clear();

        std::string sFilename(filename);
        std::string buf;
        if (!flatbuffers::LoadFile(sFilename.c_str(), true, &buf))
        {
            m_errors.add(fmt::format("Failed to load file '{}'", sFilename));
            return false;
        }

        const auto logicEngine = rlogic_serialization::GetLogicEngine(buf.data());

        if (nullptr == logicEngine || nullptr == logicEngine->ramsesVersion() || nullptr == logicEngine->rlogicVersion())
        {
            m_errors.add(fmt::format("File '{}' doesn't contain logic engine data with readable version specifiers", sFilename));
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading file '{}'! Expected Ramses version {}.x.x but found {} in file",
                sFilename, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()));
            return false;
        }

        const auto& rlogicVersion = *logicEngine->rlogicVersion();
        if (!CheckLogicVersionFromFile(rlogicVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading file '{}'! Expected version {}.{}.x but found {} in file",
                sFilename, g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR,
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
                setupLogicNodeInternal(*m_scripts.back());
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
            m_ramsesBindings.emplace_back(m_ramsesNodeBindings.back().get());
            setupLogicNodeInternal(*m_ramsesBindings.back());
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
            m_ramsesBindings.emplace_back(m_ramsesAppearanceBindings.back().get());
            setupLogicNodeInternal(*m_ramsesBindings.back());
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

    bool LogicEngineImpl::saveToFile(std::string_view filename)
    {
        m_errors.clear();
        // TODO Violin/Sven/Tobias implement check belonging of nodes to scene after Ramses has API to get scene of node

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

        auto& allLinks = m_logicNodeConnector.getLinks();

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
        std::string sFileName(filename);

        return flatbuffers::SaveFile(sFileName.c_str(), reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize(), true); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) We know what we do and this is only solvable with reinterpret-cast
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

        if (m_logicNodes.find(&sourceProperty.m_impl->getLogicNode()) == m_logicNodes.end())
        {
            m_errors.add(fmt::format("LogicNode '{}' is not an instance of this LogicEngine", sourceProperty.m_impl->getLogicNode().getName()));
            return false;
        }

        if (m_logicNodes.find(&targetProperty.m_impl->getLogicNode()) == m_logicNodes.end())
        {
            m_errors.add(fmt::format("LogicNode '{}' is not an instance of this LogicEngine", targetProperty.m_impl->getLogicNode().getName()));
            return false;
        }

        if (&sourceProperty.m_impl->getLogicNode() == &targetProperty.m_impl->getLogicNode())
        {
            m_errors.add("SourceNode and TargetNode are equal");
            return false;
        }


        if (!(sourceProperty.m_impl->isOutput() && targetProperty.m_impl->isInput()))
        {
            std::string_view lhsType = sourceProperty.m_impl->isOutput() ? "output" : "input";
            std::string_view rhsType = targetProperty.m_impl->isOutput() ? "output" : "input";
            m_errors.add(fmt::format("Failed to link {} property '{}' to {} property '{}'. Only outputs can be linked to inputs", lhsType, sourceProperty.getName(), rhsType, targetProperty.getName()));
            return false;
        }

        if (sourceProperty.getType() != targetProperty.getType())
        {
            m_errors.add(fmt::format("Types of source property '{}:{}' does not match target property '{}:{}'",
                                              sourceProperty.getName(),
                                              GetLuaPrimitiveTypeName(sourceProperty.getType()),
                                              targetProperty.getName(),
                                              GetLuaPrimitiveTypeName(targetProperty.getType())));
            return false;
        }

        // TODO Violin solve this in a more generic way, it will break as soon as we add other complex types
        if (sourceProperty.getType() == EPropertyType::Struct ||
            sourceProperty.getType() == EPropertyType::Array)
        {
            m_errors.add(fmt::format("Can't link properties of complex types directly, currently only primitive properties can be linked"));
            return false;
        }


        // TODO Violin/Sven try to find a way to avoid const_cast
        auto& _targetProperty = const_cast<Property&>(targetProperty); // NOLINT(cppcoreguidelines-pro-type-const-cast) We do this to keep all Parameters in API const
        auto& _sourceProperty = const_cast<Property&>(sourceProperty); // NOLINT(cppcoreguidelines-pro-type-const-cast) We do this to keep all Parameters in API const
        auto&  targetNode      = _targetProperty.m_impl->getLogicNode();
        auto&  sourceNode      = _sourceProperty.m_impl->getLogicNode();

        if(!m_logicNodeConnector.link(*_sourceProperty.m_impl, *_targetProperty.m_impl))
        {
            m_errors.add(fmt::format("The property '{}' of LogicNode '{}' is already linked to the property '{}' of LogicNode '{}'",
                                              sourceProperty.getName(),
                                              sourceNode.getName(),
                                              targetProperty.getName(),
                                              targetNode.getName()
            ));
            return false;
        }

        m_disconnectedNodes.erase(&sourceNode);
        m_disconnectedNodes.erase(&targetNode);
        m_logicNodeGraph.addLink(sourceNode, targetNode);
        targetNode.setDirty(true);

        return true;
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        // TODO Violin/Sven try to find a way to avoid const_cast
        auto& _targetProperty = const_cast<Property&>(targetProperty); // NOLINT(cppcoreguidelines-pro-type-const-cast) We do this to keep all Parameters in API const

        if(!m_logicNodeConnector.unlink(*_targetProperty.m_impl))
        {
            m_errors.add(fmt::format("No link available from source property '{}' to target property '{}'", sourceProperty.getName(), targetProperty.getName()));
            return false;
        }

        auto& sourceNode = sourceProperty.m_impl->getLogicNode();
        auto& targetNode = targetProperty.m_impl->getLogicNode();

        targetNode.setDirty(true);

        if (!m_logicNodeConnector.isLinked(sourceNode))
        {
            m_disconnectedNodes.insert(&sourceNode);
        }
        if (!m_logicNodeConnector.isLinked(targetNode))
        {
            m_disconnectedNodes.insert(&targetNode);
        }

        m_logicNodeGraph.removeLink(sourceNode, targetNode);

        return true;
    }

    const LogicNodeConnector& LogicEngineImpl::getLogicNodeConnector() const
    {
        return m_logicNodeConnector;
    }

    const LogicNodeGraph& LogicEngineImpl::getLogicNodeGraph() const
    {
        return m_logicNodeGraph;
    }
}
