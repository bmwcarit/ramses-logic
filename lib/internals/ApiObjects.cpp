//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/ApiObjects.h"
#include "internals/ErrorReporting.h"

#include "ramses-logic-build-config.h"

#include "ramses-logic/LogicObject.h"
#include "ramses-logic/LogicNode.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"

#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"
#include "impl/DataArrayImpl.h"
#include "impl/AnimationNodeImpl.h"
#include "impl/TimerNodeImpl.h"

#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"

#include "generated/ApiObjectsGen.h"
#include "generated/RamsesAppearanceBindingGen.h"
#include "generated/RamsesBindingGen.h"
#include "generated/RamsesCameraBindingGen.h"
#include "generated/RamsesNodeBindingGen.h"
#include "generated/LinkGen.h"
#include "generated/DataArrayGen.h"
#include "generated/AnimationNodeGen.h"
#include "generated/TimerNodeGen.h"

#include "fmt/format.h"
#include "TypeUtils.h"

namespace rlogic::internal
{
    ApiObjects::ApiObjects() = default;
    ApiObjects::~ApiObjects() noexcept = default;

    bool ApiObjects::checkLuaModules(const ModuleMapping& moduleMapping, ErrorReporting& errorReporting)
    {
        for (const auto& module : moduleMapping)
        {
            if (m_luaModules.cend() == std::find_if(m_luaModules.cbegin(), m_luaModules.cend(),
                [&module](const auto& m) { return m == module.second; }))
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

        std::unique_ptr<LuaScript> up     = std::make_unique<LuaScript>(std::make_unique<LuaScriptImpl>(std::move(*compiledScript), scriptName, getNextLogicObjectId()));
        LuaScript*                 script = up.get();
        m_scripts.push_back(script);
        registerLogicObject(std::move(up));
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

        std::unique_ptr<LuaModule> up        = std::make_unique<LuaModule>(std::make_unique<LuaModuleImpl>(std::move(*compiledModule), moduleName, getNextLogicObjectId()));
        LuaModule*                 luaModule = up.get();
        m_luaModules.push_back(luaModule);
        registerLogicObject(std::move(up));
        return luaModule;
    }

    RamsesNodeBinding* ApiObjects::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name)
    {
        std::unique_ptr<RamsesNodeBinding> up = std::make_unique<RamsesNodeBinding>(std::make_unique<RamsesNodeBindingImpl>(ramsesNode, rotationType, name, getNextLogicObjectId()));
        RamsesNodeBinding*                 binding = up.get();
        m_ramsesNodeBindings.push_back(binding);
        registerLogicObject(std::move(up));
        return binding;
    }

    RamsesAppearanceBinding* ApiObjects::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        std::unique_ptr<RamsesAppearanceBinding> up      = std::make_unique<RamsesAppearanceBinding>(std::make_unique<RamsesAppearanceBindingImpl>(ramsesAppearance, name, getNextLogicObjectId()));
        RamsesAppearanceBinding*                 binding = up.get();
        m_ramsesAppearanceBindings.push_back(binding);
        registerLogicObject(std::move(up));
        return binding;
    }

    RamsesCameraBinding* ApiObjects::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        std::unique_ptr<RamsesCameraBinding> up      = std::make_unique<RamsesCameraBinding>(std::make_unique<RamsesCameraBindingImpl>(ramsesCamera, name, getNextLogicObjectId()));
        RamsesCameraBinding*                 binding = up.get();
        m_ramsesCameraBindings.push_back(binding);
        registerLogicObject(std::move(up));
        return binding;
    }

    template <typename T>
    DataArray* ApiObjects::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        // make copy of users data and move into data array
        std::vector<T> dataCopy = data;
        auto                       impl      = std::make_unique<DataArrayImpl>(std::move(dataCopy), name, getNextLogicObjectId());
        std::unique_ptr<DataArray> up        = std::make_unique<DataArray>(std::move(impl));
        DataArray*                 dataArray = up.get();
        m_dataArrays.push_back(dataArray);
        registerLogicObject(std::move(up));
        return dataArray;
    }

    AnimationNode* ApiObjects::createAnimationNode(const AnimationChannels& channels, std::string_view name)
    {
        std::unique_ptr<AnimationNode> up        = std::make_unique<AnimationNode>(std::make_unique<AnimationNodeImpl>(channels, name, getNextLogicObjectId()));
        AnimationNode*                 animation = up.get();
        m_animationNodes.push_back(animation);
        registerLogicObject(std::move(up));
        return animation;
    }

    TimerNode* ApiObjects::createTimerNode(std::string_view name)
    {
        std::unique_ptr<TimerNode> up = std::make_unique<TimerNode>(std::make_unique<TimerNodeImpl>(name, getNextLogicObjectId()));
        TimerNode* timer = up.get();
        m_timerNodes.push_back(timer);
        registerLogicObject(std::move(up));
        return timer;
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

        auto timer = dynamic_cast<TimerNode*>(&object);
        if (timer)
            return destroyInternal(*timer, errorReporting);

        errorReporting.add(fmt::format("Tried to destroy object '{}' with unknown type", object.getName()), &object);

        return false;
    }

    bool ApiObjects::destroyInternal(DataArray& dataArray, ErrorReporting& errorReporting)
    {
        const auto it = std::find_if(m_dataArrays.cbegin(), m_dataArrays.cend(), [&](const auto& d) {
            return d == &dataArray;
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
        unregisterLogicObject(dataArray);
        m_dataArrays.erase(it);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaScript& luaScript, ErrorReporting& errorReporting)
    {
        auto scriptIter = find_if(m_scripts.begin(), m_scripts.end(), [&](const LuaScript* script) {
            return script == &luaScript;
            });
        if (scriptIter == m_scripts.end())
        {
            errorReporting.add("Can't find script in logic engine!", &luaScript);
            return false;
        }

        unregisterLogicObject(luaScript);
        m_scripts.erase(scriptIter);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaModule& luaModule, ErrorReporting& errorReporting)
    {
        const auto it = std::find_if(m_luaModules.cbegin(), m_luaModules.cend(), [&](const auto& m) {
            return m == &luaModule;
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

        unregisterLogicObject(luaModule);
        m_luaModules.erase(it);
        return true;
    }

    bool ApiObjects::destroyInternal(RamsesNodeBinding& ramsesNodeBinding, ErrorReporting& errorReporting)
    {
        auto nodeIter = find_if(m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), [&](const RamsesNodeBinding* nodeBinding)
            {
                return nodeBinding == &ramsesNodeBinding;
            });

        if (nodeIter == m_ramsesNodeBindings.end())
        {
            errorReporting.add("Can't find RamsesNodeBinding in logic engine!", &ramsesNodeBinding);
            return false;
        }

        unregisterLogicObject(ramsesNodeBinding);
        m_ramsesNodeBindings.erase(nodeIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding, ErrorReporting& errorReporting)
    {
        auto appearanceIter = find_if(m_ramsesAppearanceBindings.begin(), m_ramsesAppearanceBindings.end(), [&](const RamsesAppearanceBinding* appearanceBinding) {
            return appearanceBinding == &ramsesAppearanceBinding;
            });

        if (appearanceIter == m_ramsesAppearanceBindings.end())
        {
            errorReporting.add("Can't find RamsesAppearanceBinding in logic engine!", &ramsesAppearanceBinding);
            return false;
        }

        unregisterLogicObject(ramsesAppearanceBinding);
        m_ramsesAppearanceBindings.erase(appearanceIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesCameraBinding& ramsesCameraBinding, ErrorReporting& errorReporting)
    {
        auto cameraIter = find_if(m_ramsesCameraBindings.begin(), m_ramsesCameraBindings.end(), [&](const RamsesCameraBinding* cameraBinding) {
            return cameraBinding == &ramsesCameraBinding;
        });

        if (cameraIter == m_ramsesCameraBindings.end())
        {
            errorReporting.add("Can't find RamsesCameraBinding in logic engine!", &ramsesCameraBinding);
            return false;
        }

        unregisterLogicObject(ramsesCameraBinding);
        m_ramsesCameraBindings.erase(cameraIter);

        return true;
    }

    bool ApiObjects::destroyInternal(AnimationNode& node, ErrorReporting& errorReporting)
    {
        auto nodeIt = find_if(m_animationNodes.begin(), m_animationNodes.end(), [&](const auto& n) {
            return n == &node;
        });

        if (nodeIt == m_animationNodes.end())
        {
            errorReporting.add("Can't find AnimationNode in logic engine!", &node);
            return false;
        }

        unregisterLogicObject(node);
        m_animationNodes.erase(nodeIt);

        return true;
    }

    bool ApiObjects::destroyInternal(TimerNode& node, ErrorReporting& errorReporting)
    {
        auto nodeIt = find_if(m_timerNodes.begin(), m_timerNodes.end(), [&](const auto& n) {
            return n == &node;
        });

        if (nodeIt == m_timerNodes.end())
        {
            errorReporting.add("Can't find TimerNode in logic engine!", &node);
            return false;
        }

        unregisterLogicObject(node);
        m_timerNodes.erase(nodeIt);

        return true;
    }

    void ApiObjects::registerLogicObject(std::unique_ptr<LogicObject> obj)
    {
        m_logicObjects.push_back(obj.get());
        auto logicNode = dynamic_cast<LogicNode*>(obj.get());
        if (logicNode)
        {
            registerLogicNode(*logicNode);
        }
        m_logicObjectIdMapping.emplace(obj->getId(), obj.get());
        m_objectsOwningContainer.push_back(move(obj));
    }

    void ApiObjects::unregisterLogicObject(LogicObject& objToDelete)
    {
        const auto findOwnedObj = std::find_if(m_objectsOwningContainer.cbegin(), m_objectsOwningContainer.cend(), [&](const auto& obj) { return obj.get() == &objToDelete; });
        assert(findOwnedObj != m_objectsOwningContainer.cend() && "Can't find LogicObject in owned objects!");

        const auto findLogicNode = std::find_if(m_logicObjects.cbegin(), m_logicObjects.cend(), [&](const auto& obj) { return obj == &objToDelete; });
        assert(findLogicNode != m_logicObjects.cend() && "Can't find LogicObject in logic objects!");

        auto logicNode = dynamic_cast<LogicNode*>(&objToDelete);
        if (logicNode)
        {
            unregisterLogicNode(*logicNode);
        }
        m_logicObjectIdMapping.erase(objToDelete.getId());
        m_objectsOwningContainer.erase(findOwnedObj);
        m_logicObjects.erase(findLogicNode);
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
                    node.getName(), nodeSceneId.getValue(), sceneId->getValue()), binding);
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
                    appearance.getName(), appearanceSceneId.getValue(), sceneId->getValue()), binding);
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
                    camera.getName(), cameraSceneId.getValue(), sceneId->getValue()), binding);
                return false;
            }
        }

        return true;
    }

    template <typename T>
    const ApiObjectContainer<T>& ApiObjects::getApiObjectContainer() const
    {
        if constexpr (std::is_same_v<T, LogicObject>)
        {
            return m_logicObjects;
        }
        else if constexpr (std::is_same_v<T, LuaScript>)
        {
            return m_scripts;
        }
        else if constexpr (std::is_same_v<T, LuaModule>)
        {
            return m_luaModules;
        }
        else if constexpr (std::is_same_v<T, RamsesNodeBinding>)
        {
            return m_ramsesNodeBindings;
        }
        else if constexpr (std::is_same_v<T, RamsesAppearanceBinding>)
        {
            return m_ramsesAppearanceBindings;
        }
        else if constexpr (std::is_same_v<T, RamsesCameraBinding>)
        {
            return m_ramsesCameraBindings;
        }
        else if constexpr (std::is_same_v<T, DataArray>)
        {
            return m_dataArrays;
        }
        else if constexpr (std::is_same_v<T, AnimationNode>)
        {
            return m_animationNodes;
        }
        else if constexpr (std::is_same_v<T, TimerNode>)
        {
            return m_timerNodes;
        }
    }

    template <typename T>
    ApiObjectContainer<T>& ApiObjects::getApiObjectContainer()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast) non-const version of getApiObjectContainer cast to its const version to avoid duplicating code
        return const_cast<ApiObjectContainer<T>&>((const_cast<const ApiObjects&>(*this)).getApiObjectContainer<T>());
    }

    const ApiObjectOwningContainer& ApiObjects::getApiObjectOwningContainer() const
    {
        return m_objectsOwningContainer;
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

    LogicObject* ApiObjects::getApiObjectById(uint64_t id) const
    {
        auto apiObjectIter = m_logicObjectIdMapping.find(id);
        if (apiObjectIter != m_logicObjectIdMapping.end())
        {
            assert(apiObjectIter->second->getId() == id);
            return apiObjectIter->second;
        }
        return nullptr;
    }

    const std::unordered_map<LogicNodeImpl*, LogicNode*>& ApiObjects::getReverseImplMapping() const
    {
        return m_reverseImplMapping;
    }

    flatbuffers::Offset<rlogic_serialization::ApiObjects> ApiObjects::Serialize(const ApiObjects& apiObjects, flatbuffers::FlatBufferBuilder& builder)
    {
        SerializationMap serializationMap;

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>> luaModules;
        luaModules.reserve(apiObjects.m_luaModules.size());
        for (const auto& luaModule : apiObjects.m_luaModules)
        {
            luaModules.push_back(LuaModuleImpl::Serialize(luaModule->m_impl, builder, serializationMap));
            serializationMap.storeLuaModule(*luaModule, luaModules.back());
        }

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(apiObjects.m_scripts.size());
        std::transform(apiObjects.m_scripts.begin(), apiObjects.m_scripts.end(), std::back_inserter(luascripts),
            [&builder, &serializationMap](const std::vector<LuaScript*>::value_type& it) {
                return LuaScriptImpl::Serialize(it->m_script, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(apiObjects.m_ramsesNodeBindings.size());
        std::transform(apiObjects.m_ramsesNodeBindings.begin(),
            apiObjects.m_ramsesNodeBindings.end(),
            std::back_inserter(ramsesnodebindings),
            [&builder, &serializationMap](const std::vector<RamsesNodeBinding*>::value_type& it) {
                return RamsesNodeBindingImpl::Serialize(it->m_nodeBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>> ramsesappearancebindings;
        ramsesappearancebindings.reserve(apiObjects.m_ramsesAppearanceBindings.size());
        std::transform(apiObjects.m_ramsesAppearanceBindings.begin(),
            apiObjects.m_ramsesAppearanceBindings.end(),
            std::back_inserter(ramsesappearancebindings),
            [&builder, &serializationMap](const std::vector<RamsesAppearanceBinding*>::value_type& it) {
                return RamsesAppearanceBindingImpl::Serialize(it->m_appearanceBinding, builder, serializationMap);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>> ramsescamerabindings;
        ramsescamerabindings.reserve(apiObjects.m_ramsesCameraBindings.size());
        std::transform(apiObjects.m_ramsesCameraBindings.begin(),
            apiObjects.m_ramsesCameraBindings.end(),
            std::back_inserter(ramsescamerabindings),
            [&builder, &serializationMap](const std::vector<RamsesCameraBinding*>::value_type& it) {
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

        std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>> timerNodes;
        timerNodes.reserve(apiObjects.m_timerNodes.size());
        for (const auto& timerNode : apiObjects.m_timerNodes)
            timerNodes.push_back(TimerNodeImpl::Serialize(timerNode->m_timerNodeImpl, builder, serializationMap));

        // links must go last due to dependency on serialized properties
        std::vector<flatbuffers::Offset<rlogic_serialization::Link>> links;

        std::function<void(const Property& input)> serializeLinks = [&serializeLinks, &links, &serializationMap, &builder](const Property& input){
            const auto inputCount = input.getChildCount();
            for (size_t i = 0; i < inputCount; ++i)
            {
                const auto child = input.getChild(i);

                if (TypeUtils::CanHaveChildren(child->getType()))
                {
                    serializeLinks(*child);
                }
                else
                {
                    assert(TypeUtils::IsPrimitiveType(child->getType()));
                    const PropertyImpl* potentialLinkedOutput = child->m_impl->getLinkedIncomingProperty();
                    if (potentialLinkedOutput != nullptr)
                    {
                        links.emplace_back(rlogic_serialization::CreateLink(builder,
                            serializationMap.resolvePropertyOffset(*potentialLinkedOutput),
                            serializationMap.resolvePropertyOffset(*child->m_impl)));
                    }
                }
            }
        };

        for (const auto& item : apiObjects.m_reverseImplMapping)
        {
            serializeLinks(*item.first->getInputs());
        }

        const auto logicEngine = rlogic_serialization::CreateApiObjects(
            builder,
            builder.CreateVector(luaModules),
            builder.CreateVector(luascripts),
            builder.CreateVector(ramsesnodebindings),
            builder.CreateVector(ramsesappearancebindings),
            builder.CreateVector(ramsescamerabindings),
            builder.CreateVector(dataArrays),
            builder.CreateVector(animationNodes),
            builder.CreateVector(timerNodes),
            builder.CreateVector(links)
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

        if (!apiObjects.luaModules())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing Lua modules container!", nullptr);
            return nullptr;
        }

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

        if (!apiObjects.timerNodes())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing timer nodes container!", nullptr);
            return nullptr;
        }

        const size_t logicObjectsTotalSize =
            static_cast<size_t>(apiObjects.luaModules()->size()) +
            static_cast<size_t>(apiObjects.luaScripts()->size()) +
            static_cast<size_t>(apiObjects.nodeBindings()->size()) +
            static_cast<size_t>(apiObjects.appearanceBindings()->size()) +
            static_cast<size_t>(apiObjects.cameraBindings()->size()) +
            static_cast<size_t>(apiObjects.dataArrays()->size()) +
            static_cast<size_t>(apiObjects.animationNodes()->size()) +
            static_cast<size_t>(apiObjects.timerNodes()->size());

        deserialized->m_objectsOwningContainer.reserve(logicObjectsTotalSize);
        deserialized->m_logicObjects.reserve(logicObjectsTotalSize);

        const auto& luaModules = *apiObjects.luaModules();
        deserialized->m_luaModules.reserve(luaModules.size());
        for (const auto* module : luaModules)
        {
            std::unique_ptr<LuaModuleImpl> deserializedModule = LuaModuleImpl::Deserialize(*deserialized->m_solState, *module, errorReporting, deserializationMap);
            if (!deserializedModule)
                return nullptr;

            std::unique_ptr<LuaModule> up        = std::make_unique<LuaModule>(std::move(deserializedModule));
            LuaModule*                 luaModule = up.get();
            deserialized->m_luaModules.push_back(luaModule);
            deserialized->registerLogicObject(std::move(up));
            deserializationMap.storeLuaModule(*module, *deserialized->m_luaModules.back());
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
                std::unique_ptr<LuaScript> up             = std::make_unique<LuaScript>(std::move(deserializedScript));
                LuaScript*                 luascript = up.get();
                deserialized->m_scripts.push_back(luascript);
                deserialized->registerLogicObject(std::move(up));
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
                std::unique_ptr<RamsesNodeBinding> up      = std::make_unique<RamsesNodeBinding>(std::move(deserializedBinding));
                RamsesNodeBinding*                 nodeBinding = up.get();
                deserialized->m_ramsesNodeBindings.push_back(nodeBinding);
                deserialized->registerLogicObject(std::move(up));
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
                std::unique_ptr<RamsesAppearanceBinding> up      = std::make_unique<RamsesAppearanceBinding>(std::move(deserializedBinding));
                RamsesAppearanceBinding*                 appBinding = up.get();
                deserialized->m_ramsesAppearanceBindings.push_back(appBinding);
                deserialized->registerLogicObject(std::move(up));
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
                std::unique_ptr<RamsesCameraBinding> up      = std::make_unique<RamsesCameraBinding>(std::move(deserializedBinding));
                RamsesCameraBinding*                 camBinding = up.get();
                deserialized->m_ramsesCameraBindings.push_back(camBinding);
                deserialized->registerLogicObject(std::move(up));
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

            std::unique_ptr<DataArray> up        = std::make_unique<DataArray>(std::move(deserializedDataArray));
            DataArray*                 dataArray = up.get();
            deserialized->m_dataArrays.push_back(dataArray);
            deserialized->registerLogicObject(std::move(up));
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

            std::unique_ptr<AnimationNode> up        = std::make_unique<AnimationNode>(std::move(deserializedAnimNode));
            AnimationNode*                 animation = up.get();
            deserialized->m_animationNodes.push_back(animation);
            deserialized->registerLogicObject(std::move(up));
        }

        const auto& timerNodes = *apiObjects.timerNodes();
        deserialized->m_timerNodes.reserve(timerNodes.size());
        for (const auto* fbData : timerNodes)
        {
            assert(fbData);
            auto deserializedTimer = TimerNodeImpl::Deserialize(*fbData, errorReporting, deserializationMap);
            if (!deserializedTimer)
                return nullptr;

            auto up = std::make_unique<TimerNode>(std::move(deserializedTimer));
            deserialized->m_timerNodes.push_back(up.get());
            deserialized->registerLogicObject(std::move(up));
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

    uint64_t ApiObjects::getNextLogicObjectId()
    {
        return ++m_lastObjectId;
    }

    template DataArray* ApiObjects::createDataArray<float>(const std::vector<float>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec2f>(const std::vector<vec2f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec3f>(const std::vector<vec3f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec4f>(const std::vector<vec4f>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<int32_t>(const std::vector<int32_t>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec2i>(const std::vector<vec2i>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec3i>(const std::vector<vec3i>&, std::string_view);
    template DataArray* ApiObjects::createDataArray<vec4i>(const std::vector<vec4i>&, std::string_view);

    template ApiObjectContainer<LogicObject>&             ApiObjects::getApiObjectContainer<LogicObject>();
    template ApiObjectContainer<LuaScript>&               ApiObjects::getApiObjectContainer<LuaScript>();
    template ApiObjectContainer<LuaModule>&               ApiObjects::getApiObjectContainer<LuaModule>();
    template ApiObjectContainer<RamsesNodeBinding>&       ApiObjects::getApiObjectContainer<RamsesNodeBinding>();
    template ApiObjectContainer<RamsesAppearanceBinding>& ApiObjects::getApiObjectContainer<RamsesAppearanceBinding>();
    template ApiObjectContainer<RamsesCameraBinding>&     ApiObjects::getApiObjectContainer<RamsesCameraBinding>();
    template ApiObjectContainer<DataArray>&               ApiObjects::getApiObjectContainer<DataArray>();
    template ApiObjectContainer<AnimationNode>&           ApiObjects::getApiObjectContainer<AnimationNode>();
    template ApiObjectContainer<TimerNode>&               ApiObjects::getApiObjectContainer<TimerNode>();

    template const ApiObjectContainer<LogicObject>&             ApiObjects::getApiObjectContainer<LogicObject>() const;
    template const ApiObjectContainer<LuaScript>&               ApiObjects::getApiObjectContainer<LuaScript>() const;
    template const ApiObjectContainer<LuaModule>&               ApiObjects::getApiObjectContainer<LuaModule>() const;
    template const ApiObjectContainer<RamsesNodeBinding>&       ApiObjects::getApiObjectContainer<RamsesNodeBinding>() const;
    template const ApiObjectContainer<RamsesAppearanceBinding>& ApiObjects::getApiObjectContainer<RamsesAppearanceBinding>() const;
    template const ApiObjectContainer<RamsesCameraBinding>&     ApiObjects::getApiObjectContainer<RamsesCameraBinding>() const;
    template const ApiObjectContainer<DataArray>&               ApiObjects::getApiObjectContainer<DataArray>() const;
    template const ApiObjectContainer<AnimationNode>&           ApiObjects::getApiObjectContainer<AnimationNode>() const;
    template const ApiObjectContainer<TimerNode>&               ApiObjects::getApiObjectContainer<TimerNode>() const;
}
