//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-client.h"
#include "Result.h"

class ISceneSetup;

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

    private:
        [[nodiscard]] int init(int argc, char const* const* argv);

        std::unique_ptr<ramses::RamsesFramework>     m_framework;
        std::unique_ptr<rlogic::LogicViewerSettings> m_settings;
        std::unique_ptr<rlogic::ImguiClientHelper>   m_imguiHelper;
        std::unique_ptr<rlogic::LogicViewer>         m_viewer;
        std::unique_ptr<rlogic::LogicViewerGui>      m_gui;
        std::unique_ptr<ISceneSetup>                 m_sceneSetup;

        ramses::Scene* m_scene = nullptr;
        rlogic::Result m_loadLuaStatus;

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
}

