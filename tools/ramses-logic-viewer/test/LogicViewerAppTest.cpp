//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicViewerApp.h"
#include "LogicViewer.h"
#include "gmock/gmock.h"
#include "RamsesTestUtils.h"
#include "WithTempDirectory.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-client.h"
#include "ImguiClientHelper.h"
#include "fmt/format.h"
#include "CLI/CLI.hpp"
#include <fstream>
#include <list>
#include <mutex>

const auto defaultLuaFile = R"(function default()
    --Interfaces
    rlogic.interfaces["myInterface"]["IN"]["paramFloat"].value = 0
    --Scripts
    --Node bindings
    rlogic.nodeBindings["myNode"]["IN"]["visibility"].value = true
    rlogic.nodeBindings["myNode"]["IN"]["translation"].value = { 0, 0, 0 }
    rlogic.nodeBindings["myNode"]["IN"]["scaling"].value = { 1, 1, 1 }
    rlogic.nodeBindings["myNode"]["IN"]["enabled"].value = true
    --Appearance bindings
    rlogic.appearanceBindings["myAppearance"]["IN"]["green"].value = 0
    rlogic.appearanceBindings["myAppearance"]["IN"]["blue"].value = 0
    --Camera bindings
    rlogic.cameraBindings["myCamera"]["IN"]["viewport"]["offsetX"].value = 0
    rlogic.cameraBindings["myCamera"]["IN"]["viewport"]["offsetY"].value = 0
    rlogic.cameraBindings["myCamera"]["IN"]["viewport"]["width"].value = 800
    rlogic.cameraBindings["myCamera"]["IN"]["viewport"]["height"].value = 800
    rlogic.cameraBindings["myCamera"]["IN"]["frustum"]["nearPlane"].value = 0.1
    rlogic.cameraBindings["myCamera"]["IN"]["frustum"]["farPlane"].value = 100
    rlogic.cameraBindings["myCamera"]["IN"]["frustum"]["fieldOfView"].value = 20
    rlogic.cameraBindings["myCamera"]["IN"]["frustum"]["aspectRatio"].value = 1
    --RenderPass bindings
    rlogic.renderPassBindings["myRenderPass"]["IN"]["enabled"].value = true
    rlogic.renderPassBindings["myRenderPass"]["IN"]["renderOrder"].value = 0
    rlogic.renderPassBindings["myRenderPass"]["IN"]["clearColor"].value = { 0, 0, 0, 1 }
    rlogic.renderPassBindings["myRenderPass"]["IN"]["renderOnce"].value = false
    --Anchor points
end


defaultView = {
    name = "Default",
    description = "",
    update = function(time_ms)
        default()
    end
}

rlogic.views = {defaultView}

-- sample test function for automated image base tests
-- can be executed by command line parameter --exec=test_default
function test_default()
    -- modify properties
    default()
    -- stores a screenshot (relative to the working directory)
    rlogic.screenshot("test_default.png")
end

)";

namespace rlogic::internal
{
    const char* const logicFile = "ALogicViewerAppTest.rlogic";
    const char* const ramsesFile = "ALogicViewerAppTest.ramses";
    const char* const luaFile = "ALogicViewerAppTest.lua";

    class LogHandler
    {
    public:
        void add(ramses::ELogLevel level, const std::string& context, const std::string& msg)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_log.push_back({level, context, msg});
        }

        void clear()
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_log.clear();
        }

        size_t find(const std::string& token)
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            size_t                      count = 0u;
            for (const auto& entry : m_log)
            {
                if (entry.msg.find(token) != std::string::npos)
                {
                    ++count;
                }
            }
            return count;
        }

    private:
        struct LogEntry
        {
            ramses::ELogLevel level;
            std::string       context;
            std::string       msg;
        };
        std::mutex          m_mutex;
        std::list<LogEntry> m_log;
    };


    class ALogicViewerApp : public ::testing::Test
    {
    public:
        ALogicViewerApp()
        {
            createLogicFile();
            m_scene.scene->saveToFile(ramsesFile, true);
            auto handler = [&](ramses::ELogLevel level, const std::string& context, const std::string& msg) { m_log.add(level, context, msg); };
            ramses::RamsesFramework::SetLogHandler(handler);
        }

        ~ALogicViewerApp() override
        {
            ramses::RamsesFramework::SetLogHandler(ramses::LogHandlerFunc());
        }

        void createLogicFile()
        {
            LogicEngine engine{EFeatureLevel_02};

            auto* interface = engine.createLuaInterface(R"(
                function interface(IN,OUT)
                    IN.paramFloat = Type:Float()
                end
            )", "myInterface");

            auto* script = engine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.paramFloat = Type:Float()
                    OUT.paramVec3f = Type:Vec3f()
                end

                function run(IN,OUT)
                    OUT.paramVec3f = {0, 0, IN.paramFloat}
                end
            )", {}, "myScript");

            auto* nodeBinding = engine.createRamsesNodeBinding(*m_scene.meshNode, rlogic::ERotationType::Euler_XYZ, "myNode");

            engine.createRamsesAppearanceBinding(*m_scene.appearance, "myAppearance");
            engine.createRamsesCameraBinding(*m_scene.camera, "myCamera");
            engine.createRamsesRenderPassBinding(*m_scene.renderPass, "myRenderPass");

            engine.link(*interface->getOutputs()->getChild("paramFloat"), *script->getInputs()->getChild("paramFloat"));
            engine.link(*script->getOutputs()->getChild("paramVec3f"), *nodeBinding->getInputs()->getChild("rotation"));

            engine.update();
            engine.saveToFile(logicFile);
        }

        template <typename... T> void createApp(T&&... arglist)
        {
            const auto args = std::array<const char*, sizeof...(T)>{arglist...};
            m_app = std::make_unique<LogicViewerApp>(static_cast<int>(args.size()), args.data());
        }

        bool runUntil(const std::string& message)
        {
            const size_t maxCycles = 200u; // timeout:3.2s (200 * 16ms)
            bool         running   = true;
            for (size_t i = 0u; running && i < maxCycles; ++i)
            {
                running = m_app->doOneLoop();
                const auto count = m_log.find(message);
                m_log.clear();
                if (count > 0)
                {
                    return true;
                }
            }
            return false;
        }

        static void SaveFile(std::string_view text, std::string_view filename = luaFile)
        {
            std::ofstream fileStream(filename.data(), std::ofstream::binary);
            if (!fileStream.is_open())
            {
                throw std::runtime_error("filestream not open");
            }
            fileStream << text;
            if (fileStream.bad())
            {
                throw std::runtime_error("bad filestream");
            }
        }

    protected:
        WithTempDirectory m_withTempDirectory;
        RamsesTestSetup   m_ramses;
        TriangleTestScene m_scene = {m_ramses.createTriangleTestScene()};
        LogHandler m_log;
        std::unique_ptr<LogicViewerApp> m_app;
    };

    TEST_F(ALogicViewerApp, nullparameter)
    {
        LogicViewerApp app(0, nullptr);
        EXPECT_EQ(-1, app.run());
        EXPECT_EQ(-1, app.exitCode());
    }

    TEST_F(ALogicViewerApp, emptyParam)
    {
        createApp("viewer");
        EXPECT_EQ(static_cast<int>(CLI::ExitCodes::RequiredError), m_app->run());
        EXPECT_EQ(static_cast<int>(CLI::ExitCodes::RequiredError), m_app->exitCode());
    }

    TEST_F(ALogicViewerApp, writeDefaultLuaConfiguration)
    {
        createApp("viewer", "--write-config", ramsesFile);
        auto* viewer = m_app->getViewer();
        ASSERT_TRUE(viewer != nullptr);
        EXPECT_EQ(Result(), viewer->update());
        EXPECT_EQ(0, m_app->run());
        EXPECT_TRUE(fs::exists(luaFile));
        std::ifstream str(luaFile);
        std::string   genfile((std::istreambuf_iterator<char>(str)), std::istreambuf_iterator<char>());
        EXPECT_EQ(defaultLuaFile, genfile);
    }

    TEST_F(ALogicViewerApp, writeDefaultLuaConfigurationToOtherFile)
    {
        createApp("viewer", "--write-config=foobar.lua", ramsesFile);
        auto* viewer = m_app->getViewer();
        ASSERT_TRUE(viewer != nullptr);
        EXPECT_EQ(Result(), viewer->update());
        EXPECT_EQ(0, m_app->run());
        EXPECT_TRUE(fs::exists("foobar.lua"));
        std::ifstream str("foobar.lua");
        std::string   genfile((std::istreambuf_iterator<char>(str)), std::istreambuf_iterator<char>());
        EXPECT_EQ(defaultLuaFile, genfile);
    }

    TEST_F(ALogicViewerApp, runInteractive)
    {
        createApp("viewer", ramsesFile);
        EXPECT_TRUE(runUntil("is in state RENDERED caused by command SHOW"));
        EXPECT_TRUE(m_app->doOneLoop());
        EXPECT_TRUE(m_app->doOneLoop());
        EXPECT_TRUE(m_app->doOneLoop());
        auto imgui = m_app->getImguiClientHelper();
        imgui->windowClosed(ramses::displayId_t());
        EXPECT_FALSE(m_app->doOneLoop());
        EXPECT_EQ(0, m_app->exitCode());
    }

    TEST_F(ALogicViewerApp, exec_screenshot)
    {
        SaveFile(R"(
            function test_default()
                -- stores a screenshot (relative to the working directory)
                rlogic.screenshot("test_red.png")
                rlogic.appearanceBindings.myAppearance.IN.green.value = 1
                rlogic.screenshot("test_yellow.png")
            end
        )");
        createApp("viewer", "--exec=test_default", ramsesFile);
        EXPECT_EQ(0, m_app->run());
        EXPECT_TRUE(fs::exists("test_red.png"));
        EXPECT_TRUE(fs::exists("test_yellow.png"));
    }

    TEST_F(ALogicViewerApp, exec_luaError)
    {
        SaveFile(R"(
            function test_default()
                -- stores a screenshot (relative to the working directory)
                rlogic.screenshot("test_red.png")
                rlogic.appearanceBindings.myAppearance.IN.green.value = 1
                rlogic.screenshot("test_yellow.png")
        )");
        createApp("viewer", "--exec=test_default", ramsesFile);
        EXPECT_EQ(static_cast<int>(LogicViewerApp::ExitCode::ErrorLoadLua), m_app->run());
    }

    TEST_F(ALogicViewerApp, interactive_luaError)
    {
        SaveFile(R"(
            function test_default()
                -- stores a screenshot (relative to the working directory)
                rlogic.screenshot("test_red.png")
                rlogic.appearanceBindings.myAppearance.IN.green.value = 1
                rlogic.screenshot("test_yellow.png")
        )");
        createApp("viewer", ramsesFile);
        EXPECT_TRUE(runUntil("is in state RENDERED caused by command SHOW"));
        EXPECT_THAT(m_app->getViewer()->getLastResult().getMessage(), testing::HasSubstr("ALogicViewerAppTest.lua:7: 'end' expected"));
        EXPECT_TRUE(m_app->doOneLoop());
        EXPECT_TRUE(m_app->doOneLoop());
        // does not terminate
    }

    TEST_F(ALogicViewerApp, no_offscreen)
    {
        SaveFile(R"(
            function test_default()
                -- stores a screenshot (relative to the working directory)
                rlogic.screenshot("test_red.png")
            end
        )");
        createApp("viewer", "--exec=test_default", "--no-offscreen", ramsesFile);
        EXPECT_EQ(0, m_app->run());
        EXPECT_TRUE(fs::exists("test_red.png"));
    }

    TEST_F(ALogicViewerApp, windowSize)
    {
        SaveFile(R"(
            function test_default()
                -- stores a screenshot (relative to the working directory)
                rlogic.screenshot("test_red.png")
            end
        )");
        createApp("viewer", "--exec=test_default", "--width=200", "--height=300", ramsesFile);
        EXPECT_EQ(0, m_app->run());
        EXPECT_TRUE(fs::exists("test_red.png"));
    }
}
