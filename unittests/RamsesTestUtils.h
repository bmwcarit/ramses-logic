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

#include <memory>
#include <array>

#include "ramses-client-api/RamsesClient.h"
#include "ramses-client-api/Scene.h"
#include "ramses-framework-api/RamsesFramework.h"
#include "ramses-client-api/EffectDescription.h"

namespace rlogic
{
    class RamsesTestSetup
    {
    public:
        RamsesTestSetup()
        {
            std::array<const char*, 3> commandLineConfigForTest = { "test", "-l", "off" };
            ramses::RamsesFrameworkConfig frameworkConfig(static_cast<uint32_t>(commandLineConfigForTest.size()), commandLineConfigForTest.data());
            m_ramsesFramework = std::make_unique<ramses::RamsesFramework>(frameworkConfig);
            m_ramsesClient = m_ramsesFramework->createClient("test client");
        }

        ramses::Scene* createScene(ramses::sceneId_t sceneId = ramses::sceneId_t(1))
        {
            return m_ramsesClient->createScene(sceneId);
        }

        void destroyScene(ramses::Scene& scene)
        {
            m_ramsesClient->destroy(scene);
        }

        static ramses::Appearance& CreateTestAppearance(ramses::Scene& scene, std::string_view vertShader, std::string_view fragShader)
        {
            ramses::EffectDescription effectDesc;
            effectDesc.setUniformSemantic("u_DisplayBufferResolution", ramses::EEffectUniformSemantic::DisplayBufferResolution);
            effectDesc.setVertexShader(vertShader.data());
            effectDesc.setFragmentShader(fragShader.data());
            return *scene.createAppearance(*scene.createEffect(effectDesc), "test appearance");
        }

        static ramses::Appearance& CreateTrivialTestAppearance(ramses::Scene& scene)
        {
            std::string_view vertShader = R"(
                #version 100

                uniform highp float floatUniform;
                attribute vec3 a_position;

                void main()
                {
                    gl_Position = floatUniform * vec4(a_position, 1.0);
                })";

            std::string_view fragShader = R"(
                #version 100

                void main(void)
                {
                    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
                })";

            return CreateTestAppearance(scene, vertShader, fragShader);
        }


    private:
        std::unique_ptr<ramses::RamsesFramework> m_ramsesFramework;
        ramses::RamsesClient* m_ramsesClient;
    };
}
