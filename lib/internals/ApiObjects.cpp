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

#include "generated/ApiObjectsGen.h"
#include "generated/RamsesAppearanceBindingGen.h"
#include "generated/RamsesBindingGen.h"
#include "generated/RamsesCameraBindingGen.h"
#include "generated/RamsesNodeBindingGen.h"
#include "generated/LinkGen.h"

#include "fmt/format.h"

namespace rlogic::internal
{
    LuaScript* ApiObjects::createLuaScript(SolState& solState, std::string_view source, std::string_view filename, std::string_view scriptName, ErrorReporting& errorReporting)
    {
        std::optional<CompiledScript> compiledScript = LuaScriptImpl::Compile(solState, source, scriptName, filename, errorReporting);
        if (compiledScript)
        {
            m_scripts.emplace_back(std::make_unique<LuaScript>(std::make_unique<LuaScriptImpl>(std::move(*compiledScript))));
            LuaScript* script = m_scripts.back().get();
            registerLogicNode(*script);
            return script;
        }

        return nullptr;
    }

    RamsesNodeBinding* ApiObjects::createRamsesNodeBinding(ramses::Node& ramsesNode, std::string_view name)
    {
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::make_unique<RamsesNodeBindingImpl>(ramsesNode, name)));
        RamsesNodeBinding* binding = m_ramsesNodeBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesAppearanceBinding* ApiObjects::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::make_unique<RamsesAppearanceBindingImpl>(ramsesAppearance, name)));
        RamsesAppearanceBinding* binding = m_ramsesAppearanceBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesCameraBinding* ApiObjects::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::make_unique<RamsesCameraBindingImpl>(ramsesCamera, name)));
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
            const ramses::Node& node = binding->m_nodeBinding->getRamsesNode();
            const ramses::sceneId_t nodeSceneId = node.getSceneId();
            if (!sceneId)
            {
                sceneId = nodeSceneId;
            }

            if (*sceneId != nodeSceneId)
            {
                errorReporting.add(fmt::format("Ramses node '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    node.getName(), nodeSceneId.getValue(), sceneId->getValue()), *binding);
                return false;
            }
        }

        for (const auto& binding : m_ramsesAppearanceBindings)
        {
            const ramses::Appearance& appearance = binding->m_appearanceBinding->getRamsesAppearance();
            const ramses::sceneId_t appearanceSceneId = appearance.getSceneId();
            if (!sceneId)
            {
                sceneId = appearanceSceneId;
            }

            if (*sceneId != appearanceSceneId)
            {
                errorReporting.add(fmt::format("Ramses appearance '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    appearance.getName(), appearanceSceneId.getValue(), sceneId->getValue()), *binding);
                return false;
            }
        }

        for (const auto& binding : m_ramsesCameraBindings)
        {
            const ramses::Camera& camera = binding->m_cameraBinding->getRamsesCamera();
            const ramses::sceneId_t cameraSceneId = camera.getSceneId();
            if (!sceneId)
            {
                sceneId = cameraSceneId;
            }

            if (*sceneId != cameraSceneId)
            {
                errorReporting.add(fmt::format("Ramses camera '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    camera.getName(), cameraSceneId.getValue(), sceneId->getValue()), *binding);
                return false;
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

    flatbuffers::Offset<rlogic_serialization::ApiObjects> ApiObjects::Serialize(const ApiObjects& apiObjects, flatbuffers::FlatBufferBuilder& builder)
    {
        SerializationMap serializationMap;

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(apiObjects.m_scripts.size());

        std::transform(apiObjects.m_scripts.begin(), apiObjects.m_scripts.end(), std::back_inserter(luascripts),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) {
                return LuaScriptImpl::Serialize(*it->m_script, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(apiObjects.m_ramsesNodeBindings.size());

        std::transform(apiObjects.m_ramsesNodeBindings.begin(),
            apiObjects.m_ramsesNodeBindings.end(),
            std::back_inserter(ramsesnodebindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) {
                return RamsesNodeBindingImpl::Serialize(*it->m_nodeBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(apiObjects.m_ramsesAppearanceBindings.size());

        std::transform(apiObjects.m_ramsesAppearanceBindings.begin(),
            apiObjects.m_ramsesAppearanceBindings.end(),
            std::back_inserter(ramsesappearancebindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesAppearanceBinding>>::value_type& it) {
                return RamsesAppearanceBindingImpl::Serialize(*it->m_appearanceBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> ramsescamerabindings;
        ramsescamerabindings.reserve(apiObjects.m_ramsesCameraBindings.size());

        std::transform(apiObjects.m_ramsesCameraBindings.begin(),
            apiObjects.m_ramsesCameraBindings.end(),
            std::back_inserter(ramsescamerabindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesCameraBinding>>::value_type& it) {
                return RamsesCameraBindingImpl::Serialize(*it->m_cameraBinding, builder, serializationMap);
            });

        const LinksMap& allLinks = apiObjects.m_logicNodeDependencies.getLinks();

        std::vector<flatbuffers::Offset<rlogic_serialization::Link>> links;
        links.reserve(allLinks.size());

        for (const auto& link : allLinks)
        {
            links.emplace_back(rlogic_serialization::CreateLink(builder,
                serializationMap.resolvePropertyOffset(*link.second),
                serializationMap.resolvePropertyOffset(*link.first)));
        }

        const auto logicEngine = rlogic_serialization::CreateApiObjects(
            builder,
            builder.CreateVector(luascripts),
            builder.CreateVector(ramsesnodebindings),
            builder.CreateVector(ramsesappearancebindings),
            builder.CreateVector(ramsescamerabindings),
            builder.CreateVector(links)
        );

        builder.Finish(logicEngine);

        return logicEngine;
    }

    std::optional<ApiObjects> ApiObjects::Deserialize(
        SolState& solState,
        const rlogic_serialization::ApiObjects& apiObjects,
        const IRamsesObjectResolver& ramsesResolver,
        const std::string& dataSourceDescription,
        ErrorReporting& errorReporting)
    {
        // Collect data here, only return if no error occurred
        ApiObjects deserialized;

        // Collect deserialized object mappings to resolve dependencies
        DeserializationMap deserializationMap;

        if (!apiObjects.luaScripts())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing scripts container!");
            return std::nullopt;
        }

        if (!apiObjects.nodeBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing node bindings container!");
            return std::nullopt;
        }

        if (!apiObjects.appearanceBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing appearance bindings container!");
            return std::nullopt;
        }

        if (!apiObjects.cameraBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing camera bindings container!");
            return std::nullopt;
        }

        if (!apiObjects.links())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing links container!");
            return std::nullopt;
        }

        const auto& luascripts = *apiObjects.luaScripts();
        deserialized.m_scripts.reserve(luascripts.size());

        for (const auto* script : luascripts)
        {
            // TODO Violin find ways to unit-test this case - also for other container types
            // Ideas: see if verifier catches it; or: disable flatbuffer's internal asserts if possible
            assert (script);
            std::unique_ptr<LuaScriptImpl> deserializedScript = LuaScriptImpl::Deserialize(solState, *script, errorReporting, deserializationMap);

            if (deserializedScript)
            {
                deserialized.m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(deserializedScript)));
                deserialized.registerLogicNode(*deserialized.m_scripts.back());
            }
            else
            {
                return std::nullopt;
            }
        }

        const auto& ramsesNodeBindings = *apiObjects.nodeBindings();

        deserialized.m_ramsesNodeBindings.reserve(ramsesNodeBindings.size());

        for (const auto* binding : ramsesNodeBindings)
        {
            assert (binding);
            std::unique_ptr<RamsesNodeBindingImpl> deserializedBinding = RamsesNodeBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized.m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::move(deserializedBinding)));
                deserialized.registerLogicNode(*deserialized.m_ramsesNodeBindings.back());
            }
            else
            {
                return std::nullopt;
            }
        }

        const auto& ramsesAppearanceBindings = *apiObjects.appearanceBindings();

        deserialized.m_ramsesAppearanceBindings.reserve(ramsesAppearanceBindings.size());

        for (const auto* binding : ramsesAppearanceBindings)
        {
            assert(binding);
            std::unique_ptr<RamsesAppearanceBindingImpl> deserializedBinding = RamsesAppearanceBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized.m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::move(deserializedBinding)));
                deserialized.registerLogicNode(*deserialized.m_ramsesAppearanceBindings.back());
            }
            else
            {
                return std::nullopt;
            }
        }

        const auto& ramsesCameraBindings = *apiObjects.cameraBindings();

        deserialized.m_ramsesCameraBindings.reserve(ramsesCameraBindings.size());

        for (const auto* binding : ramsesCameraBindings)
        {
            assert(binding);
            std::unique_ptr<RamsesCameraBindingImpl> deserializedBinding = RamsesCameraBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized.m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::move(deserializedBinding)));
                deserialized.registerLogicNode(*deserialized.m_ramsesCameraBindings.back());
            }
            else
            {
                return std::nullopt;
            }
        }

        const auto& links = *apiObjects.links();

        // TODO Violin move this code (serialization parts too) to LogicNodeDependencies
        for (const auto* rLink : links)
        {
            assert(rLink);

            if (!rLink->sourceProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link source property!");
                return std::nullopt;
            }

            if (!rLink->targetProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link target property!");
                return std::nullopt;
            }

            const rlogic_serialization::Property* sourceProp = rLink->sourceProperty();
            const rlogic_serialization::Property* targetProp = rLink->targetProperty();

            const bool success = deserialized.m_logicNodeDependencies.link(
                deserializationMap.resolvePropertyImpl(*sourceProp),
                deserializationMap.resolvePropertyImpl(*targetProp),
                errorReporting);
            // TODO Violin handle (and unit test!) this error properly. Consider these error cases:
            // - maliciously forged properties (not attached to any node anywhere)
            // - cycles! (we check this on serialization, but Murphy's law says if something can break, it will break)
            if (!success)
            {
                errorReporting.add(
                    fmt::format("Fatal error during loading from {}! Could not link property '{}' to property '{}'!",
                        dataSourceDescription,
                        sourceProp->name()->string_view(),
                        targetProp->name()->string_view()
                    ));
                return std::nullopt;
            }
        }

        // This syntax is compatible with GCC7 (c++11 converts automatically)
        return std::make_optional(std::move(deserialized));
    }

    bool ApiObjects::isDirty() const
    {
        // TODO Violin improve internal management of logic nodes so that we don't have to loop over three
        // different containers below which all call a method on LogicNode
        return std::any_of(m_scripts.cbegin(), m_scripts.cend(), [](const auto& s) { return s->m_impl.get().isDirty(); })
            || bindingsDirty();
    }

    bool ApiObjects::bindingsDirty() const
    {
        return
            std::any_of(m_ramsesNodeBindings.cbegin(), m_ramsesNodeBindings.cend(), [](const auto& b) { return b->m_impl.get().isDirty(); }) ||
            std::any_of(m_ramsesAppearanceBindings.cbegin(), m_ramsesAppearanceBindings.cend(), [](const auto& b) { return b->m_impl.get().isDirty(); }) ||
            std::any_of(m_ramsesCameraBindings.cbegin(), m_ramsesCameraBindings.cend(), [](const auto& b) { return b->m_impl.get().isDirty(); });
    }
}
