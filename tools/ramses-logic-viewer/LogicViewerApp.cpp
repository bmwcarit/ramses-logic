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
        ramses::DisplayConfig  displayConfig(argc, argv);
        displayConfig.setResizable(true);
        bool autoDetectViewportSize = true;

        try
        {
            args.registerOptions(cli);
            auto setMsaa       = [&](uint32_t numSamples) { displayConfig.setMultiSampling(numSamples); };
            auto setWindowSize = [&](uint32_t /*unused*/) { autoDetectViewportSize = false; };
            auto setClearColor = [&](const auto& clearColor) { m_defaultClearColor = clearColor; };
            cli.add_option_function<uint32_t>("--width", setWindowSize, "Window width (auto-detected by default)");   // value was already parsed by DisplayConfig
            cli.add_option_function<uint32_t>("--height", setWindowSize, "Window height (auto-detected by default)"); // value was already parsed by DisplayConfig
            cli.add_option_function<uint32_t>("--msaa", setMsaa, "Instructs the renderer to apply multisampling.")->check(CLI::IsMember({1, 2, 4, 8}));
            cli.add_option_function<std::array<float, 4u>>("--clear-color", setClearColor, "Background clear color as RGBA (ex. --clearColor 0 0.5 0.8 1");
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
        m_client = m_framework->createClient("ramses-logic-viewer");
        if (!m_client)
        {
            std::cerr << "Could not create ramses client" << std::endl;
            return static_cast<int>(ExitCode::ErrorRamsesClient);
        }

        m_scene = m_client->loadSceneFromFile(args.sceneFile().c_str());
        if (!m_scene)
        {
            std::cerr << "Failed to load scene: " << args.sceneFile() << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadScene);
        }
        m_scene->publish();
        m_scene->flush();

        const auto guiSceneId = ramses::sceneId_t(m_scene->getSceneId().getValue() + 1);

        if (autoDetectViewportSize && getPreferredSize(*m_scene, m_width, m_height))
        {
            displayConfig.setWindowRectangle(0, 0, m_width, m_height);
        }

        int32_t winX = 0;
        int32_t winY = 0;
        displayConfig.getWindowRectangle(winX, winY, m_width, m_height);
        m_imguiHelper = std::make_unique<rlogic::ImguiClientHelper>(*m_client, m_width, m_height, guiSceneId);

        ramses::RamsesRenderer* renderer = nullptr;
        ramses::displayId_t displayId;
        if (!args.headless())
        {
            renderer = m_framework->createRenderer({});
            if (!renderer)
            {
                std::cerr << "Could not create ramses renderer" << std::endl;
                return static_cast<int>(ExitCode::ErrorRamsesRenderer);
            }

            displayId = initDisplay(args, *renderer, displayConfig);
            if (!displayId.isValid())
            {
                std::cerr << "Could not create ramses display" << std::endl;
                return static_cast<int>(ExitCode::ErrorNoDisplay);
            }
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

        if (!fs::exists(args.logicFile()))
        {
            std::cerr << "Logic file does not exist: " << args.logicFile() << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadLogic);
        }

        rlogic::EFeatureLevel engineFeatureLevel = rlogic::EFeatureLevel_01;
        if (!rlogic::LogicEngine::GetFeatureLevelFromFile(args.logicFile(), engineFeatureLevel))
        {
            std::cerr << "Could not parse feature level from logic file" << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadLogic);
        }

        m_settings = std::make_unique<rlogic::LogicViewerSettings>();
        if (args.headless())
        {
            m_viewer = std::make_unique<rlogic::LogicViewer>(engineFeatureLevel, LogicViewer::ScreenshotFunc());
        }
        else
        {
            m_viewer = std::make_unique<rlogic::LogicViewer>(engineFeatureLevel, takeScreenshot);
        }

        if (!m_viewer->loadRamsesLogic(args.logicFile(), m_scene))
        {
            std::cerr << "Failed to load logic file: " << args.logicFile() << std::endl;
            return static_cast<int>(ExitCode::ErrorLoadLogic);
        }

        m_gui = std::make_unique<rlogic::LogicViewerGui>(*m_viewer, *m_settings, args.luaFile());
        if (!args.headless())
        {
            m_gui->setSceneTexture(m_sceneSetup->getTextureSampler(), m_width, m_height);
            m_sceneSetup->apply();

            m_gui->setRendererInfo(*renderer, displayId, m_sceneSetup->getOffscreenBuffer(), m_defaultClearColor);
        }

        if (args.writeConfig())
        {
            ImGui::NewFrame();
            m_gui->saveDefaultLuaFile();
            ImGui::EndFrame();
            m_imguiHelper->windowClosed({});
        }
        else if (!args.luaFunction().empty())
        {
            m_loadLuaStatus = m_viewer->loadLuaFile(args.luaFile());

            if (m_loadLuaStatus.ok())
            {
                m_loadLuaStatus = m_viewer->call(args.luaFunction());
            }
            if (!m_loadLuaStatus.ok())
            {
                std::cerr << m_loadLuaStatus.getMessage() << std::endl;
                return static_cast<int>(ExitCode::ErrorLoadLua);
            }
            m_imguiHelper->windowClosed({});
        }
        else if (!args.exec().empty())
        {
            // default lua file may be missing (explicit lua file is checked by CLI11 before)
            m_loadLuaStatus = m_viewer->loadLuaFile(fs::exists(args.luaFile()) ? args.luaFile() : "");
            if (m_loadLuaStatus.ok())
            {
                m_loadLuaStatus = m_viewer->exec(args.exec());
            }
            if (!m_loadLuaStatus.ok())
            {
                std::cerr << m_loadLuaStatus.getMessage() << std::endl;
                return static_cast<int>(ExitCode::ErrorLoadLua);
            }
            m_imguiHelper->windowClosed({});
        }
        else
        {
            // interactive mode
            // default lua file may be missing (explicit lua file is checked by CLI11 before)
            if (fs::exists(args.luaFile()))
            {
                m_loadLuaStatus = m_viewer->loadLuaFile(args.luaFile());
            }
        }
        return 0;
    }

    ramses::displayId_t LogicViewerApp::initDisplay(const Arguments& args, ramses::RamsesRenderer& renderer, const ramses::DisplayConfig& displayConfig)
    {
        renderer.startThread();
        m_imguiHelper->setRenderer(&renderer);

        const auto display = renderer.createDisplay(displayConfig);
        m_imguiHelper->setDisplayId(display);
        renderer.flush();

        if (!m_imguiHelper->waitForDisplay(display))
        {
            return {};
        }

        if (args.noOffscreen())
        {
            m_sceneSetup = std::make_unique<FramebufferSetup>(*m_imguiHelper, renderer, m_scene, display);
        }
        else
        {
            m_sceneSetup = std::make_unique<OffscreenSetup>(*m_imguiHelper, renderer, m_scene, display, m_width, m_height);
        }

        renderer.setDisplayBufferClearColor(display, m_sceneSetup->getOffscreenBuffer(), m_defaultClearColor[0], m_defaultClearColor[1], m_defaultClearColor[2], m_defaultClearColor[3]);
        renderer.flush();

        return display;
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

