//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RamsesTestUtils.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesRenderPassBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/AnchorPoint.h"
#include "ramses-client-api/OrthographicCamera.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-utils.h"

namespace rlogic
{
    class ALogicEngineBase
    {
    public:
        explicit ALogicEngineBase(EFeatureLevel featureLevel = EFeatureLevel_01)
            : m_logicEngine{ featureLevel }
        {
            // make ramses camera valid, needed for anchor points
            m_camera->setFrustum(-1.f, 1.f, -1.f, 1.f, 0.1f, 1.f);
        }

        static LuaConfig CreateDeps(const std::vector<std::pair<std::string_view, const LuaModule*>>& dependencies)
        {
            LuaConfig config;
            for (const auto& [alias, module] : dependencies)
            {
                config.addDependency(alias, *module);
            }

            return config;
        }

        static LuaConfig WithStdModules(std::initializer_list<EStandardModule> modules)
        {
            LuaConfig config;
            for (auto m : modules)
            {
                config.addStandardModuleDependency(m);
            }
            return config;
        }


    protected:
        LogicEngine m_logicEngine;
        RamsesTestSetup m_ramses;
        ramses::Scene* m_scene = { m_ramses.createScene() };
        ramses::Node* m_node = { m_scene->createNode() };
        ramses::OrthographicCamera* m_camera = { m_scene->createOrthographicCamera() };
        ramses::Appearance* m_appearance = { &RamsesTestSetup::CreateTrivialTestAppearance(*m_scene) };
        ramses::RenderPass* m_renderPass = { m_scene->createRenderPass() };

        const std::string_view m_valid_empty_script = R"(
            function interface(IN,OUT)
            end
            function run(IN,OUT)
            end
        )";

        const std::string_view m_invalid_empty_script = R"(
        )";

        const std::string_view m_moduleSourceCode = R"(
            local mymath = {}
            function mymath.add(a,b)
                print(a+b)
            end
            return mymath
        )";

        const std::string_view m_interfaceSourceCode = R"(
            function interface(inout_params)
                inout_params.translation_x = Type:Int32()
            end
        )";

        void recreate()
        {
            const ramses::sceneId_t sceneId = m_scene->getSceneId();

            m_ramses.destroyScene(*m_scene);
            m_scene = m_ramses.createScene(sceneId);
            m_node = m_scene->createNode();
            m_camera = m_scene->createOrthographicCamera();
            m_appearance = &RamsesTestSetup::CreateTrivialTestAppearance(*m_scene);
            m_renderPass = m_scene->createRenderPass();
        }
    };

    class ALogicEngine : public ALogicEngineBase, public ::testing::Test
    {
    public:
        explicit ALogicEngine(EFeatureLevel featureLevel = EFeatureLevel_01)
            : ALogicEngineBase{ featureLevel }
        {
        }
    };
}
