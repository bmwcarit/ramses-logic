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

    private:
        std::unique_ptr<ramses::RamsesFramework> m_ramsesFramework;
        ramses::RamsesClient* m_ramsesClient;
    };
}
