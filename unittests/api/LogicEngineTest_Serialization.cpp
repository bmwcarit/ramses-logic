//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "RamsesTestUtils.h"
#include "WithTempDirectory.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/AnimationNodeConfig.h"
#include "ramses-logic/Logger.h"
#include "ramses-logic/RamsesLogicVersion.h"

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-framework-api/RamsesVersion.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LogicEngineImpl.h"
#include "impl/DataArrayImpl.h"
#include "impl/PropertyImpl.h"
#include "internals/ApiObjects.h"
#include "internals/FileUtils.h"
#include "internals/FileFormatVersions.h"
#include "LogTestUtils.h"

#include "generated/LogicEngineGen.h"
#include "ramses-logic-build-config.h"
#include "fmt/format.h"

#include <fstream>

namespace rlogic::internal
{
    class ALogicEngine_Serialization : public ALogicEngine
    {
    protected:
        static std::vector<char> CreateTestBuffer(const SaveFileConfig& config = {})
        {
            LogicEngine logicEngineForSaving;
            logicEngineForSaving.createLuaScript(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", {}, "luascript");

            logicEngineForSaving.saveToFile("tempfile.bin", config);

            return *FileUtils::LoadBinary("tempfile.bin");
        }

        static void SaveBufferToFile(const std::vector<char>& bufferData, const std::string& file)
        {
            FileUtils::SaveBinary(file, static_cast<const void*>(bufferData.data()), bufferData.size());
        }

        WithTempDirectory m_tempDirectory;
    };

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromInvalidFile)
    {
        EXPECT_FALSE(m_logicEngine.loadFromFile("invalid"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Failed to load file 'invalid'"));
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithoutApiObjects)
    {
        {
            ramses::RamsesVersion ramsesVersion = ramses::GetRamsesVersion();
            flatbuffers::FlatBufferBuilder builder;
            auto logicEngine = rlogic_serialization::CreateLogicEngine(
                builder,
                rlogic_serialization::CreateVersion(builder,
                    ramsesVersion.major,
                    ramsesVersion.minor,
                    ramsesVersion.patch,
                    builder.CreateString(ramsesVersion.string)),
                rlogic_serialization::CreateVersion(builder,
                    g_PROJECT_VERSION_MAJOR,
                    g_PROJECT_VERSION_MINOR,
                    g_PROJECT_VERSION_PATCH,
                    builder.CreateString(g_PROJECT_VERSION),
                    g_FileFormatVersion),
                0
            );

            builder.Finish(logicEngine);
            ASSERT_TRUE(FileUtils::SaveBinary("no_api_objects.bin", builder.GetBufferPointer(), builder.GetSize()));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("no_api_objects.bin"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("doesn't contain API objects"));
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorWhenProvidingAFolderAsTargetForSaving)
    {
        fs::create_directories("folder");
        EXPECT_FALSE(m_logicEngine.saveToFile("folder"));
        EXPECT_EQ("Failed to save content to path 'folder'!", m_logicEngine.getErrors()[0].message);
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFolder)
    {
        fs::create_directories("folder");
        EXPECT_FALSE(m_logicEngine.loadFromFile("folder"));
        EXPECT_EQ("Failed to load file 'folder'", m_logicEngine.getErrors()[0].message);
    }

    TEST_F(ALogicEngine_Serialization, DeserializesFromMemoryBuffer)
    {
        const std::vector<char> bufferData = CreateTestBuffer();

        EXPECT_TRUE(m_logicEngine.loadFromBuffer(bufferData.data(), bufferData.size()));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());

        {
            auto script = m_logicEngine.findByName<LuaScript>("luascript");
            ASSERT_NE(nullptr, script);
            const auto inputs = script->getInputs();
            ASSERT_NE(nullptr, inputs);
            EXPECT_EQ(1u, inputs->getChildCount());
        }
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserializedFromCorruptedData)
    {
        // Emulate data corruption
        {
            std::vector<char> bufferData = CreateTestBuffer();
            ASSERT_GT(bufferData.size(), 60u);
            // Do a random byte corruption
            // byte 60 happens to break the format - found out by trial and error
            bufferData[60] = 42;
            SaveBufferToFile(bufferData, "LogicEngine.bin");
        }

        // Test with file API
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("contains corrupted data!"));
        }

        // Test with buffer API
        {
            std::vector<char> corruptedMemory = *FileUtils::LoadBinary("LogicEngine.bin");
            EXPECT_FALSE(m_logicEngine.loadFromBuffer(corruptedMemory.data(), corruptedMemory.size()));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("contains corrupted data!"));
        }
    }

    TEST_F(ALogicEngine_Serialization, PrintsMetadataInfoOnLoad)
    {
        SaveFileConfig config;
        config.setMetadataString("This is a scene exported for tests");
        config.setExporterVersion(3, 1, 2, 42);

        // Test different constructor variations
        SaveFileConfig config2(config);
        config2 = config;
        config2 = std::move(config);

        SaveBufferToFile(CreateTestBuffer(config2), "LogicEngine.bin");

        TestLogCollector logCollector(ELogMessageType::Info);

        // Test with file API
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));

            ASSERT_EQ(logCollector.logs.size(), 3);
            EXPECT_THAT(logCollector.logs[0].message, ::testing::HasSubstr("Loading logic engine content from 'file 'LogicEngine.bin'"));
            EXPECT_THAT(logCollector.logs[1].message, ::testing::HasSubstr("Logic Engine content metadata: 'This is a scene exported for tests'"));
            EXPECT_THAT(logCollector.logs[2].message, ::testing::HasSubstr("Exporter version: 3.1.2 (file format version 42)"));
        }

        logCollector.logs.clear();

        // Test with buffer API
        {
            const std::vector<char> byteBuffer = *FileUtils::LoadBinary("LogicEngine.bin");
            EXPECT_TRUE(m_logicEngine.loadFromBuffer(byteBuffer.data(), byteBuffer.size()));

            ASSERT_EQ(logCollector.logs.size(), 3);
            EXPECT_THAT(logCollector.logs[0].message, ::testing::HasSubstr("Loading logic engine content from 'data buffer"));
            EXPECT_THAT(logCollector.logs[1].message, ::testing::HasSubstr("Logic Engine content metadata: 'This is a scene exported for tests'"));
            EXPECT_THAT(logCollector.logs[2].message, ::testing::HasSubstr("Exporter version: 3.1.2 (file format version 42)"));
        }
    }

    TEST_F(ALogicEngine_Serialization, PrintsMetadataInfoOnLoad_NoVersionInfoProvided)
    {
        SaveFileConfig config;
        SaveBufferToFile(CreateTestBuffer(config), "LogicEngine.bin");

        TestLogCollector logCollector(ELogMessageType::Info);

        // Test with file API
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));

            ASSERT_EQ(logCollector.logs.size(), 3);
            EXPECT_THAT(logCollector.logs[0].message, ::testing::HasSubstr("Loading logic engine content from 'file 'LogicEngine.bin'"));
            EXPECT_THAT(logCollector.logs[1].message, ::testing::HasSubstr("Logic Engine content metadata: ''"));
            EXPECT_THAT(logCollector.logs[2].message, ::testing::HasSubstr("Exporter version: 0.0.0 (file format version 0)"));
        }

        logCollector.logs.clear();

        // Test with buffer API
        {
            const std::vector<char> byteBuffer = *FileUtils::LoadBinary("LogicEngine.bin");
            EXPECT_TRUE(m_logicEngine.loadFromBuffer(byteBuffer.data(), byteBuffer.size()));

            ASSERT_EQ(logCollector.logs.size(), 3);
            EXPECT_THAT(logCollector.logs[0].message, ::testing::HasSubstr("Loading logic engine content from 'data buffer"));
            EXPECT_THAT(logCollector.logs[1].message, ::testing::HasSubstr("Logic Engine content metadata: ''"));
            EXPECT_THAT(logCollector.logs[2].message, ::testing::HasSubstr("Exporter version: 0.0.0 (file format version 0)"));
        }
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserializedFromTruncatedData)
    {
        // Emulate data truncation
        {
            std::vector<char> bufferData = CreateTestBuffer();
            ASSERT_GT(bufferData.size(), 60u);

            // Cutting off the data at byte 60 breaks deserialization (found by trial and error)
            std::vector<char> truncated(bufferData.begin(), bufferData.begin() + 60);
            SaveBufferToFile(truncated, "LogicEngine.bin");
        }

        // Test with file API
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("(size: 60) contains corrupted data!"));
        }

        // Test with buffer API
        {
            std::vector<char> truncatedMemory = *FileUtils::LoadBinary("LogicEngine.bin");
            EXPECT_FALSE(m_logicEngine.loadFromBuffer(truncatedMemory.data(), truncatedMemory.size()));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("(size: 60) contains corrupted data!"));
        }
    }

// The Windows API doesn't allow non-admin access to symlinks, this breaks on dev machines
#ifndef _WIN32
    TEST_F(ALogicEngine_Serialization, CanBeDeserializedFromHardLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_hard_link("testfile.bin", "hardlink");
        EXPECT_TRUE(m_logicEngine.loadFromFile("hardlink"));
    }

    TEST_F(ALogicEngine_Serialization, CanBeDeserializedFromSymLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_symlink("testfile.bin", "symlink");
        EXPECT_TRUE(m_logicEngine.loadFromFile("symlink"));
    }

    TEST_F(ALogicEngine_Serialization, FailsGracefullyWhenTryingToOpenFromDanglingSymLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_symlink("testfile.bin", "dangling_symlink");
        fs::remove("testfile.bin");
        EXPECT_FALSE(m_logicEngine.loadFromFile("dangling_symlink"));
        EXPECT_EQ("Failed to load file 'dangling_symlink'", m_logicEngine.getErrors()[0].message);
    }
#endif

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserializedWithNoScriptsAndNoNodeBindings)
    {
        {
            LogicEngine logicEngine;
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());
        }
    }

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserializedWithNoScripts)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto rNodeBinding = m_logicEngine.findByName<RamsesNodeBinding>("binding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
            }
        }
    }

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserilizedWithoutNodeBindings)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScript(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", {}, "luascript");

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto script = m_logicEngine.findByName<LuaScript>("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());
            }
        }
    }

    TEST_F(ALogicEngine_Serialization, ProducesWarningIfSavedWithBindingValuesWithoutCallingUpdateBefore)
    {
        // Put logic engine to a dirty state (create new object and don't call update)
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
        ASSERT_TRUE(m_logicEngine.m_impl->getApiObjects().isDirty());

        std::string warningMessage;
        ELogMessageType messageType;
        ScopedLogContextLevel scopedLogs(ELogMessageType::Warn, [&warningMessage, &messageType](ELogMessageType msgType, std::string_view message) {
            warningMessage = message;
            messageType = msgType;
        });

        // Set a valud and save -> causes warning
        nodeBinding->getInputs()->getChild("visibility")->set<bool>(false);
        m_logicEngine.saveToFile("LogicEngine.bin");

        EXPECT_EQ("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!", warningMessage);
        EXPECT_EQ(ELogMessageType::Warn, messageType);

        // Unset custom log handler
        Logger::SetLogHandler([](ELogMessageType msgType, std::string_view message) {
            (void)message;
            (void)msgType;
        });
    }

    TEST_F(ALogicEngine_Serialization, RefusesToSaveTwoNodeBindingsWhichPointToDifferentScenes)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        ramses::Node* node1 = scene1->createNode("node1");
        ramses::Node* node2 = scene2->createNode("node2");

        m_logicEngine.createRamsesNodeBinding(*node1, ERotationType::Euler_XYZ, "binding1");
        rlogic::RamsesNodeBinding* binding2 = m_logicEngine.createRamsesNodeBinding(*node2, ERotationType::Euler_XYZ, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_F(ALogicEngine_Serialization, RefusesToSaveTwoCameraBindingsWhichPointToDifferentScenes)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        ramses::PerspectiveCamera* camera1 = scene1->createPerspectiveCamera("camera1");
        ramses::PerspectiveCamera* camera2 = scene2->createPerspectiveCamera("camera2");

        m_logicEngine.createRamsesCameraBinding(*camera1, "binding1");
        rlogic::RamsesCameraBinding* binding2 = m_logicEngine.createRamsesCameraBinding(*camera2, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses camera 'camera2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_F(ALogicEngine_Serialization, RefusesToSaveAppearanceBindingWhichIsFromDifferentSceneThanNodeBinding)
    {
        ramses::Scene* scene2 = m_ramses.createScene(ramses::sceneId_t(2));

        m_logicEngine.createRamsesNodeBinding(*scene2->createNode(), ERotationType::Euler_XYZ, "node binding");
        rlogic::RamsesAppearanceBinding* appBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "app binding");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses appearance 'test appearance' is from scene with id:1 but other objects are from scene with id:2!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(appBinding, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserilizedSuccessfully)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScript(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", {}, "luascript");

            logicEngine.createLuaModule(R"(
                local mymath = {}
                mymath.PI=3.1415
                return mymath
            )", {}, "luamodule");

            logicEngine.createRamsesAppearanceBinding(*m_appearance, "appearancebinding");
            logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
            logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
            const auto data = logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f }, "dataarray");
            AnimationNodeConfig config;
            config.addChannel({ "channel", data, data });
            logicEngine.createAnimationNode(config, "animNode");

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto scriptByName = m_logicEngine.findByName<LuaScript>("luascript");
                auto scriptById = m_logicEngine.findLogicObjectById(1u);
                ASSERT_NE(nullptr, scriptByName);
                ASSERT_EQ(scriptById, scriptByName);
                const auto inputs = scriptByName->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());
                EXPECT_TRUE(scriptByName->m_impl.isDirty());
            }
            {
                auto moduleByName = m_logicEngine.findByName<LuaModule>("luamodule");
                auto moduleById   = m_logicEngine.findLogicObjectById(2u);
                ASSERT_NE(nullptr, moduleByName);
                ASSERT_EQ(moduleById, moduleByName);
            }
            {
                auto rNodeBindingByName = m_logicEngine.findByName<RamsesNodeBinding>("nodebinding");
                auto rNodeBindingById = m_logicEngine.findLogicObjectById(4u);
                ASSERT_NE(nullptr, rNodeBindingByName);
                ASSERT_EQ(rNodeBindingById, rNodeBindingByName);
                const auto inputs = rNodeBindingByName->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
                EXPECT_TRUE(rNodeBindingByName->m_impl.isDirty());
            }
            {
                auto rCameraBindingByName = m_logicEngine.findByName<RamsesCameraBinding>("camerabinding");
                auto rCameraBindingById = m_logicEngine.findLogicObjectById(5u);
                ASSERT_NE(nullptr, rCameraBindingByName);
                ASSERT_EQ(rCameraBindingById, rCameraBindingByName);
                const auto inputs = rCameraBindingByName->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(2u, inputs->getChildCount());
                EXPECT_TRUE(rCameraBindingByName->m_impl.isDirty());
            }
            {
                auto rAppearanceBindingByName = m_logicEngine.findByName<RamsesAppearanceBinding>("appearancebinding");
                auto rAppearanceBindingById = m_logicEngine.findLogicObjectById(3u);
                ASSERT_NE(nullptr, rAppearanceBindingByName);
                ASSERT_EQ(rAppearanceBindingById, rAppearanceBindingByName);
                const auto inputs = rAppearanceBindingByName->getInputs();
                ASSERT_NE(nullptr, inputs);

                ASSERT_EQ(1u, inputs->getChildCount());
                auto floatUniform = inputs->getChild(0);
                ASSERT_NE(nullptr, floatUniform);
                EXPECT_EQ("floatUniform", floatUniform->getName());
                EXPECT_EQ(EPropertyType::Float, floatUniform->getType());
                EXPECT_TRUE(rAppearanceBindingByName->m_impl.isDirty());
            }
            {
                const auto dataArrayByName = m_logicEngine.findByName<DataArray>("dataarray");
                const auto dataArrayById = m_logicEngine.findLogicObjectById(6u);
                ASSERT_NE(nullptr, dataArrayByName);
                ASSERT_EQ(dataArrayById, dataArrayByName);
                EXPECT_EQ(EPropertyType::Float, dataArrayByName->getDataType());
                ASSERT_NE(nullptr, dataArrayByName->getData<float>());
                const std::vector<float> expectedData{ 1.f, 2.f };
                EXPECT_EQ(expectedData, *m_logicEngine.findByName<DataArray>("dataarray")->getData<float>());

                const auto animNodeByName = m_logicEngine.findByName<AnimationNode>("animNode");
                const auto animNodeById = m_logicEngine.findLogicObjectById(7u);
                ASSERT_NE(nullptr, animNodeByName);
                ASSERT_EQ(animNodeById, animNodeByName);
                ASSERT_EQ(1u, animNodeByName->getChannels().size());
                EXPECT_EQ(dataArrayByName, animNodeByName->getChannels().front().timeStamps);
                EXPECT_EQ(dataArrayByName, animNodeByName->getChannels().front().keyframes);
            }
        }
    }

    TEST_F(ALogicEngine_Serialization, ReplacesCurrentStateWithStateFromFile)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScript(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", {}, "luascript");

            logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            m_logicEngine.createLuaScript(R"(
                function interface()
                    IN.param2 = FLOAT
                end
                function run()
                end
            )", {}, "luascript2");

            m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding2");
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                ASSERT_EQ(nullptr, m_logicEngine.findByName<LuaScript>("luascript2"));
                ASSERT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("binding2"));

                auto script = m_logicEngine.findByName<LuaScript>("luascript");
                ASSERT_NE(nullptr, script);
                auto rNodeBinding = m_logicEngine.findByName<RamsesNodeBinding>("binding");
                ASSERT_NE(nullptr, rNodeBinding);
                EXPECT_EQ(m_node, &rNodeBinding->getRamsesNode());
            }
        }
    }

    TEST_F(ALogicEngine_Serialization, DeserializesLinks)
    {
        {
            std::string_view scriptSource = R"(
                function interface()
                    IN.input = INT
                    OUT.output = INT
                end
                function run()
                end
            )";

            LogicEngine logicEngine;
            auto sourceScript1 = logicEngine.createLuaScript(scriptSource, {}, "SourceScript1");
            auto targetScript1 = logicEngine.createLuaScript(scriptSource, {}, "TargetScript1");
            auto sourceScript2 = logicEngine.createLuaScript(scriptSource, {}, "SourceScript2");
            auto targetScript2 = logicEngine.createLuaScript(scriptSource, {}, "TargetScript2");
            logicEngine.createLuaScript(scriptSource, {}, "NotLinkedScript");

            auto srcOutput1 = sourceScript1->getOutputs()->getChild("output");
            auto tgtInput1 = targetScript1->getInputs()->getChild("input");
            auto srcOutput2 = sourceScript2->getOutputs()->getChild("output");
            auto tgtInput2 = targetScript2->getInputs()->getChild("input");

            EXPECT_TRUE(logicEngine.link(*srcOutput1, *tgtInput1));
            EXPECT_TRUE(logicEngine.linkWeak(*srcOutput2, *tgtInput2));

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            auto sourceScript1 = m_logicEngine.findByName<LuaScript>("SourceScript1");
            auto targetScript1 = m_logicEngine.findByName<LuaScript>("TargetScript1");
            auto sourceScript2 = m_logicEngine.findByName<LuaScript>("SourceScript2");
            auto targetScript2 = m_logicEngine.findByName<LuaScript>("TargetScript2");
            auto notLinkedScript = m_logicEngine.findByName<LuaScript>("NotLinkedScript");

            EXPECT_TRUE(m_logicEngine.isLinked(*sourceScript1));
            EXPECT_TRUE(m_logicEngine.isLinked(*targetScript1));
            EXPECT_TRUE(m_logicEngine.isLinked(*sourceScript2));
            EXPECT_TRUE(m_logicEngine.isLinked(*targetScript2));
            EXPECT_FALSE(m_logicEngine.isLinked(*notLinkedScript));

            const internal::LogicNodeDependencies& internalNodeDependencies = m_logicEngine.m_impl->getApiObjects().getLogicNodeDependencies();
            EXPECT_TRUE(internalNodeDependencies.isLinked(sourceScript1->m_impl));
            EXPECT_TRUE(internalNodeDependencies.isLinked(targetScript1->m_impl));
            EXPECT_TRUE(internalNodeDependencies.isLinked(sourceScript2->m_impl));
            EXPECT_TRUE(internalNodeDependencies.isLinked(targetScript2->m_impl));

            const auto srcOutput1 = sourceScript1->getOutputs()->getChild("output");
            const auto tgtInput1 = targetScript1->getInputs()->getChild("input");
            const auto srcOutput2 = sourceScript2->getOutputs()->getChild("output");
            const auto tgtInput2 = targetScript2->getInputs()->getChild("input");
            const auto& srcOutLinks1 = srcOutput1->m_impl->getOutgoingLinks();
            const auto& srcOutLinks2 = srcOutput2->m_impl->getOutgoingLinks();
            ASSERT_EQ(1u, srcOutLinks1.size());
            ASSERT_EQ(1u, srcOutLinks2.size());
            EXPECT_EQ(tgtInput1->m_impl.get(), srcOutLinks1[0].property);
            EXPECT_EQ(tgtInput2->m_impl.get(), srcOutLinks2[0].property);
            EXPECT_FALSE(srcOutLinks1[0].isWeakLink);
            EXPECT_TRUE(srcOutLinks2[0].isWeakLink);
            EXPECT_EQ(srcOutput1->m_impl.get(), tgtInput1->m_impl->getIncomingLink().property);
            EXPECT_EQ(srcOutput2->m_impl.get(), tgtInput2->m_impl->getIncomingLink().property);
            EXPECT_FALSE(tgtInput1->m_impl->getIncomingLink().isWeakLink);
            EXPECT_TRUE(tgtInput2->m_impl->getIncomingLink().isWeakLink);
        }
    }

    TEST_F(ALogicEngine_Serialization, InternalLinkDataIsDeletedAfterDeserialization)
    {
        std::string_view scriptSource = R"(
            function interface()
                IN.input = INT
                OUT.output = INT
            end
            function run()
            end
        )";

        auto sourceScript = m_logicEngine.createLuaScript(scriptSource, {}, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScript(scriptSource, {}, "TargetScript");

        // Save logic engine state without links to file
        m_logicEngine.saveToFile("LogicEngine.bin");

        // Create link (should be wiped after loading from file)
        auto output = sourceScript->getOutputs()->getChild("output");
        auto input = targetScript->getInputs()->getChild("input");
        m_logicEngine.link(*output, *input);

        EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));

        auto sourceScriptAfterLoading = m_logicEngine.findByName<LuaScript>("SourceScript");
        auto targetScriptAfterLoading = m_logicEngine.findByName<LuaScript>("TargetScript");

        // Make a copy of the object so that we can call non-const methods on it too (getTopologicallySortedNodes())
        // This can't happen in user code, we only do this to test internal data
        internal::LogicNodeDependencies internalNodeDependencies = m_logicEngine.m_impl->getApiObjects().getLogicNodeDependencies();
        ASSERT_TRUE(internalNodeDependencies.getTopologicallySortedNodes().has_value());

        // New objects are not linked (because they weren't before saving)
        EXPECT_FALSE(m_logicEngine.isLinked(*sourceScriptAfterLoading));
        EXPECT_FALSE(m_logicEngine.isLinked(*targetScriptAfterLoading));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));

        // Internal topological graph has two unsorted nodes, before and after update()
        EXPECT_EQ(2u, (*internalNodeDependencies.getTopologicallySortedNodes()).size());
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(2u, (*internalNodeDependencies.getTopologicallySortedNodes()).size());
    }

    TEST_F(ALogicEngine_Serialization, PreviouslyCreatedModulesAreDeletedInSolStateAfterDeserialization)
    {
        {
            LogicEngine logicEngineForSaving;
            const std::string_view moduleSrc = R"(
                local mymath = {}
                mymath.PI=3.1415
                return mymath
            )";

            std::string_view script = R"(
                modules("mymath")
                function interface()
                    OUT.pi = FLOAT
                end
                function run()
                    OUT.pi = mymath.PI
                end
            )";

            LuaModule* mymath = logicEngineForSaving.createLuaModule(moduleSrc, {}, "mymath");
            LuaConfig config;
            config.addDependency("mymath", *mymath);
            logicEngineForSaving.createLuaScript(script, config, "script");

            logicEngineForSaving.saveToFile("LogicEngine.bin");
        }

        // Create a module with name colliding with the one from file - it should be deleted
        const std::string_view moduleToBeWipedSrc = R"(
                local mymath = {}
                mymath.PI=4
                return mymath
            )";

        // This module will be overwritten when loading the file below. The logic engine should not
        // keep any leftovers from modules or scripts when loading from file - all content should be
        // taken from the file!
        LuaModule* moduleToBeWiped = m_logicEngine.createLuaModule(moduleToBeWipedSrc, {}, "mymath");
        EXPECT_NE(nullptr, moduleToBeWiped);

        EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));

        m_logicEngine.update();

        auto script = m_logicEngine.findByName<LuaScript>("script");

        // This is the PI from the loaded module, not from 'moduleToBeWiped'
        EXPECT_FLOAT_EQ(3.1415f, *script->getOutputs()->getChild("pi")->get<float>());
    }

    class ALogicEngine_Serialization_Compatibility : public ALogicEngine
    {
    protected:
        void createFlatLogicEngineData(ramses::RamsesVersion ramsesVersion, rlogic::RamsesLogicVersion logicVersion, uint32_t fileFormatVersion)
        {
            ApiObjects emptyApiObjects;

            auto logicEngine = rlogic_serialization::CreateLogicEngine(
                m_fbBuilder,
                rlogic_serialization::CreateVersion(m_fbBuilder,
                    ramsesVersion.major, ramsesVersion.minor, ramsesVersion.patch, m_fbBuilder.CreateString(ramsesVersion.string)),
                rlogic_serialization::CreateVersion(m_fbBuilder,
                    logicVersion.major, logicVersion.minor, logicVersion.patch, m_fbBuilder.CreateString(logicVersion.string),
                    fileFormatVersion),
                ApiObjects::Serialize(emptyApiObjects, m_fbBuilder)
            );

            m_fbBuilder.Finish(logicEngine);
        }

        static ramses::RamsesVersion FakeRamsesVersion()
        {
            ramses::RamsesVersion version{
                "10.20.900-suffix",
                10,
                20,
                900
            };
            return version;
        }

        flatbuffers::FlatBufferBuilder m_fbBuilder;
        WithTempDirectory m_tempDir;
    };

    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesErrorIfDeserilizedFromFileReferencingIncompatibleRamsesVersion)
    {
        const uint32_t fileVersionDoesNotMatter = 0;
        createFlatLogicEngineData(FakeRamsesVersion(), GetRamsesLogicVersion(), fileVersionDoesNotMatter);

        ASSERT_TRUE(FileUtils::SaveBinary("wrong_ramses_version.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        EXPECT_FALSE(m_logicEngine.loadFromFile("wrong_ramses_version.bin"));
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Version mismatch while loading file 'wrong_ramses_version.bin' (size: "));
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr(fmt::format("Expected Ramses version {}.x.x but found 10.20.900-suffix", ramses::GetRamsesVersion().major)));

        //Also test with buffer version of the API
        EXPECT_FALSE(m_logicEngine.loadFromBuffer(m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));
        errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Version mismatch while loading data buffer"));
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Expected Ramses version 27.x.x but found 10.20.900-suffix"));
    }

    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesErrorIfDeserilizedFromNewerFileVersion)
    {
        // Format was changed
        constexpr uint32_t versionFromFuture = g_FileFormatVersion + 1;
        createFlatLogicEngineData(ramses::GetRamsesVersion(), GetRamsesLogicVersion(), versionFromFuture);

        ASSERT_TRUE(FileUtils::SaveBinary("temp.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        EXPECT_FALSE(m_logicEngine.loadFromFile("temp.bin"));
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr(fmt::format("is too new! Expected file version {} but found {}", g_FileFormatVersion, versionFromFuture)));
    }

    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesErrorIfDeserilizedFromOlderFileVersion)
    {
        // Format was changed
        constexpr uint32_t oldVersion = g_FileFormatVersion - 1;
        createFlatLogicEngineData(ramses::GetRamsesVersion(), GetRamsesLogicVersion(), oldVersion);

        ASSERT_TRUE(FileUtils::SaveBinary("temp.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        EXPECT_FALSE(m_logicEngine.loadFromFile("temp.bin"));
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr(fmt::format("is too old! Expected file version {} but found {}", g_FileFormatVersion, oldVersion)));
    }

    TEST(ALogicEngine_Binary_Compatibility, CanLoadAndUpdateABinaryFileExportedWithLastCompatibleVersionOfEngine)
    {
        // Execute the testAssetProducer in /testAssetProducer to re-export with newer version on file breaking changes
        // Then copy the resulting testLogic.bin and testScene.bin to unittests/res folder
        {
            // Load and update works
            RamsesTestSetup ramses;
            LogicEngine     logicEngine;
            ramses::Scene*  scene = &ramses.loadSceneFromFile("res/unittests/testScene.bin");
            ASSERT_NE(nullptr, scene);
            ASSERT_TRUE(logicEngine.loadFromFile("res/unittests/testLogic.bin", scene));

            // Contains objects and their input/outputs

            ASSERT_NE(nullptr, logicEngine.findByName<LuaModule>("nestedModuleMath"));
            ASSERT_NE(nullptr, logicEngine.findByName<LuaModule>("moduleMath"));
            ASSERT_NE(nullptr, logicEngine.findByName<LuaModule>("moduleTypes"));
            ASSERT_NE(nullptr, logicEngine.findByName<LuaScript>("script1"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("intInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("int64Input"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec2iInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec3iInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec4iInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec2fInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec3fInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("vec4fInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("boolInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("stringInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("structInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("arrayInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getOutputs()->getChild("floatOutput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script1")->getOutputs()->getChild("nodeTranslation"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("cameraViewport")->getChild("offsetX"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("cameraViewport")->getChild("offsetY"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("cameraViewport")->getChild("width"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("cameraViewport")->getChild("height"));
            EXPECT_NE(nullptr, logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("floatUniform"));
            ASSERT_NE(nullptr, logicEngine.findByName<AnimationNode>("animNode"));
            EXPECT_EQ(5u, logicEngine.findByName<AnimationNode>("animNode")->getInputs()->getChildCount());
            EXPECT_NE(nullptr, logicEngine.findByName<AnimationNode>("animNode")->getOutputs()->getChild("channel"));
            ASSERT_NE(nullptr, logicEngine.findByName<AnimationNode>("animNodeWithDataProperties"));
            EXPECT_EQ(6u, logicEngine.findByName<AnimationNode>("animNodeWithDataProperties")->getInputs()->getChildCount());
            EXPECT_NE(nullptr, logicEngine.findByName<TimerNode>("timerNode"));

            EXPECT_NE(nullptr, logicEngine.findByName<RamsesNodeBinding>("nodebinding"));
            EXPECT_NE(nullptr, logicEngine.findByName<RamsesCameraBinding>("camerabinding"));
            EXPECT_NE(nullptr, logicEngine.findByName<RamsesAppearanceBinding>("appearancebinding"));
            EXPECT_NE(nullptr, logicEngine.findByName<DataArray>("dataarray"));

            // Can set new value and update()
            logicEngine.findByName<LuaScript>("script1")->getInputs()->getChild("floatInput")->set<float>(42.5f);
            EXPECT_TRUE(logicEngine.update());

            // Values on Ramses are updated according to expectations
            vec3f translation;
            auto node = ramses::RamsesUtils::TryConvert<ramses::Node>(*scene->findObjectByName("test node"));
            auto camera = ramses::RamsesUtils::TryConvert<ramses::OrthographicCamera>(*scene->findObjectByName("test camera"));
            node->getTranslation(translation[0], translation[1], translation[2]);
            EXPECT_THAT(translation, ::testing::ElementsAre(42.5f, 2.f, 3.f));

            EXPECT_EQ(camera->getViewportX(), 45);
            EXPECT_EQ(camera->getViewportY(), 47);
            EXPECT_EQ(camera->getViewportWidth(), 143u);
            EXPECT_EQ(camera->getViewportHeight(), 243u);

            // Animation node is linked and can be animated
            const auto animNode = logicEngine.findByName<AnimationNode>("animNode");
            EXPECT_TRUE(logicEngine.isLinked(*animNode));
            animNode->getInputs()->getChild("play")->set(true);
            animNode->getInputs()->getChild("timeDelta")->set(1.5f);
            EXPECT_TRUE(logicEngine.update());

            ramses::UniformInput uniform;
            auto appearance = ramses::RamsesUtils::TryConvert<ramses::Appearance>(*scene->findObjectByName("test appearance"));
            appearance->getEffect().getUniformInput(1, uniform);
            float floatValue = 0.f;
            appearance->getInputValueFloat(uniform, floatValue);
            EXPECT_FLOAT_EQ(1.5f, floatValue);

            EXPECT_EQ(957, *logicEngine.findByName<LuaScript>("script2")->getOutputs()->getChild("nestedModulesResult")->get<int32_t>());
        }
    }
}

