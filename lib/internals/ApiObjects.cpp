//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/ApiObjects.h"
#include "internals/ErrorReporting.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "impl/LuaScriptImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-client-api/Scene.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"

#include "fmt/format.h"

namespace rlogic::internal
{
    ApiObjects::ApiObjects(
        ScriptsContainer scripts,
        NodeBindingsContainer ramsesNodeBindings,
        AppearanceBindingsContainer ramsesAppearanceBindings,
        CameraBindingsContainer ramsesCameraBindings,
        LogicNodeDependencies logicNodeDependencies)
        : m_scripts(std::move(scripts))
        , m_ramsesNodeBindings(std::move(ramsesNodeBindings))
        , m_ramsesAppearanceBindings(std::move(ramsesAppearanceBindings))
        , m_ramsesCameraBindings(std::move(ramsesCameraBindings))
        , m_logicNodeDependencies(std::move(logicNodeDependencies))
    {
        for (const auto& apiObject : m_scripts)
        {
            m_reverseImplMapping.emplace(std::make_pair(&apiObject->m_impl.get(), apiObject.get()));
        }
        for (const auto& apiObject : m_ramsesNodeBindings)
        {
            m_reverseImplMapping.emplace(std::make_pair(&apiObject->m_impl.get(), apiObject.get()));
        }
        for (const auto& apiObject : m_ramsesAppearanceBindings)
        {
            m_reverseImplMapping.emplace(std::make_pair(&apiObject->m_impl.get(), apiObject.get()));
        }
        for (const auto& apiObject : m_ramsesCameraBindings)
        {
            m_reverseImplMapping.emplace(std::make_pair(&apiObject->m_impl.get(), apiObject.get()));
        }
    }

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
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Create(name)));
        RamsesNodeBinding* binding = m_ramsesNodeBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesAppearanceBinding* ApiObjects::createRamsesAppearanceBinding(std::string_view name)
    {
        m_ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(RamsesAppearanceBindingImpl::Create(name)));
        RamsesAppearanceBinding* binding = m_ramsesAppearanceBindings.back().get();
        registerLogicNode(*binding);
        return binding;
    }

    RamsesCameraBinding* ApiObjects::createRamsesCameraBinding(std::string_view name)
    {
        m_ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(RamsesCameraBindingImpl::Create(name)));
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

}
