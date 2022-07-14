//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicViewerApp.h"

#include "Arguments.h"
#include "SceneSetup.h"
#include "ImguiClientHelper.h"
#include "LogicViewerGui.h"
#include "LogicViewer.h"
#include "LogicViewerSettings.h"
#include "ramses-logic/Logger.h"

namespace rlogic
{
    namespace
    {
        bool getPreferredSize(const ramses::Scene& scene, uint32_t& width, uint32_t& height)
        {
            ramses::SceneObjectIterator it(scene, ramses::ERamsesObjectType_RenderPass);
            ramses::RamsesObject*       obj = nullptr;
            while (nullptr != (obj = it.getNext()))
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
                auto* rp = static_cast<ramses::RenderPass*>(obj);
                if (!rp->getRenderTarget())
                {
                    auto* camera = rp->getCamera();
                    if (camera)
                    {
                        width = camera->getViewportWidth();
                        height = camera->getViewportHeight();
                        return true;
                    }
                }
            }
            return false;
        }
    } // namespace

    LogicViewerApp::LogicViewerApp(int argc, const char * const * argv)
    {
        m_exitCode = init(argc, argv);
    }

    LogicViewerApp::~LogicViewerApp() = default;

    int LogicViewerApp::init(int argc, char const* const* argv)
    {
        if (argc == 0 || argv == nullptr)
        {
            return static_cast<int>(ExitCode::ErrorUnknown);
        }

        CLI::App                      cli;
        Arguments                     args;
        ramses::RamsesFrameworkConfig frameworkConfig;
        frameworkConfig.setPeriodicLogsEnabled(false);
        ramses::RendererConfig rendererConfig;
        ramses::DisplayConfig  displayConfig(argc, argv);
        displayConfig.setResizable(true);
        bool autoDetectViewportSize = true;

        try
        {
            args.registerOptions(cli);
            auto setMsaa       = [&](uint32_t numSamples) { displayConfig.setMultiSampling(numSamples); };
            auto setWindowSize = [&](uint32_t /*unused*/) { autoDetectViewportSize = false; };
            cli.add_option_function<uint32_t>("--width", setWindowSize, "Window width (auto-detected by default)");   // value was already parsed by DisplayConfig
            cli.add_option_function<uint32_t>("--height", setWindowSize, "Window height (auto-detected by default)"); // value was already parsed by DisplayConfig
            cli.add_option_function<uint32_t>("--msaa", setMsaa, "Instructs the renderer to apply multisampling.")->check(CLI::IsMember({1, 2, 4, 8}));
        }
        catch (const CLI::Error& err)
        {
            // catch configuration errors
            std::cerr << err.what() << std::endl;
            return -1;
        }

        CLI11_PARSE(cli, argc, argv);

        ramses::RamsesFramework::SetConsoleLogLevel(args.ramsesLogLevel());
        rlogic::Logger::SetLogVerbosityLimit(args.ramsesLogicLogLevel());
        m_framework = std::make_unique<ramses::RamsesFramework>(frameworkConfig);
        ramses::RamsesClient*   client = m_framework->createClient("ramses-logic-viewer");
        if (!client)
        {
            std::cerr << "Could not create ramses client" << std::endl;
            return static_cast<int>(ExitCode::ErrorRamsesClient);
        }

        ramses::RamsesRenderer* renderer = m_framework->createRenderer(rendererConfig);
        if (!renderer)
        {
            std::cerr << "Could not create ramses renderer" << std::endl;
            return static_cast<int>(ExitCode::ErrorRamsesRenderer);
        }

        ramses::RendererSceneControl* sceneControl = renderer->getSceneControlAPI();
        if (!sceneControl)
        {
            std::cerr << "Could not create scene Control" << std::endl;
            return static_cast<int>(ExitCode::ErrorSceneControl);
        }
        renderer->startThread();

        m_scene = client->loadSceneFromFile(args.sceneFile().c_str());
        if (!m_scene)
        {
            std::cerr << "Failed to load scene: " << args.sceneFile() << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadScene);
        }
        m_scene->publish();
        m_scene->flush();

        const auto guiSceneId = ramses::sceneId_t(m_scene->getSceneId().getValue() + 1);

        uint32_t width  = 0;
        uint32_t height = 0;
        if (autoDetectViewportSize && getPreferredSize(*m_scene, width, height))
        {
            displayConfig.setWindowRectangle(0, 0, width, height);
        }

        int32_t winX = 0;
        int32_t winY = 0;
        displayConfig.getWindowRectangle(winX, winY, width, height);
        m_imguiHelper = std::make_unique<rlogic::ImguiClientHelper>(*client, width, height, guiSceneId);
        m_imguiHelper->setRenderer(renderer);

        const ramses::displayId_t display = renderer->createDisplay(displayConfig);
        m_imguiHelper->setDisplayId(display);
        renderer->flush();

        if (!m_imguiHelper->waitForDisplay(display))
        {
            return static_cast<int>(ExitCode::ErrorNoDisplay);
        }

        if (args.noOffscreen())
        {
            m_sceneSetup = std::make_unique<FramebufferSetup>(*m_imguiHelper, renderer, m_scene, display);
        }
        else
        {
            m_sceneSetup = std::make_unique<OffscreenSetup>(*m_imguiHelper, renderer, m_scene, display, width, height);
        }

        auto takeScreenshot = [&](const std::string& filename) {
            static ramses::sceneVersionTag_t ver = 42;
            ++ver;
            m_scene->flush(ver);
            if (m_imguiHelper->waitForSceneVersion(m_scene->getSceneId(), ver))
            {
                if (m_imguiHelper->saveScreenshot(filename, m_sceneSetup->getOffscreenBuffer(), 0, 0, m_sceneSetup->getWidth(), m_sceneSetup->getHeight()))
                {
                    if (m_imguiHelper->waitForScreenshot())
                    {
                        return true;
                    }
                }
            }
            return false;
        };

        rlogic::EFeatureLevel engineFeatureLevel = rlogic::EFeatureLevel_01;
        if (!rlogic::LogicEngine::GetFeatureLevelFromFile(args.logicFile(), engineFeatureLevel))
        {
            std::cerr << "Could not parse feature level from logic file" << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadLogic);
        }

        m_settings = std::make_unique<rlogic::LogicViewerSettings>();
        m_viewer = std::make_unique<rlogic::LogicViewer>(engineFeatureLevel, takeScreenshot);

        if (!m_viewer->loadRamsesLogic(args.logicFile(), m_scene))
        {
            std::cerr << "Failed to load logic file: " << args.logicFile() << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadLogic);
        }

        m_gui = std::make_unique<rlogic::LogicViewerGui>(*m_viewer, *m_settings);
        m_gui->setSceneTexture(m_sceneSetup->getTextureSampler(), width, height);

        m_sceneSetup->apply();

        m_loadLuaStatus = (args.luaFile().empty() || args.writeConfig()) ? rlogic::Result() : m_viewer->loadLuaFile(args.luaFile());

        if (args.writeConfig())
        {
            ImGui::NewFrame();
            m_gui->saveDefaultLuaFile(args.luaFile());
            ImGui::EndFrame();
            m_imguiHelper->windowClosed(display);
        }
        else if (!args.luaFunction().empty())
        {
            if (m_loadLuaStatus.ok())
            {
                m_loadLuaStatus = m_viewer->call(args.luaFunction());
            }
            if (!m_loadLuaStatus.ok())
            {
                std::cerr << m_loadLuaStatus.getMessage() << std::endl;
                return static_cast<int>(ExitCode::ErrorLoadLua);
            }
            m_imguiHelper->windowClosed(display);
        }
        else
        {
        }
        return 0;
    }

    bool LogicViewerApp::doOneLoop()
    {
        const auto isRunning = (m_exitCode == 0) && m_imguiHelper && m_imguiHelper->isRunning();
        if (isRunning)
        {
            const auto updateStatus = m_viewer->update();
            m_scene->flush();
            m_imguiHelper->dispatchEvents();
            ImGui::NewFrame();
            m_gui->draw();
            if (!m_loadLuaStatus.ok())
            {
                m_gui->openErrorPopup(m_loadLuaStatus.getMessage());
                m_loadLuaStatus = rlogic::Result();
            }
            if (!updateStatus.ok())
                m_gui->openErrorPopup(updateStatus.getMessage());
            ImGui::EndFrame();
            m_imguiHelper->draw();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return isRunning;
    }
}

