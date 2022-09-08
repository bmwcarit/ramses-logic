//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-client.h"
#include "ramses-framework-api/RamsesFrameworkTypes.h"
#include "Result.h"
#include <array>

class ISceneSetup;
class Arguments;

namespace ramses
{
    class DisplayConfig;
    class RendererConfig;
}

namespace rlogic
{
    class ImguiClientHelper;
    class LogicViewer;
    class LogicViewerGui;
    struct LogicViewerSettings;

    class LogicViewerApp
    {
    public:
        enum class ExitCode
        {
            Ok                  = 0,
            ErrorRamsesClient   = 1,
            ErrorRamsesRenderer = 2,
            ErrorSceneControl   = 3,
            ErrorLoadScene      = 4,
            ErrorLoadLogic      = 5,
            ErrorLoadLua        = 6,
            ErrorNoDisplay      = 7,
            ErrorUnknown        = -1,
        };

        LogicViewerApp(int argc, char const* const* argv);

        ~LogicViewerApp();

        LogicViewerApp(const LogicViewerApp&) = delete;

        LogicViewerApp& operator=(const LogicViewerApp&) = delete;

        [[nodiscard]] bool doOneLoop();

        [[nodiscard]] int exitCode() const;

        [[nodiscard]] int run();

        [[nodiscard]] rlogic::LogicViewer* getViewer();

        [[nodiscard]] rlogic::ImguiClientHelper* getImguiClientHelper();

        [[nodiscard]] const rlogic::LogicViewerSettings* getSettings() const;

    private:
        [[nodiscard]] int init(int argc, char const* const* argv);
        [[nodiscard]] ramses::displayId_t initDisplay(const Arguments& args, ramses::RamsesRenderer& renderer, const ramses::DisplayConfig& displayConfig);

        std::unique_ptr<ramses::RamsesFramework>     m_framework;
        std::unique_ptr<rlogic::LogicViewerSettings> m_settings;
        std::unique_ptr<rlogic::ImguiClientHelper>   m_imguiHelper;
        std::unique_ptr<rlogic::LogicViewer>         m_viewer;
        std::unique_ptr<rlogic::LogicViewerGui>      m_gui;
        std::unique_ptr<ISceneSetup>                 m_sceneSetup;

        ramses::RamsesClient* m_client = nullptr;
        ramses::Scene* m_scene = nullptr;
        rlogic::Result m_loadLuaStatus;

        uint32_t m_width  = 0u;
        uint32_t m_height = 0u;
        std::array<float, 4> m_defaultClearColor{ 0, 0, 0, 1 };

        int m_exitCode = -1;
    };

    inline int LogicViewerApp::exitCode() const
    {
        return m_exitCode;
    }

    inline int LogicViewerApp::run()
    {
        while (doOneLoop())
            ;
        return m_exitCode;
    }

    inline rlogic::LogicViewer* LogicViewerApp::getViewer()
    {
        return m_viewer.get();
    }

    inline rlogic::ImguiClientHelper* LogicViewerApp::getImguiClientHelper()
    {
        return m_imguiHelper.get();
    }

    inline const rlogic::LogicViewerSettings* LogicViewerApp::getSettings() const
    {
        return m_settings.get();
    }
}

