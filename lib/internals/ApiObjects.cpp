//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/ApiObjects.h"
#include "internals/ErrorReporting.h"
#include "internals/RamsesObjectResolver.h"

#include "ramses-logic-build-config.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"
#include "ramses-framework-api/RamsesVersion.h"

#include "generated/logicengine_gen.h"
#include "fmt/format.h"

namespace rlogic::internal
{
    LuaScript* ApiObjects::createLuaScript(SolState& solState, std::string_view source, std::string_view filename, std::string_view scriptName, ErrorReporting& errorReporting)
    {
        std::unique_ptr<LuaScriptImpl> scriptImpl = LuaScriptImpl::Create(solState, source, scriptName, filename, errorReporting);
        if (scriptImpl)
        {
            m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(scriptImpl)));
            LuaScript* script = m_scripts.back().get();
            registerLogicNode(*script);
            return script;
        }

        return nullptr;
    }

    RamsesNodeBinding* ApiObjects::createRamsesNodeBinding(std::string_view name)
    {
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::make_unique<RamsesNodeBindingImpl>(name)));
        RamsesNodeBinding* binding = m_ramsesNodeBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesAppearanceBinding* ApiObjects::createRamsesAppearanceBinding(std::string_view name)
    {
        m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::make_unique<RamsesAppearanceBindingImpl>(name)));
        RamsesAppearanceBinding* binding = m_ramsesAppearanceBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesCameraBinding* ApiObjects::createRamsesCameraBinding(std::string_view name)
    {
        m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::make_unique<RamsesCameraBindingImpl>(name)));
        RamsesCameraBinding* binding = m_ramsesCameraBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    void ApiObjects::registerLogicNode(LogicNode& logicNode)
    {
        m_reverseImplMapping.emplace(std::make_pair(&logicNode.m_impl.get(), &logicNode));
        m_logicNodeDependencies.addNode(logicNode.m_impl);
    }

    void ApiObjects::unregisterLogicNode(LogicNode& logicNode)
    {
        LogicNodeImpl& logicNodeImpl = logicNode.m_impl;
        auto implIter = m_reverseImplMapping.find(&logicNodeImpl);
        assert(implIter != m_reverseImplMapping.end());
        m_reverseImplMapping.erase(implIter);

        m_logicNodeDependencies.removeNode(logicNodeImpl);
    }

    bool ApiObjects::destroy(LogicNode& logicNode, ErrorReporting& errorReporting)
    {
        {
            auto luaScript = dynamic_cast<LuaScript*>(&logicNode);
            if (nullptr != luaScript)
            {
                return destroyInternal(*luaScript, errorReporting);
            }
        }

        {
            auto ramsesNodeBinding = dynamic_cast<RamsesNodeBinding*>(&logicNode);
            if (nullptr != ramsesNodeBinding)
            {
                return destroyInternal(*ramsesNodeBinding, errorReporting);
            }
        }

        {
            auto ramsesAppearanceBinding = dynamic_cast<RamsesAppearanceBinding*>(&logicNode);
            if (nullptr != ramsesAppearanceBinding)
            {
                return destroyInternal(*ramsesAppearanceBinding, errorReporting);
            }
        }


        {
            auto ramsesCameraBinding = dynamic_cast<RamsesCameraBinding*>(&logicNode);
            if (nullptr != ramsesCameraBinding)
            {
                return destroyInternal(*ramsesCameraBinding, errorReporting);
            }
        }

        errorReporting.add(fmt::format("Tried to destroy object '{}' with unknown type", logicNode.getName()), logicNode);
        return false;
    }

    bool ApiObjects::destroyInternal(LuaScript& luaScript, ErrorReporting& errorReporting)
    {
        auto scriptIter = find_if(m_scripts.begin(), m_scripts.end(), [&](const std::unique_ptr<LuaScript>& script) {
            return script.get() == &luaScript;
            });
        if (scriptIter == m_scripts.end())
        {
            errorReporting.add("Can't find script in logic engine!");
            return false;
        }

        auto implIter = m_reverseImplMapping.find(&luaScript.m_impl.get());
        assert(implIter != m_reverseImplMapping.end());
        m_reverseImplMapping.erase(implIter);

        m_logicNodeDependencies.removeNode(luaScript.m_impl);
        m_scripts.erase(scriptIter);
        return true;
    }

    bool ApiObjects::destroyInternal(RamsesNodeBinding& ramsesNodeBinding, ErrorReporting& errorReporting)
    {
        auto nodeIter = find_if(m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), [&](const std::unique_ptr<RamsesNodeBinding>& nodeBinding)
            {
                return nodeBinding.get() == &ramsesNodeBinding;
            });

        if (nodeIter == m_ramsesNodeBindings.end())
        {
            errorReporting.add("Can't find RamsesNodeBinding in logic engine!");
            return false;
        }

        unregisterLogicNode(ramsesNodeBinding);
        m_ramsesNodeBindings.erase(nodeIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding, ErrorReporting& errorReporting)
    {
        auto appearanceIter = find_if(m_ramsesAppearanceBindings.begin(), m_ramsesAppearanceBindings.end(), [&](const std::unique_ptr<RamsesAppearanceBinding>& appearanceBinding) {
            return appearanceBinding.get() == &ramsesAppearanceBinding;
            });

        if (appearanceIter == m_ramsesAppearanceBindings.end())
        {
            errorReporting.add("Can't find RamsesAppearanceBinding in logic engine!");
            return false;
        }

        unregisterLogicNode(ramsesAppearanceBinding);
        m_ramsesAppearanceBindings.erase(appearanceIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesCameraBinding& ramsesCameraBinding, ErrorReporting& errorReporting)
    {
        auto cameraIter = find_if(m_ramsesCameraBindings.begin(), m_ramsesCameraBindings.end(), [&](const std::unique_ptr<RamsesCameraBinding>& cameraBinding) {
            return cameraBinding.get() == &ramsesCameraBinding;
        });

        if (cameraIter == m_ramsesCameraBindings.end())
        {
            errorReporting.add("Can't find RamsesCameraBinding in logic engine!");
            return false;
        }

        unregisterLogicNode(ramsesCameraBinding);
        m_ramsesCameraBindings.erase(cameraIter);

        return true;
    }

    bool ApiObjects::checkBindingsReferToSameRamsesScene(ErrorReporting& errorReporting) const
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
                    errorReporting.add(fmt::format("Ramses node '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                        node->getName(), nodeSceneId.getValue(), sceneId->getValue()), *binding);
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
                    errorReporting.add(fmt::format("Ramses appearance '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                        appearance->getName(), appearanceSceneId.getValue(), sceneId->getValue()), *binding);
                    return false;
                }
            }
        }

        for (const auto& binding : m_ramsesCameraBindings)
        {
            const ramses::Camera* camera = binding->m_cameraBinding->getRamsesCamera();
            if (camera)
            {
                const ramses::sceneId_t cameraSceneId = camera->getSceneId();
                if (!sceneId)
                {
                    sceneId = cameraSceneId;
                }

                if (*sceneId != cameraSceneId)
                {
                    errorReporting.add(fmt::format("Ramses camera '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                        camera->getName(), cameraSceneId.getValue(), sceneId->getValue()), *binding);
                    return false;
                }
            }
        }

        return true;
    }

    ScriptsContainer& ApiObjects::getScripts()
    {
        return m_scripts;
    }

    const ScriptsContainer& ApiObjects::getScripts() const
    {
        return m_scripts;
    }

    NodeBindingsContainer& ApiObjects::getNodeBindings()
    {
        return m_ramsesNodeBindings;
    }

    const NodeBindingsContainer& ApiObjects::getNodeBindings() const
    {
        return m_ramsesNodeBindings;
    }

    AppearanceBindingsContainer& ApiObjects::getAppearanceBindings()
    {
        return m_ramsesAppearanceBindings;
    }

    const AppearanceBindingsContainer& ApiObjects::getAppearanceBindings() const
    {
        return m_ramsesAppearanceBindings;
    }

    CameraBindingsContainer& ApiObjects::getCameraBindings()
    {
        return m_ramsesCameraBindings;
    }

    const CameraBindingsContainer& ApiObjects::getCameraBindings() const
    {
        return m_ramsesCameraBindings;
    }

    LogicNodeDependencies& ApiObjects::getLogicNodeDependencies()
    {
        return m_logicNodeDependencies;
    }

    const LogicNodeDependencies& ApiObjects::getLogicNodeDependencies() const
    {
        return m_logicNodeDependencies;
    }

    LogicNode* ApiObjects::getApiObject(LogicNodeImpl& impl) const
    {
        auto apiObjectIter = m_reverseImplMapping.find(&impl);
        assert(apiObjectIter != m_reverseImplMapping.end());
        return apiObjectIter->second;
    }

    const std::unordered_map<LogicNodeImpl*, LogicNode*>& ApiObjects::getReverseImplMapping() const
    {
        return m_reverseImplMapping;
    }

    void ApiObjects::Serialize(const ApiObjects& apiObjects, flatbuffers::FlatBufferBuilder& builder)
    {
        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(apiObjects.m_scripts.size());

        std::transform(apiObjects.m_scripts.begin(), apiObjects.m_scripts.end(), std::back_inserter(luascripts),
            [&builder](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) { return LuaScriptImpl::Serialize(*it->m_script, builder); });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(apiObjects.m_ramsesNodeBindings.size());

        std::transform(apiObjects.m_ramsesNodeBindings.begin(),
            apiObjects.m_ramsesNodeBindings.end(),
            std::back_inserter(ramsesnodebindings),
            [&builder](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) { return RamsesNodeBindingImpl::Serialize(*it->m_nodeBinding, builder); });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(apiObjects.m_ramsesAppearanceBindings.size());

        std::transform(apiObjects.m_ramsesAppearanceBindings.begin(),
            apiObjects.m_ramsesAppearanceBindings.end(),
            std::back_inserter(ramsesappearancebindings),
            [&builder](const std::vector<std::unique_ptr<RamsesAppearanceBinding>>::value_type& it) { return RamsesAppearanceBindingImpl::Serialize(*it->m_appearanceBinding, builder); });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> ramsescamerabindings;
        ramsescamerabindings.reserve(apiObjects.m_ramsesCameraBindings.size());

        std::transform(apiObjects.m_ramsesCameraBindings.begin(),
            apiObjects.m_ramsesCameraBindings.end(),
            std::back_inserter(ramsescamerabindings),
            [&builder](const std::vector<std::unique_ptr<RamsesCameraBinding>>::value_type& it) { return RamsesCameraBindingImpl::Serialize(*it->m_cameraBinding, builder); });

        // TODO Violin this code needs some redesign... Re-visit once we catch all errors and rewrite it
        // Ideas: keep at least some of the relationship infos (we need them for error checking anyway)
        // and don't traverse everything recursively to collect the data here
        // Or: add and use object IDs, like in ramses

        // TODO Violin move this part of the serialization to LogicNodeDependencies

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

        for (uint32_t scriptIndex = 0; scriptIndex < apiObjects.m_scripts.size(); ++scriptIndex)
        {
            const PropertyImpl* inputs = apiObjects.m_scripts[scriptIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::Script, scriptIndex, {} };
            collectPropertyChildrenMetadata(*inputs);

            const PropertyImpl* outputs = apiObjects.m_scripts[scriptIndex]->getOutputs()->m_impl.get();
            propertyLocation[outputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::Script, scriptIndex, {} };
            collectPropertyChildrenMetadata(*outputs);

        }

        for (uint32_t nbIndex = 0; nbIndex < apiObjects.m_ramsesNodeBindings.size(); ++nbIndex)
        {
            const PropertyImpl* inputs = apiObjects.m_ramsesNodeBindings[nbIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesNodeBinding, nbIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        for (uint32_t abIndex = 0; abIndex < apiObjects.m_ramsesAppearanceBindings.size(); ++abIndex)
        {
            const PropertyImpl* inputs = apiObjects.m_ramsesAppearanceBindings[abIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding, abIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        for (uint32_t cbIndex = 0; cbIndex < apiObjects.m_ramsesCameraBindings.size(); ++cbIndex)
        {
            const PropertyImpl* inputs = apiObjects.m_ramsesCameraBindings[cbIndex]->getInputs()->m_impl.get();
            propertyLocation[inputs] = PropertyLocation{ rlogic_serialization::ELogicNodeType::RamsesCameraBinding, cbIndex, {} };
            collectPropertyChildrenMetadata(*inputs);
        }

        const LinksMap& allLinks = apiObjects.m_logicNodeDependencies.getLinks();

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
    }

    std::optional<ApiObjects> ApiObjects::Deserialize(
        SolState& solState,
        const rlogic_serialization::LogicEngine& logicEngineData,
        const IRamsesObjectResolver& ramsesResolver,
        const std::string& dataSourceDescription,
        ErrorReporting& errorReporting)
    {
        // Collect data here, only return if no error occurred
        ApiObjects apiObjects;

        // TODO Violin handle this fatal error gracefully
        // Also, traverse the entire deserialization and handle errors gracefully everywhere - it's not much work, and is a better way of dealing with file corruption
        assert(nullptr != logicEngineData.luascripts());
        assert(nullptr != logicEngineData.ramsesnodebindings());
        assert(nullptr != logicEngineData.ramsesappearancebindings());
        assert(nullptr != logicEngineData.ramsescamerabindings());

        const auto luascripts = logicEngineData.luascripts();
        assert(nullptr != luascripts);

        if (luascripts->size() != 0)
        {
            apiObjects.m_scripts.reserve(luascripts->size());
        }

        for (auto script : *luascripts)
        {
            // script can't be null. Already handled by flatbuffers
            assert(nullptr != script);
            auto newScript = LuaScriptImpl::Deserialize(solState, *script);
            assert (nullptr != newScript);
            apiObjects.m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(newScript)));
            apiObjects.registerLogicNode(*apiObjects.m_scripts.back());
        }

        const auto ramsesnodebindings = logicEngineData.ramsesnodebindings();
        assert(nullptr != ramsesnodebindings);

        if (ramsesnodebindings->size() != 0)
        {
            apiObjects.m_ramsesNodeBindings.reserve(ramsesnodebindings->size());
        }

        for (auto rNodeBinding : *ramsesnodebindings)
        {
            assert(nullptr != rNodeBinding);
            ramses::Node* ramsesNode = nullptr;
            ramses::sceneObjectId_t objectId(rNodeBinding->ramsesNode());

            if (objectId.isValid())
            {
                ramsesNode = ramsesResolver.findRamsesNodeInScene(rNodeBinding->logicnode()->name()->string_view(), objectId);
                if (!ramsesNode)
                {
                    return std::nullopt;
                }
            }
            apiObjects.m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Deserialize(*rNodeBinding, ramsesNode)));
            apiObjects.registerLogicNode(*apiObjects.m_ramsesNodeBindings.back());
        }

        const auto ramsesappearancebindings = logicEngineData.ramsesappearancebindings();

        if (ramsesappearancebindings->size() != 0)
        {
            apiObjects.m_ramsesAppearanceBindings.reserve(ramsesappearancebindings->size());
        }

        for (auto rAppearanceBinding : *ramsesappearancebindings)
        {
            assert(nullptr != rAppearanceBinding);
            ramses::Appearance* ramsesAppearance = nullptr;
            ramses::sceneObjectId_t objectId(rAppearanceBinding->ramsesAppearance());

            if (objectId.isValid())
            {
                ramsesAppearance = ramsesResolver.findRamsesAppearanceInScene(rAppearanceBinding->logicnode()->name()->string_view(), objectId);
                if (!ramsesAppearance)
                {
                    return std::nullopt;
                }
            }

            auto newBinding = RamsesAppearanceBindingImpl::Deserialize(*rAppearanceBinding, ramsesAppearance, errorReporting);
            if (nullptr == newBinding)
            {
                return std::nullopt;
            }

            apiObjects.m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::move(newBinding)));
            apiObjects.registerLogicNode(*apiObjects.m_ramsesAppearanceBindings.back());
        }

        const auto ramsescamerabindings = logicEngineData.ramsescamerabindings();

        if (ramsescamerabindings->size() != 0)
        {
            apiObjects.m_ramsesCameraBindings.reserve(ramsescamerabindings->size());
        }

        for (auto rCameraBinding : *ramsescamerabindings)
        {
            assert(nullptr != rCameraBinding);

            ramses::Camera* ramsesCamera = nullptr;
            ramses::sceneObjectId_t objectId(rCameraBinding->ramsesCamera());

            if (objectId.isValid())
            {
                ramsesCamera = ramsesResolver.findRamsesCameraInScene(rCameraBinding->logicnode()->name()->string_view(), objectId);
                if (!ramsesCamera)
                {
                    return std::nullopt;
                }
            }

            auto newBinding = RamsesCameraBindingImpl::Deserialize(*rCameraBinding, ramsesCamera);
            if (nullptr == newBinding)
            {
                return std::nullopt;
            }

            apiObjects.m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::move(newBinding)));
            apiObjects.registerLogicNode(*apiObjects.m_ramsesCameraBindings.back());
        }

        assert(logicEngineData.links());
        const auto& links = *logicEngineData.links();

        // TODO Violin open discussion if we want to handle errors differently here
        for (const auto rLink : links)
        {
            assert(nullptr != rLink);

            const LogicNode* srcLogicNode = nullptr;
            switch (rLink->sourceLogicNodeType())
            {
            case rlogic_serialization::ELogicNodeType::Script:
                assert(rLink->sourceLogicNodeId() < apiObjects.m_scripts.size());
                srcLogicNode = apiObjects.m_scripts[rLink->sourceLogicNodeId()].get();
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
                assert(rLink->targetLogicNodeId() < apiObjects.m_scripts.size());
                tgtLogicNode = apiObjects.m_scripts[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesNodeBinding:
                assert(rLink->targetLogicNodeId() < apiObjects.m_ramsesNodeBindings.size());
                tgtLogicNode = apiObjects.m_ramsesNodeBindings[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesAppearanceBinding:
                assert(rLink->targetLogicNodeId() < apiObjects.m_ramsesAppearanceBindings.size());
                tgtLogicNode = apiObjects.m_ramsesAppearanceBindings[rLink->targetLogicNodeId()].get();
                break;
            case rlogic_serialization::ELogicNodeType::RamsesCameraBinding:
                assert(rLink->targetLogicNodeId() < apiObjects.m_ramsesCameraBindings.size());
                tgtLogicNode = apiObjects.m_ramsesCameraBindings[rLink->targetLogicNodeId()].get();
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

            const bool success = apiObjects.m_logicNodeDependencies.link(*sourceProperty->m_impl, *targetProperty->m_impl, errorReporting);
            if (!success)
            {
                errorReporting.add(
                    fmt::format("Fatal error during loading from {}! Could not link property '{}' (from LogicNode {}) to property '{}' (from LogicNode {})!",
                        dataSourceDescription,
                        sourceProperty->getName(),
                        srcLogicNode->getName(),
                        targetProperty->getName(),
                        tgtLogicNode->getName()
                    ));
                return std::nullopt;
            }
        }

        // This syntax is compatible with GCC7 (c++11 converts automatically)
        return std::make_optional(std::move(apiObjects));
    }
}
