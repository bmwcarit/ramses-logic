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
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"

#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"
#include "impl/DataArrayImpl.h"
#include "impl/AnimationNodeImpl.h"

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
#include "generated/DataArrayGen.h"
#include "generated/AnimationNodeGen.h"

#include "fmt/format.h"

namespace rlogic::internal
{
    ApiObjects::ApiObjects() = default;
    ApiObjects::~ApiObjects() noexcept = default;

    bool ApiObjects::checkLuaModules(const ModuleMapping& moduleMapping, ErrorReporting& errorReporting)
    {
        for (const auto& module : moduleMapping)
        {
            if (getLuaModules().cend() == std::find_if(getLuaModules().cbegin(), getLuaModules().cend(),
                [&module](const auto& m) { return m.get() == module.second; }))
            {
                errorReporting.add(fmt::format("Failed to map Lua module '{}'! It was created on a different instance of LogicEngine.", module.first), module.second);
                return false;
            }
        }

        return true;
    }

    LuaScript* ApiObjects::createLuaScript(
        std::string_view source,
        const LuaConfigImpl& config,
        std::string_view scriptName,
        ErrorReporting& errorReporting)
    {
        const ModuleMapping& modules = config.getModuleMapping();
        if (!checkLuaModules(modules, errorReporting))
            return nullptr;

        std::optional<LuaCompiledScript> compiledScript = LuaCompilationUtils::CompileScript(
            *m_solState,
            modules,
            config.getStandardModules(),
            std::string{ source },
            scriptName,
            errorReporting);

        if (!compiledScript)
            return nullptr;

        m_scripts.emplace_back(std::make_unique<LuaScript>(std::make_unique<LuaScriptImpl>(std::move(*compiledScript), scriptName)));
        LuaScript* script = m_scripts.back().get();
        registerLogicNode(*script);
        return script;
    }

    LuaModule* ApiObjects::createLuaModule(
        std::string_view source,
        const LuaConfigImpl& config,
        std::string_view moduleName,
        ErrorReporting& errorReporting)
    {
        const ModuleMapping& modules = config.getModuleMapping();
        if (!checkLuaModules(modules, errorReporting))
            return nullptr;

        std::optional<LuaCompiledModule> compiledModule = LuaCompilationUtils::CompileModule(
            *m_solState,
            modules,
            config.getStandardModules(),
            std::string{source},
            moduleName,
            errorReporting);

        if (!compiledModule)
            return nullptr;

        m_luaModules.emplace_back(std::make_unique<LuaModule>(std::make_unique<LuaModuleImpl>(std::move(*compiledModule), moduleName)));
        return m_luaModules.back().get();
    }

    RamsesNodeBinding* ApiObjects::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name)
    {
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::make_unique<RamsesNodeBindingImpl>(ramsesNode, rotationType, name)));
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

    template <typename T>
    DataArray* ApiObjects::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        // make copy of users data and move into data array
        std::vector<T> dataCopy = data;
        auto impl = std::make_unique<DataArrayImpl>(std::move(dataCopy), name);
        m_dataArrays.push_back(std::make_unique<DataArray>(std::move(impl)));
        return m_dataArrays.back().get();
    }

    AnimationNode* ApiObjects::createAnimationNode(const AnimationChannels& channels, std::string_view name)
    {
        m_animationNodes.push_back(std::make_unique<AnimationNode>(std::make_unique<AnimationNodeImpl>(channels, name)));
        auto* animation = m_animationNodes.back().get();
        registerLogicNode(*animation);
        return animation;
    }

    void ApiObjects::registerLogicNode(LogicNode& logicNode)
    {
        m_reverseImplMapping.emplace(std::make_pair(&logicNode.m_impl, &logicNode));
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

    bool ApiObjects::destroy(LogicObject& object, ErrorReporting& errorReporting)
    {
        auto luaScript = dynamic_cast<LuaScript*>(&object);
        if (luaScript)
            return destroyInternal(*luaScript, errorReporting);

        auto luaModule = dynamic_cast<LuaModule*>(&object);
        if (luaModule)
            return destroyInternal(*luaModule, errorReporting);

        auto ramsesNodeBinding = dynamic_cast<RamsesNodeBinding*>(&object);
        if (ramsesNodeBinding)
            return destroyInternal(*ramsesNodeBinding, errorReporting);

        auto ramsesAppearanceBinding = dynamic_cast<RamsesAppearanceBinding*>(&object);
        if (ramsesAppearanceBinding)
            return destroyInternal(*ramsesAppearanceBinding, errorReporting);

        auto ramsesCameraBinding = dynamic_cast<RamsesCameraBinding*>(&object);
        if (ramsesCameraBinding)
            return destroyInternal(*ramsesCameraBinding, errorReporting);

        auto animNode = dynamic_cast<AnimationNode*>(&object);
        if (animNode)
            return destroyInternal(*animNode, errorReporting);

        auto dataArray = dynamic_cast<DataArray*>(&object);
        if (dataArray)
            return destroyInternal(*dataArray, errorReporting);

        errorReporting.add(fmt::format("Tried to destroy object '{}' with unknown type", object.getName()), &object);

        return false;
    }

    bool ApiObjects::destroyInternal(DataArray& dataArray, ErrorReporting& errorReporting)
    {
        const auto it = std::find_if(m_dataArrays.cbegin(), m_dataArrays.cend(), [&](const auto& d) {
            return d.get() == &dataArray;
        });
        if (it == m_dataArrays.cend())
        {
            errorReporting.add("Can't find data array in logic engine!", &dataArray);
            return false;
        }
        for (const auto& animNode : m_animationNodes)
        {
            for (const auto& channel : animNode->getChannels())
            {
                if (channel.timeStamps == &dataArray ||
                    channel.keyframes == &dataArray ||
                    channel.tangentsIn == &dataArray ||
                    channel.tangentsOut == &dataArray)
                {
                    errorReporting.add(fmt::format("Failed to destroy data array '{}', it is used in animation node '{}' channel '{}'", dataArray.getName(), animNode->getName(), channel.name), &dataArray);
                    return false;
                }
            }
        }

        m_dataArrays.erase(it);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaScript& luaScript, ErrorReporting& errorReporting)
    {
        auto scriptIter = find_if(m_scripts.begin(), m_scripts.end(), [&](const std::unique_ptr<LuaScript>& script) {
            return script.get() == &luaScript;
            });
        if (scriptIter == m_scripts.end())
        {
            errorReporting.add("Can't find script in logic engine!", &luaScript);
            return false;
        }

        auto implIter = m_reverseImplMapping.find(&luaScript.m_impl);
        assert(implIter != m_reverseImplMapping.end());
        m_reverseImplMapping.erase(implIter);

        m_logicNodeDependencies.removeNode(luaScript.m_impl);
        m_scripts.erase(scriptIter);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaModule& luaModule, ErrorReporting& errorReporting)
    {
        const auto it = std::find_if(m_luaModules.cbegin(), m_luaModules.cend(), [&](const auto& m) {
            return m.get() == &luaModule;
        });
        if (it == m_luaModules.cend())
        {
            errorReporting.add("Can't find Lua module in logic engine!", &luaModule);
            return false;
        }
        for (const auto& script : m_scripts)
        {
            for (const auto& moduleInUse : script->m_script.getModules())
            {
                if (moduleInUse.second == &luaModule)
                {
                    errorReporting.add(fmt::format("Failed to destroy LuaModule '{}', it is used in LuaScript '{}'", luaModule.getName(), script->getName()), &luaModule);
                    return false;
                }
            }
        }

        m_luaModules.erase(it);
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
            errorReporting.add("Can't find RamsesNodeBinding in logic engine!", &ramsesNodeBinding);
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
            errorReporting.add("Can't find RamsesAppearanceBinding in logic engine!", &ramsesAppearanceBinding);
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
            errorReporting.add("Can't find RamsesCameraBinding in logic engine!", &ramsesCameraBinding);
            return false;
        }

        unregisterLogicNode(ramsesCameraBinding);
        m_ramsesCameraBindings.erase(cameraIter);

        return true;
    }

    bool ApiObjects::destroyInternal(AnimationNode& node, ErrorReporting& errorReporting)
    {
        auto nodeIt = find_if(m_animationNodes.begin(), m_animationNodes.end(), [&](const auto& n) {
            return n.get() == &node;
        });

        if (nodeIt == m_animationNodes.end())
        {
            errorReporting.add("Can't find AnimationNode in logic engine!", &node);
            return false;
        }

        unregisterLogicNode(node);
        m_animationNodes.erase(nodeIt);

        return true;
    }

    bool ApiObjects::checkBindingsReferToSameRamsesScene(ErrorReporting& errorReporting) const
    {
        // Optional because it's OK that no Ramses object is referenced at all (and thus no ramses scene)
        std::optional<ramses::sceneId_t> sceneId;

        for (const auto& binding : m_ramsesNodeBindings)
        {
            const ramses::Node& node = binding->m_nodeBinding.getRamsesNode();
            const ramses::sceneId_t nodeSceneId = node.getSceneId();
            if (!sceneId)
            {
                sceneId = nodeSceneId;
            }

            if (*sceneId != nodeSceneId)
            {
                errorReporting.add(fmt::format("Ramses node '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    node.getName(), nodeSceneId.getValue(), sceneId->getValue()), binding.get());
                return false;
            }
        }

        for (const auto& binding : m_ramsesAppearanceBindings)
        {
            const ramses::Appearance& appearance = binding->m_appearanceBinding.getRamsesAppearance();
            const ramses::sceneId_t appearanceSceneId = appearance.getSceneId();
            if (!sceneId)
            {
                sceneId = appearanceSceneId;
            }

            if (*sceneId != appearanceSceneId)
            {
                errorReporting.add(fmt::format("Ramses appearance '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    appearance.getName(), appearanceSceneId.getValue(), sceneId->getValue()), binding.get());
                return false;
            }
        }

        for (const auto& binding : m_ramsesCameraBindings)
        {
            const ramses::Camera& camera = binding->m_cameraBinding.getRamsesCamera();
            const ramses::sceneId_t cameraSceneId = camera.getSceneId();
            if (!sceneId)
            {
                sceneId = cameraSceneId;
            }

            if (*sceneId != cameraSceneId)
            {
                errorReporting.add(fmt::format("Ramses camera '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    camera.getName(), cameraSceneId.getValue(), sceneId->getValue()), binding.get());
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

    LuaModulesContainer& ApiObjects::getLuaModules()
    {
        return m_luaModules;
    }

    const LuaModulesContainer& ApiObjects::getLuaModules() const
    {
        return m_luaModules;
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

    DataArrayContainer& ApiObjects::getDataArrays()
    {
        return m_dataArrays;
    }

    const DataArrayContainer& ApiObjects::getDataArrays() const
    {
        return m_dataArrays;
    }

    AnimationNodesContainer& ApiObjects::getAnimationNodes()
    {
        return m_animationNodes;
    }

    const AnimationNodesContainer& ApiObjects::getAnimationNodes() const
    {
        return m_animationNodes;
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

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>> luaModulesOffset = 0;
        if (!apiObjects.m_luaModules.empty())
        {
            std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>> luaModules;
            luaModules.reserve(apiObjects.m_luaModules.size());
            for (const auto& luaModule : apiObjects.m_luaModules)
            {
                luaModules.push_back(LuaModuleImpl::Serialize(luaModule->m_impl, builder, serializationMap));
                serializationMap.storeLuaModule(*luaModule, luaModules.back());
            }

            luaModulesOffset = builder.CreateVector(luaModules);
        }

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(apiObjects.m_scripts.size());
        std::transform(apiObjects.m_scripts.begin(), apiObjects.m_scripts.end(), std::back_inserter(luascripts),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) {
                return LuaScriptImpl::Serialize(it->m_script, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(apiObjects.m_ramsesNodeBindings.size());
        std::transform(apiObjects.m_ramsesNodeBindings.begin(),
            apiObjects.m_ramsesNodeBindings.end(),
            std::back_inserter(ramsesnodebindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) {
                return RamsesNodeBindingImpl::Serialize(it->m_nodeBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(apiObjects.m_ramsesAppearanceBindings.size());
        std::transform(apiObjects.m_ramsesAppearanceBindings.begin(),
            apiObjects.m_ramsesAppearanceBindings.end(),
            std::back_inserter(ramsesappearancebindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesAppearanceBinding>>::value_type& it) {
                return RamsesAppearanceBindingImpl::Serialize(it->m_appearanceBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> ramsescamerabindings;
        ramsescamerabindings.reserve(apiObjects.m_ramsesCameraBindings.size());
        std::transform(apiObjects.m_ramsesCameraBindings.begin(),
            apiObjects.m_ramsesCameraBindings.end(),
            std::back_inserter(ramsescamerabindings),
            [&builder, &serializationMap](const std::vector<std::unique_ptr<RamsesCameraBinding>>::value_type& it) {
                return RamsesCameraBindingImpl::Serialize(it->m_cameraBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>> dataArrays;
        dataArrays.reserve(apiObjects.m_dataArrays.size());
        for (const auto& da : apiObjects.m_dataArrays)
        {
            dataArrays.push_back(DataArrayImpl::Serialize(da->m_impl, builder));
            serializationMap.storeDataArray(*da, dataArrays.back());
        }

        // animation nodes must go after data arrays because they reference them
        std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>> animationNodes;
        animationNodes.reserve(apiObjects.m_animationNodes.size());
        for (const auto& animNode : apiObjects.m_animationNodes)
            animationNodes.push_back(AnimationNodeImpl::Serialize(animNode->m_animationNodeImpl, builder, serializationMap));

        // links must go last due to dependency on serialized properties
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
            builder.CreateVector(dataArrays),
            builder.CreateVector(animationNodes),
            builder.CreateVector(links),
            luaModulesOffset
        );

        builder.Finish(logicEngine);

        return logicEngine;
    }

    std::unique_ptr<ApiObjects> ApiObjects::Deserialize(
        const rlogic_serialization::ApiObjects& apiObjects,
        const IRamsesObjectResolver& ramsesResolver,
        const std::string& dataSourceDescription,
        ErrorReporting& errorReporting)
    {
        // Collect data here, only return if no error occurred
        auto deserialized = std::make_unique<ApiObjects>();

        // Collect deserialized object mappings to resolve dependencies
        DeserializationMap deserializationMap;

        if (!apiObjects.luaScripts())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing Lua scripts container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.nodeBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing node bindings container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.appearanceBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing appearance bindings container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.cameraBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing camera bindings container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.links())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing links container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.dataArrays())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing data arrays container!", nullptr);
            return nullptr;
        }

        if (!apiObjects.animationNodes())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing animation nodes container!", nullptr);
            return nullptr;
        }

        if (apiObjects.luaModules())
        {
            const auto& luaModules = *apiObjects.luaModules();
            deserialized->m_luaModules.reserve(luaModules.size());
            for (const auto* module : luaModules)
            {
                std::unique_ptr<LuaModuleImpl> deserializedModule = LuaModuleImpl::Deserialize(*deserialized->m_solState, *module, errorReporting, deserializationMap);
                if (!deserializedModule)
                    return nullptr;

                deserialized->m_luaModules.push_back(std::make_unique<LuaModule>(std::move(deserializedModule)));
                deserializationMap.storeLuaModule(*module, *deserialized->m_luaModules.back());
            }
        }

        const auto& luascripts = *apiObjects.luaScripts();
        deserialized->m_scripts.reserve(luascripts.size());
        for (const auto* script : luascripts)
        {
            // TODO Violin find ways to unit-test this case - also for other container types
            // Ideas: see if verifier catches it; or: disable flatbuffer's internal asserts if possible
            assert (script);
            std::unique_ptr<LuaScriptImpl> deserializedScript = LuaScriptImpl::Deserialize(*deserialized->m_solState, *script, errorReporting, deserializationMap);

            if (deserializedScript)
            {
                deserialized->m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(deserializedScript)));
                deserialized->registerLogicNode(*deserialized->m_scripts.back());
            }
            else
            {
                return nullptr;
            }
        }

        const auto& ramsesNodeBindings = *apiObjects.nodeBindings();
        deserialized->m_ramsesNodeBindings.reserve(ramsesNodeBindings.size());
        for (const auto* binding : ramsesNodeBindings)
        {
            assert (binding);
            std::unique_ptr<RamsesNodeBindingImpl> deserializedBinding = RamsesNodeBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized->m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::move(deserializedBinding)));
                deserialized->registerLogicNode(*deserialized->m_ramsesNodeBindings.back());
            }
            else
            {
                return nullptr;
            }
        }

        const auto& ramsesAppearanceBindings = *apiObjects.appearanceBindings();
        deserialized->m_ramsesAppearanceBindings.reserve(ramsesAppearanceBindings.size());
        for (const auto* binding : ramsesAppearanceBindings)
        {
            assert(binding);
            std::unique_ptr<RamsesAppearanceBindingImpl> deserializedBinding = RamsesAppearanceBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized->m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(std::move(deserializedBinding)));
                deserialized->registerLogicNode(*deserialized->m_ramsesAppearanceBindings.back());
            }
            else
            {
                return nullptr;
            }
        }

        const auto& ramsesCameraBindings = *apiObjects.cameraBindings();
        deserialized->m_ramsesCameraBindings.reserve(ramsesCameraBindings.size());
        for (const auto* binding : ramsesCameraBindings)
        {
            assert(binding);
            std::unique_ptr<RamsesCameraBindingImpl> deserializedBinding = RamsesCameraBindingImpl::Deserialize(*binding, ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                deserialized->m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(std::move(deserializedBinding)));
                deserialized->registerLogicNode(*deserialized->m_ramsesCameraBindings.back());
            }
            else
            {
                return nullptr;
            }
        }

        const auto& dataArrays = *apiObjects.dataArrays();
        deserialized->m_dataArrays.reserve(dataArrays.size());
        for (const auto* fbData : dataArrays)
        {
            assert(fbData);
            auto deserializedDataArray = DataArrayImpl::Deserialize(*fbData, errorReporting);
            if (!deserializedDataArray)
                return nullptr;

            deserialized->m_dataArrays.push_back(std::make_unique<DataArray>(std::move(deserializedDataArray)));
            deserializationMap.storeDataArray(*fbData, *deserialized->m_dataArrays.back());
        }

        // animation nodes must go after data arrays because they need to resolve references
        const auto& animNodes = *apiObjects.animationNodes();
        deserialized->m_animationNodes.reserve(animNodes.size());
        for (const auto* fbData : animNodes)
        {
            assert(fbData);
            auto deserializedAnimNode = AnimationNodeImpl::Deserialize(*fbData, errorReporting, deserializationMap);
            if (!deserializedAnimNode)
                return nullptr;

            deserialized->m_animationNodes.push_back(std::make_unique<AnimationNode>(std::move(deserializedAnimNode)));
            deserialized->registerLogicNode(*deserialized->m_animationNodes.back());
        }

        // links must go last due to dependency on deserialized properties
        const auto& links = *apiObjects.links();
        // TODO Violin move this code (serialization parts too) to LogicNodeDependencies
        for (const auto* rLink : links)
        {
            assert(rLink);

            if (!rLink->sourceProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link source property!", nullptr);
                return nullptr;
            }

            if (!rLink->targetProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link target property!", nullptr);
                return nullptr;
            }

            const rlogic_serialization::Property* sourceProp = rLink->sourceProperty();
            const rlogic_serialization::Property* targetProp = rLink->targetProperty();

            const bool success = deserialized->m_logicNodeDependencies.link(
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
                    ), nullptr);
                return nullptr;
            }
        }

        return deserialized;
    }

    bool ApiObjects::isDirty() const
    {
        // TODO Violin improve internal management of logic nodes so that we don't have to loop over three
        // different containers below which all call a method on LogicNode
        return std::any_of(m_scripts.cbegin(), m_scripts.cend(), [](const auto& s) { return s->m_impl.isDirty(); })
            || bindingsDirty();
    }

    bool ApiObjects::bindingsDirty() const
    {
        return
            std::any_of(m_ramsesNodeBindings.cbegin(), m_ramsesNodeBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); }) ||
            std::any_of(m_ramsesAppearanceBindings.cbegin(), m_ramsesAppearanceBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); }) ||
            std::any_of(m_ramsesCameraBindings.cbegin(), m_ramsesCameraBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); });
    }

    template DataArray* ApiObjects::createDataArray<float>(const std::vector<float>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec2f>(const std::vector<vec2f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec3f>(const std::vector<vec3f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec4f>(const std::vector<vec4f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<int32_t>(const std::vector<int32_t>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec2i>(const std::vector<vec2i>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec3i>(const std::vector<vec3i>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec4i>(const std::vector<vec4i>&, std::string_view);
}
