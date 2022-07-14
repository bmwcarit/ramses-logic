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
#include "ramses-logic/RamsesRenderPassBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/AnchorPoint.h"

#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/LuaInterfaceImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"
#include "impl/RamsesRenderPassBindingImpl.h"
#include "impl/DataArrayImpl.h"
#include "impl/AnimationNodeImpl.h"
#include "impl/AnimationNodeConfigImpl.h"
#include "impl/TimerNodeImpl.h"
#include "impl/AnchorPointImpl.h"

#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"
#include "ramses-client-api/RenderPass.h"

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
#include "ValidationResults.h"

namespace rlogic::internal
{
    ApiObjects::ApiObjects(EFeatureLevel featureLevel)
        : m_featureLevel{ featureLevel }
    {
    }

    ApiObjects::~ApiObjects() noexcept = default;

    bool ApiObjects::checkLuaModules(const ModuleMapping& moduleMapping, ErrorReporting& errorReporting)
    {
        for (const auto& module : moduleMapping)
        {
            if (m_luaModules.cend() == std::find_if(m_luaModules.cbegin(), m_luaModules.cend(),
                [&module](const auto& m) { return m == module.second; }))
            {
                errorReporting.add(fmt::format("Failed to map Lua module '{}'! It was created on a different instance of LogicEngine.", module.first), module.second, EErrorType::IllegalArgument);
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

        std::optional<LuaCompiledScript> compiledScript = LuaCompilationUtils::CompileScriptOrImportPrecompiled(
            *m_solState,
            modules,
            config.getStandardModules(),
            std::string{ source },
            scriptName,
            errorReporting,
            {}, {}, {}, m_featureLevel);

        if (!compiledScript)
            return nullptr;

        std::unique_ptr<LuaScript> up = std::make_unique<LuaScript>(std::make_unique<LuaScriptImpl>(std::move(*compiledScript), scriptName, getNextLogicObjectId()));
        LuaScript* script = up.get();
        m_scripts.push_back(script);
        registerLogicObject(std::move(up));
        script->m_impl.createRootProperties();

        return script;
    }

    LuaInterface* ApiObjects::createLuaInterface(
        std::string_view source,
        std::string_view interfaceName,
        ErrorReporting& errorReporting)
    {
        if (interfaceName.empty())
        {
            errorReporting.add("Can't create interface with empty name!", nullptr, EErrorType::IllegalArgument);
            return nullptr;
        }

        std::optional<LuaCompiledInterface> compiledInterface = LuaCompilationUtils::CompileInterface(
            *m_solState,
            std::string{ source },
            interfaceName,
            errorReporting);

        if (!compiledInterface)
            return nullptr;

        std::unique_ptr<LuaInterface> up = std::make_unique<LuaInterface>(std::make_unique<LuaInterfaceImpl>(std::move(*compiledInterface), interfaceName, getNextLogicObjectId()));
        LuaInterface* intf = up.get();
        m_interfaces.push_back(intf);
        registerLogicObject(std::move(up));
        return intf;
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

        std::optional<LuaCompiledModule> compiledModule = LuaCompilationUtils::CompileModuleOrImportPrecompiled(
            *m_solState,
            modules,
            config.getStandardModules(),
            std::string{source},
            moduleName,
            errorReporting,
            {},
            m_featureLevel);

        if (!compiledModule)
            return nullptr;

        std::unique_ptr<LuaModule> up = std::make_unique<LuaModule>(std::make_unique<LuaModuleImpl>(std::move(*compiledModule), moduleName, getNextLogicObjectId()));
        LuaModule* luaModule = up.get();
        m_luaModules.push_back(luaModule);
        registerLogicObject(std::move(up));

        return luaModule;
    }

    RamsesNodeBinding* ApiObjects::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name)
    {
        std::unique_ptr<RamsesNodeBinding> up = std::make_unique<RamsesNodeBinding>(std::make_unique<RamsesNodeBindingImpl>(ramsesNode, rotationType, name, getNextLogicObjectId(), m_featureLevel));
        RamsesNodeBinding* binding = up.get();
        m_ramsesNodeBindings.push_back(binding);
        registerLogicObject(std::move(up));
        binding->m_impl.createRootProperties();

        return binding;
    }

    RamsesAppearanceBinding* ApiObjects::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        std::unique_ptr<RamsesAppearanceBinding> up = std::make_unique<RamsesAppearanceBinding>(std::make_unique<RamsesAppearanceBindingImpl>(ramsesAppearance, name, getNextLogicObjectId()));
        RamsesAppearanceBinding* binding = up.get();
        m_ramsesAppearanceBindings.push_back(binding);
        registerLogicObject(std::move(up));
        binding->m_impl.createRootProperties();

        return binding;
    }

    RamsesCameraBinding* ApiObjects::createRamsesCameraBinding(ramses::Camera& ramsesCamera, bool withFrustumPlanes, std::string_view name)
    {
        std::unique_ptr<RamsesCameraBinding> up = std::make_unique<RamsesCameraBinding>(std::make_unique<RamsesCameraBindingImpl>(ramsesCamera, withFrustumPlanes, name, getNextLogicObjectId()));
        RamsesCameraBinding* binding = up.get();
        m_ramsesCameraBindings.push_back(binding);
        registerLogicObject(std::move(up));
        binding->m_impl.createRootProperties();

        return binding;
    }

    RamsesRenderPassBinding* ApiObjects::createRamsesRenderPassBinding(ramses::RenderPass& ramsesRenderPass, std::string_view name)
    {
        assert(m_featureLevel >= EFeatureLevel_02);
        std::unique_ptr<RamsesRenderPassBinding> up = std::make_unique<RamsesRenderPassBinding>(std::make_unique<RamsesRenderPassBindingImpl>(ramsesRenderPass, name, getNextLogicObjectId()));
        RamsesRenderPassBinding* binding = up.get();
        m_ramsesRenderPassBindings.push_back(binding);
        registerLogicObject(std::move(up));
        binding->m_impl.createRootProperties();

        return binding;
    }

    template <typename T>
    DataArray* ApiObjects::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        // make copy of users data and move into data array
        std::vector<T> dataCopy = data;
        auto impl = std::make_unique<DataArrayImpl>(std::move(dataCopy), name, getNextLogicObjectId());
        std::unique_ptr<DataArray> up = std::make_unique<DataArray>(std::move(impl));
        DataArray* dataArray = up.get();
        m_dataArrays.push_back(dataArray);
        registerLogicObject(std::move(up));
        return dataArray;
    }

    AnimationNode* ApiObjects::createAnimationNode(const AnimationNodeConfigImpl& config, std::string_view name)
    {
        std::unique_ptr<AnimationNode> up = std::make_unique<AnimationNode>(
            std::make_unique<AnimationNodeImpl>(config.getChannels(), config.getExposingOfChannelDataAsProperties(), name, getNextLogicObjectId()));
        AnimationNode* animation = up.get();
        m_animationNodes.push_back(animation);
        registerLogicObject(std::move(up));
        animation->m_impl.createRootProperties();

        return animation;
    }

    TimerNode* ApiObjects::createTimerNode(std::string_view name)
    {
        std::unique_ptr<TimerNode> up = std::make_unique<TimerNode>(std::make_unique<TimerNodeImpl>(name, getNextLogicObjectId()));
        TimerNode* timer = up.get();
        m_timerNodes.push_back(timer);
        registerLogicObject(std::move(up));
        timer->m_impl.createRootProperties();

        return timer;
    }

    AnchorPoint* ApiObjects::createAnchorPoint(RamsesNodeBindingImpl& nodeBinding, RamsesCameraBindingImpl& cameraBinding, std::string_view name)
    {
        assert(m_featureLevel >= EFeatureLevel_02);
        std::unique_ptr<AnchorPoint> up = std::make_unique<AnchorPoint>(std::make_unique<AnchorPointImpl>(nodeBinding, cameraBinding, name, getNextLogicObjectId()));
        AnchorPoint* anchor = up.get();
        m_anchorPoints.push_back(anchor);
        registerLogicObject(std::move(up));
        anchor->m_impl.createRootProperties();

        m_logicNodeDependencies.addBindingDependency(nodeBinding, anchor->m_impl);
        m_logicNodeDependencies.addBindingDependency(cameraBinding, anchor->m_impl);

        return anchor;
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

        auto luaInterface = dynamic_cast<LuaInterface*>(&object);
        if (luaInterface)
            return destroyInternal(*luaInterface, errorReporting);

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

        auto ramsesRenderPassBinding = dynamic_cast<RamsesRenderPassBinding*>(&object);
        if (ramsesRenderPassBinding)
            return destroyInternal(*ramsesRenderPassBinding, errorReporting);

        auto animNode = dynamic_cast<AnimationNode*>(&object);
        if (animNode)
            return destroyInternal(*animNode, errorReporting);

        auto dataArray = dynamic_cast<DataArray*>(&object);
        if (dataArray)
            return destroyInternal(*dataArray, errorReporting);

        auto timer = dynamic_cast<TimerNode*>(&object);
        if (timer)
            return destroyInternal(*timer, errorReporting);

        auto anchor = dynamic_cast<AnchorPoint*>(&object);
        if (anchor)
            return destroyInternal(*anchor, errorReporting);

        errorReporting.add(fmt::format("Tried to destroy object '{}' with unknown type", object.getName()), &object, EErrorType::IllegalArgument);

        return false;
    }

    bool ApiObjects::destroyInternal(DataArray& dataArray, ErrorReporting& errorReporting)
    {
        const auto it = std::find(m_dataArrays.cbegin(), m_dataArrays.cend(), &dataArray);
        if (it == m_dataArrays.cend())
        {
            errorReporting.add("Can't find data array in logic engine!", &dataArray, EErrorType::IllegalArgument);
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
                    errorReporting.add(fmt::format("Failed to destroy data array '{}', it is used in animation node '{}' channel '{}'", dataArray.getName(), animNode->getName(), channel.name), &dataArray, EErrorType::IllegalArgument);
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
        auto scriptIter = std::find(m_scripts.begin(), m_scripts.end(), &luaScript);
        if (scriptIter == m_scripts.end())
        {
            errorReporting.add("Can't find script in logic engine!", &luaScript, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(luaScript);
        m_scripts.erase(scriptIter);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaInterface& luaInterface, ErrorReporting& errorReporting)
    {
        auto interfaceIter = std::find(m_interfaces.begin(), m_interfaces.end(), &luaInterface);
        if (interfaceIter == m_interfaces.end())
        {
            errorReporting.add("Can't find interface in logic engine!", &luaInterface, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(luaInterface);
        m_interfaces.erase(interfaceIter);
        return true;
    }

    bool ApiObjects::destroyInternal(LuaModule& luaModule, ErrorReporting& errorReporting)
    {
        const auto it = std::find(m_luaModules.cbegin(), m_luaModules.cend(), &luaModule);
        if (it == m_luaModules.cend())
        {
            errorReporting.add("Can't find Lua module in logic engine!", &luaModule, EErrorType::IllegalArgument);
            return false;
        }
        for (const auto& script : m_scripts)
        {
            for (const auto& moduleInUse : script->m_script.getModules())
            {
                if (moduleInUse.second == &luaModule)
                {
                    errorReporting.add(fmt::format("Failed to destroy LuaModule '{}', it is used in LuaScript '{}'", luaModule.getName(), script->getName()), &luaModule, EErrorType::IllegalArgument);
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
        auto nodeIter = std::find(m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), &ramsesNodeBinding);
        if (nodeIter == m_ramsesNodeBindings.end())
        {
            errorReporting.add("Can't find RamsesNodeBinding in logic engine!", &ramsesNodeBinding, EErrorType::IllegalArgument);
            return false;
        }

        for (const auto& anchor : m_anchorPoints)
        {
            if (anchor->m_anchorPointImpl.getRamsesNodeBinding().getId() == ramsesNodeBinding.getId())
            {
                errorReporting.add(fmt::format("Failed to destroy Ramses node binding '{}', it is used in anchor point '{}'", ramsesNodeBinding.getName(), anchor->getName()), &ramsesNodeBinding, EErrorType::Other);
                return false;
            }
        }

        unregisterLogicObject(ramsesNodeBinding);
        m_ramsesNodeBindings.erase(nodeIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding, ErrorReporting& errorReporting)
    {
        auto appearanceIter = std::find(m_ramsesAppearanceBindings.begin(), m_ramsesAppearanceBindings.end(), &ramsesAppearanceBinding);
        if (appearanceIter == m_ramsesAppearanceBindings.end())
        {
            errorReporting.add("Can't find RamsesAppearanceBinding in logic engine!", &ramsesAppearanceBinding, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(ramsesAppearanceBinding);
        m_ramsesAppearanceBindings.erase(appearanceIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesCameraBinding& ramsesCameraBinding, ErrorReporting& errorReporting)
    {
        auto cameraIter = std::find(m_ramsesCameraBindings.begin(), m_ramsesCameraBindings.end(), &ramsesCameraBinding);
        if (cameraIter == m_ramsesCameraBindings.end())
        {
            errorReporting.add("Can't find RamsesCameraBinding in logic engine!", &ramsesCameraBinding, EErrorType::IllegalArgument);
            return false;
        }

        for (const auto& anchor : m_anchorPoints)
        {
            if (anchor->m_anchorPointImpl.getRamsesCameraBinding().getId() == ramsesCameraBinding.getId())
            {
                errorReporting.add(fmt::format("Failed to destroy Ramses camera binding '{}', it is used in anchor point '{}'", ramsesCameraBinding.getName(), anchor->getName()), &ramsesCameraBinding, EErrorType::Other);
                return false;
            }
        }

        unregisterLogicObject(ramsesCameraBinding);
        m_ramsesCameraBindings.erase(cameraIter);

        return true;
    }

    bool ApiObjects::destroyInternal(RamsesRenderPassBinding& ramsesRenderPassBinding, ErrorReporting& errorReporting)
    {
        assert(m_featureLevel >= EFeatureLevel_02);
        auto bindingIter = std::find(m_ramsesRenderPassBindings.begin(), m_ramsesRenderPassBindings.end(), &ramsesRenderPassBinding);
        if (bindingIter == m_ramsesRenderPassBindings.end())
        {
            errorReporting.add("Can't find RamsesRenderPassBinding in logic engine!", &ramsesRenderPassBinding, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(ramsesRenderPassBinding);
        m_ramsesRenderPassBindings.erase(bindingIter);

        return true;
    }

    bool ApiObjects::destroyInternal(AnimationNode& node, ErrorReporting& errorReporting)
    {
        auto nodeIt = std::find(m_animationNodes.begin(), m_animationNodes.end(), &node);
        if (nodeIt == m_animationNodes.end())
        {
            errorReporting.add("Can't find AnimationNode in logic engine!", &node, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(node);
        m_animationNodes.erase(nodeIt);

        return true;
    }

    bool ApiObjects::destroyInternal(TimerNode& node, ErrorReporting& errorReporting)
    {
        auto nodeIt = std::find(m_timerNodes.begin(), m_timerNodes.end(), &node);
        if (nodeIt == m_timerNodes.end())
        {
            errorReporting.add("Can't find TimerNode in logic engine!", &node, EErrorType::IllegalArgument);
            return false;
        }

        unregisterLogicObject(node);
        m_timerNodes.erase(nodeIt);

        return true;
    }

    bool ApiObjects::destroyInternal(AnchorPoint& node, ErrorReporting& errorReporting)
    {
        assert(m_featureLevel >= EFeatureLevel_02);
        auto nodeIt = std::find(m_anchorPoints.begin(), m_anchorPoints.end(), &node);
        if (nodeIt == m_anchorPoints.end())
        {
            errorReporting.add("Can't find AnchorPoint in logic engine!", &node, EErrorType::IllegalArgument);
            return false;
        }

        m_logicNodeDependencies.removeBindingDependency(node.m_anchorPointImpl.getRamsesNodeBinding(), node.m_impl);
        m_logicNodeDependencies.removeBindingDependency(node.m_anchorPointImpl.getRamsesCameraBinding(), node.m_impl);

        unregisterLogicObject(node);
        m_anchorPoints.erase(nodeIt);

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

        const auto findLogicNode = std::find(m_logicObjects.cbegin(), m_logicObjects.cend(), &objToDelete);
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
                    node.getName(), nodeSceneId.getValue(), sceneId->getValue()), binding, EErrorType::IllegalArgument);
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
                    appearance.getName(), appearanceSceneId.getValue(), sceneId->getValue()), binding, EErrorType::IllegalArgument);
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
                    camera.getName(), cameraSceneId.getValue(), sceneId->getValue()), binding, EErrorType::IllegalArgument);
                return false;
            }
        }

        for (const auto& binding : m_ramsesRenderPassBindings)
        {
            const ramses::RenderPass& rp = binding->m_renderPassBinding.getRamsesRenderPass();
            const ramses::sceneId_t rpSceneId = rp.getSceneId();
            if (!sceneId)
                sceneId = rpSceneId;

            if (*sceneId != rpSceneId)
            {
                errorReporting.add(fmt::format("Ramses render pass '{}' is from scene with id:{} but other objects are from scene with id:{}!",
                    rp.getName(), rpSceneId.getValue(), sceneId->getValue()), binding, EErrorType::IllegalArgument);
                return false;
            }
        }

        return true;
    }

    void ApiObjects::validateInterfaces(ValidationResults& validationResults) const
    {
        // check if there are any outputs without link
        for (const auto* intf : m_interfaces)
        {
            std::vector<const Property*> unlinkedProperties = intf->m_interface.collectUnlinkedProperties();
            for (const auto* output : unlinkedProperties)
                validationResults.add(::fmt::format("Interface [{}] has unlinked output [{}]", intf->getName(), output->getName()), intf, EWarningType::UnusedContent);
        }

        // check if there are any name conflicts
        auto interfacesByName = m_interfaces;
        std::sort(interfacesByName.begin(), interfacesByName.end(), [](const auto i1, const auto i2) { return i1->getName() < i2->getName(); });
        const auto duplicateIt = std::adjacent_find(interfacesByName.cbegin(), interfacesByName.cend(), [](const auto i1, const auto i2) { return i1->getName() == i2->getName(); });
        if (duplicateIt != interfacesByName.cend())
            validationResults.add(fmt::format("Interface [{}] does not have a unique name", (*duplicateIt)->getName()), *duplicateIt, EWarningType::Other);
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
        else if constexpr (std::is_same_v<T, LuaInterface>)
        {
            return m_interfaces;
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
        else if constexpr (std::is_same_v<T, RamsesRenderPassBinding>)
        {
            return m_ramsesRenderPassBindings;
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
        else if constexpr (std::is_same_v<T, AnchorPoint>)
        {
            return m_anchorPoints;
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

    flatbuffers::Offset<rlogic_serialization::ApiObjects> ApiObjects::Serialize(const ApiObjects& apiObjects, flatbuffers::FlatBufferBuilder& builder, EFeatureLevel featureLevel)
    {
        SerializationMap serializationMap;

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>> luaModules;
        luaModules.reserve(apiObjects.m_luaModules.size());
        for (const auto& luaModule : apiObjects.m_luaModules)
            luaModules.push_back(LuaModuleImpl::Serialize(luaModule->m_impl, builder, apiObjects.m_featureLevel, serializationMap));

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>> luascripts;
        luascripts.reserve(apiObjects.m_scripts.size());
        std::transform(apiObjects.m_scripts.begin(), apiObjects.m_scripts.end(), std::back_inserter(luascripts),
            [&builder, &serializationMap, &apiObjects](const std::vector<LuaScript*>::value_type& it) {
                return LuaScriptImpl::Serialize(it->m_script, builder, serializationMap, apiObjects.m_featureLevel);
            });

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>> luaInterfaces;
        luascripts.reserve(apiObjects.m_interfaces.size());
        std::transform(apiObjects.m_interfaces.cbegin(), apiObjects.m_interfaces.cend(), std::back_inserter(luaInterfaces),
            [&builder, &serializationMap](const std::vector<LuaInterface*>::value_type& it) {
                return LuaInterfaceImpl::Serialize(it->m_interface, builder, serializationMap);
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

        std::vector<flatbuffers::Offset<rlogic_serialization::RamsesRenderPassBinding>> ramsesrenderpassbindings;
        ramsesrenderpassbindings.reserve(apiObjects.m_ramsesRenderPassBindings.size());
        std::transform(apiObjects.m_ramsesRenderPassBindings.begin(),
            apiObjects.m_ramsesRenderPassBindings.end(),
            std::back_inserter(ramsesrenderpassbindings),
            [&builder, &serializationMap](const std::vector<RamsesRenderPassBinding*>::value_type& it) {
                return RamsesRenderPassBindingImpl::Serialize(it->m_renderPassBinding, builder, serializationMap);
            });
        assert(featureLevel >= EFeatureLevel_02 || ramsesrenderpassbindings.empty());
        (void)featureLevel;

        std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>> dataArrays;
        dataArrays.reserve(apiObjects.m_dataArrays.size());
        for (const auto& da : apiObjects.m_dataArrays)
        {
            dataArrays.push_back(DataArrayImpl::Serialize(da->m_impl, builder));
            serializationMap.storeDataArray(da->getId(), dataArrays.back());
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

        // anchor points must go after bindings because they reference them
        std::vector<flatbuffers::Offset<rlogic_serialization::AnchorPoint>> anchorPoints;
        anchorPoints.reserve(apiObjects.m_anchorPoints.size());
        for (const auto& anchorPoint : apiObjects.m_anchorPoints)
            anchorPoints.push_back(AnchorPointImpl::Serialize(anchorPoint->m_anchorPointImpl, builder, serializationMap));
        assert(featureLevel >= EFeatureLevel_02 || anchorPoints.empty());

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
                    const auto& incomingLink = child->m_impl->getIncomingLink();
                    if (incomingLink.property != nullptr)
                    {
                        links.emplace_back(rlogic_serialization::CreateLink(builder,
                            serializationMap.resolvePropertyOffset(*incomingLink.property),
                            serializationMap.resolvePropertyOffset(*child->m_impl),
                            incomingLink.isWeakLink));
                    }
                }
            }
        };

        for (const auto& item : apiObjects.m_reverseImplMapping)
        {
            if (item.first->getInputs())
                serializeLinks(*item.first->getInputs());
        }

        const auto fbModules = builder.CreateVector(luaModules);
        const auto fbScripts = builder.CreateVector(luascripts);
        const auto fbInterfaces = builder.CreateVector(luaInterfaces);
        const auto fbNodeBindings = builder.CreateVector(ramsesnodebindings);
        const auto fbAppearanceBindings = builder.CreateVector(ramsesappearancebindings);
        const auto fbCameraBindings = builder.CreateVector(ramsescamerabindings);
        const auto fbDataArrays = builder.CreateVector(dataArrays);
        const auto fbAnimations = builder.CreateVector(animationNodes);
        const auto fbTimers = builder.CreateVector(timerNodes);
        const auto fbLinks = builder.CreateVector(links);
        const auto fbRenderPasses = builder.CreateVector(ramsesrenderpassbindings);
        const auto fbAnchorPoints = builder.CreateVector(anchorPoints);

        const auto logicEngine = rlogic_serialization::CreateApiObjects(
            builder,
            fbModules,
            fbScripts,
            fbInterfaces,
            fbNodeBindings,
            fbAppearanceBindings,
            fbCameraBindings,
            fbDataArrays,
            fbAnimations,
            fbTimers,
            fbLinks,
            apiObjects.m_lastObjectId,
            fbRenderPasses,
            fbAnchorPoints
            );

        builder.Finish(logicEngine);

        return logicEngine;
    }

    std::unique_ptr<ApiObjects> ApiObjects::Deserialize(
        const rlogic_serialization::ApiObjects& apiObjects,
        const IRamsesObjectResolver* ramsesResolver,
        const std::string& dataSourceDescription,
        ErrorReporting& errorReporting,
        EFeatureLevel featureLevel)
    {
        // Collect data here, only return if no error occurred
        auto deserialized = std::make_unique<ApiObjects>(featureLevel);

        // Collect deserialized object mappings to resolve dependencies
        DeserializationMap deserializationMap;

        if (!apiObjects.luaModules())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing Lua modules container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.luaScripts())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing Lua scripts container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.luaInterfaces())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing Lua interfaces container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.nodeBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing node bindings container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.appearanceBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing appearance bindings container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.cameraBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing camera bindings container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (featureLevel >= EFeatureLevel_02 && !apiObjects.renderPassBindings())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing renderpass bindings container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.links())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing links container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.dataArrays())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing data arrays container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.animationNodes())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing animation nodes container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!apiObjects.timerNodes())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing timer nodes container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (featureLevel >= EFeatureLevel_02 && !apiObjects.anchorPoints())
        {
            errorReporting.add("Fatal error during loading from serialized data: missing anchor points container!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        deserialized->m_lastObjectId = apiObjects.lastObjectId();

        const size_t logicObjectsTotalSize =
            static_cast<size_t>(apiObjects.luaModules()->size()) +
            static_cast<size_t>(apiObjects.luaScripts()->size()) +
            static_cast<size_t>(apiObjects.luaInterfaces()->size()) +
            static_cast<size_t>(apiObjects.nodeBindings()->size()) +
            static_cast<size_t>(apiObjects.appearanceBindings()->size()) +
            static_cast<size_t>(apiObjects.cameraBindings()->size()) +
            (featureLevel >= EFeatureLevel_02 ? static_cast<size_t>(apiObjects.renderPassBindings()->size()) : 0u) +
            static_cast<size_t>(apiObjects.dataArrays()->size()) +
            static_cast<size_t>(apiObjects.animationNodes()->size()) +
            static_cast<size_t>(apiObjects.timerNodes()->size()) +
            (featureLevel >= EFeatureLevel_02 ? static_cast<size_t>(apiObjects.anchorPoints()->size()) : 0u);

        deserialized->m_objectsOwningContainer.reserve(logicObjectsTotalSize);
        deserialized->m_logicObjects.reserve(logicObjectsTotalSize);

        const auto& luaModules = *apiObjects.luaModules();
        deserialized->m_luaModules.reserve(luaModules.size());
        for (const auto* module : luaModules)
        {
            std::unique_ptr<LuaModuleImpl> deserializedModule = LuaModuleImpl::Deserialize(*deserialized->m_solState, *module, errorReporting, deserializationMap, featureLevel);
            if (!deserializedModule)
                return nullptr;

            std::unique_ptr<LuaModule> up        = std::make_unique<LuaModule>(std::move(deserializedModule));
            LuaModule*                 luaModule = up.get();
            deserialized->m_luaModules.push_back(luaModule);
            deserialized->registerLogicObject(std::move(up));
            deserializationMap.storeLuaModule(luaModule->getId(), *deserialized->m_luaModules.back());
        }

        const auto& luascripts = *apiObjects.luaScripts();
        deserialized->m_scripts.reserve(luascripts.size());
        for (const auto* script : luascripts)
        {
            // TODO Violin find ways to unit-test this case - also for other container types
            // Ideas: see if verifier catches it; or: disable flatbuffer's internal asserts if possible
            assert (script);
            std::unique_ptr<LuaScriptImpl> deserializedScript = LuaScriptImpl::Deserialize(*deserialized->m_solState, *script, errorReporting, deserializationMap, featureLevel);

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

        const auto& luaInterfaces = *apiObjects.luaInterfaces();
        deserialized->m_interfaces.reserve(luaInterfaces.size());
        for (const auto* intf : luaInterfaces)
        {
            assert(intf);
            std::unique_ptr<LuaInterfaceImpl> deserializedInterface = LuaInterfaceImpl::Deserialize(*intf, errorReporting, deserializationMap);

            if (deserializedInterface)
            {
                std::unique_ptr<LuaInterface> up = std::make_unique<LuaInterface>(std::move(deserializedInterface));
                LuaInterface* luaInterface = up.get();
                deserialized->m_interfaces.push_back(luaInterface);
                deserialized->registerLogicObject(std::move(up));
            }
            else
            {
                return nullptr;
            }
        }

        if (apiObjects.nodeBindings()->size() != 0u ||
            apiObjects.appearanceBindings()->size() != 0u ||
            apiObjects.cameraBindings()->size() != 0u ||
            (featureLevel >= EFeatureLevel_02 && apiObjects.renderPassBindings()->size() != 0u))
        {
            if (ramsesResolver == nullptr)
            {
                errorReporting.add("Fatal error during loading from file! File contains references to Ramses objects but no Ramses scene was provided!", nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }
        }

        const auto& ramsesNodeBindings = *apiObjects.nodeBindings();
        deserialized->m_ramsesNodeBindings.reserve(ramsesNodeBindings.size());
        for (const auto* binding : ramsesNodeBindings)
        {
            assert(binding);
            assert(ramsesResolver);
            std::unique_ptr<RamsesNodeBindingImpl> deserializedBinding = RamsesNodeBindingImpl::Deserialize(*binding, *ramsesResolver, errorReporting, deserializationMap, featureLevel);

            if (deserializedBinding)
            {
                std::unique_ptr<RamsesNodeBinding> up = std::make_unique<RamsesNodeBinding>(std::move(deserializedBinding));
                RamsesNodeBinding* nodeBinding = up.get();
                deserialized->m_ramsesNodeBindings.push_back(nodeBinding);
                deserialized->registerLogicObject(std::move(up));
                deserializationMap.storeNodeBinding(nodeBinding->getId(), nodeBinding->m_nodeBinding);
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
            assert(ramsesResolver);
            std::unique_ptr<RamsesAppearanceBindingImpl> deserializedBinding = RamsesAppearanceBindingImpl::Deserialize(*binding, *ramsesResolver, errorReporting, deserializationMap);

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
            assert(ramsesResolver);
            std::unique_ptr<RamsesCameraBindingImpl> deserializedBinding = RamsesCameraBindingImpl::Deserialize(*binding, *ramsesResolver, errorReporting, deserializationMap);

            if (deserializedBinding)
            {
                std::unique_ptr<RamsesCameraBinding> up      = std::make_unique<RamsesCameraBinding>(std::move(deserializedBinding));
                RamsesCameraBinding*                 camBinding = up.get();
                deserialized->m_ramsesCameraBindings.push_back(camBinding);
                deserialized->registerLogicObject(std::move(up));
                deserializationMap.storeCameraBinding(camBinding->getId(), camBinding->m_cameraBinding);
            }
            else
            {
                return nullptr;
            }
        }

        if (featureLevel >= EFeatureLevel_02)
        {
            const auto& ramsesRenderPassBindings = *apiObjects.renderPassBindings();
            deserialized->m_ramsesRenderPassBindings.reserve(ramsesRenderPassBindings.size());
            for (const auto* binding : ramsesRenderPassBindings)
            {
                assert(binding);
                assert(ramsesResolver);
                std::unique_ptr<RamsesRenderPassBindingImpl> deserializedBinding = RamsesRenderPassBindingImpl::Deserialize(*binding, *ramsesResolver, errorReporting, deserializationMap);

                if (deserializedBinding)
                {
                    std::unique_ptr<RamsesRenderPassBinding> up = std::make_unique<RamsesRenderPassBinding>(std::move(deserializedBinding));
                    RamsesRenderPassBinding* rpBinding = up.get();
                    deserialized->m_ramsesRenderPassBindings.push_back(rpBinding);
                    deserialized->registerLogicObject(std::move(up));
                }
                else
                {
                    return nullptr;
                }
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

        // anchor points must go after bindings because they need to resolve references
        if (featureLevel >= EFeatureLevel_02)
        {
            const auto& anchorPoints = *apiObjects.anchorPoints();
            deserialized->m_anchorPoints.reserve(anchorPoints.size());
            for (const auto* fbAnchor : anchorPoints)
            {
                assert(fbAnchor);
                assert(ramsesResolver);
                std::unique_ptr<AnchorPointImpl> deserializedAnchor = AnchorPointImpl::Deserialize(*fbAnchor, errorReporting, deserializationMap);

                if (deserializedAnchor)
                {
                    auto up = std::make_unique<AnchorPoint>(std::move(deserializedAnchor));
                    AnchorPoint* anchor = up.get();
                    deserialized->m_anchorPoints.push_back(anchor);
                    deserialized->registerLogicObject(std::move(up));
                }
                else
                {
                    return nullptr;
                }
            }
        }

        // links must go last due to dependency on deserialized properties
        const auto& links = *apiObjects.links();
        // TODO Violin move this code (serialization parts too) to LogicNodeDependencies
        for (const auto* rLink : links)
        {
            assert(rLink);

            if (!rLink->sourceProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link source property!", nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }

            if (!rLink->targetProperty())
            {
                errorReporting.add("Fatal error during loading from serialized data: missing link target property!", nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }

            const rlogic_serialization::Property* sourceProp = rLink->sourceProperty();
            const rlogic_serialization::Property* targetProp = rLink->targetProperty();

            const bool success = deserialized->m_logicNodeDependencies.link(
                deserializationMap.resolvePropertyImpl(*sourceProp),
                deserializationMap.resolvePropertyImpl(*targetProp),
                rLink->isWeak(),
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
                    ), nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }
        }

        return deserialized;
    }

    bool ApiObjects::bindingsDirty() const
    {
        return
            std::any_of(m_ramsesNodeBindings.cbegin(), m_ramsesNodeBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); }) ||
            std::any_of(m_ramsesAppearanceBindings.cbegin(), m_ramsesAppearanceBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); }) ||
            std::any_of(m_ramsesCameraBindings.cbegin(), m_ramsesCameraBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); }) ||
            std::any_of(m_ramsesRenderPassBindings.cbegin(), m_ramsesRenderPassBindings.cend(), [](const auto& b) { return b->m_impl.isDirty(); });
    }

    uint64_t ApiObjects::getNextLogicObjectId()
    {
        return ++m_lastObjectId;
    }

    int ApiObjects::getNumElementsInLuaStack() const
    {
        return m_solState->getNumElementsInLuaStack();
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
    template ApiObjectContainer<LuaInterface>&            ApiObjects::getApiObjectContainer<LuaInterface>();
    template ApiObjectContainer<LuaModule>&               ApiObjects::getApiObjectContainer<LuaModule>();
    template ApiObjectContainer<RamsesNodeBinding>&       ApiObjects::getApiObjectContainer<RamsesNodeBinding>();
    template ApiObjectContainer<RamsesAppearanceBinding>& ApiObjects::getApiObjectContainer<RamsesAppearanceBinding>();
    template ApiObjectContainer<RamsesCameraBinding>&     ApiObjects::getApiObjectContainer<RamsesCameraBinding>();
    template ApiObjectContainer<RamsesRenderPassBinding>& ApiObjects::getApiObjectContainer<RamsesRenderPassBinding>();
    template ApiObjectContainer<DataArray>&               ApiObjects::getApiObjectContainer<DataArray>();
    template ApiObjectContainer<AnimationNode>&           ApiObjects::getApiObjectContainer<AnimationNode>();
    template ApiObjectContainer<TimerNode>&               ApiObjects::getApiObjectContainer<TimerNode>();
    template ApiObjectContainer<AnchorPoint>&             ApiObjects::getApiObjectContainer<AnchorPoint>();

    template const ApiObjectContainer<LogicObject>&             ApiObjects::getApiObjectContainer<LogicObject>() const;
    template const ApiObjectContainer<LuaScript>&               ApiObjects::getApiObjectContainer<LuaScript>() const;
    template const ApiObjectContainer<LuaInterface>&            ApiObjects::getApiObjectContainer<LuaInterface>() const;
    template const ApiObjectContainer<LuaModule>&               ApiObjects::getApiObjectContainer<LuaModule>() const;
    template const ApiObjectContainer<RamsesNodeBinding>&       ApiObjects::getApiObjectContainer<RamsesNodeBinding>() const;
    template const ApiObjectContainer<RamsesAppearanceBinding>& ApiObjects::getApiObjectContainer<RamsesAppearanceBinding>() const;
    template const ApiObjectContainer<RamsesCameraBinding>&     ApiObjects::getApiObjectContainer<RamsesCameraBinding>() const;
    template const ApiObjectContainer<RamsesRenderPassBinding>& ApiObjects::getApiObjectContainer<RamsesRenderPassBinding>() const;
    template const ApiObjectContainer<DataArray>&               ApiObjects::getApiObjectContainer<DataArray>() const;
    template const ApiObjectContainer<AnimationNode>&           ApiObjects::getApiObjectContainer<AnimationNode>() const;
    template const ApiObjectContainer<TimerNode>&               ApiObjects::getApiObjectContainer<TimerNode>() const;
    template const ApiObjectContainer<AnchorPoint>&             ApiObjects::getApiObjectContainer<AnchorPoint>() const;
}
