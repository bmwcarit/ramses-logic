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
#include "FeatureLevelTestValues.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesRenderPassBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/AnimationNodeConfig.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/Logger.h"
#include "ramses-logic/RamsesLogicVersion.h"

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "ramses-client-api/RenderPass.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-framework-api/RamsesVersion.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LogicEngineImpl.h"
#include "impl/DataArrayImpl.h"
#include "impl/PropertyImpl.h"
#include "internals/ApiObjects.h"
#include "internals/FileUtils.h"
#include "LogTestUtils.h"

#include "generated/LogicEngineGen.h"
#include "ramses-logic-build-config.h"
#include "fmt/format.h"

#include <fstream>

namespace rlogic::internal
{
    class ALogicEngine_Serialization : public ALogicEngineBase, public ::testing::TestWithParam<EFeatureLevel>
    {
    public:
        ALogicEngine_Serialization() : ALogicEngineBase{ GetParam() }
        {
        }

    protected:
        static std::vector<char> CreateTestBuffer(const SaveFileConfig& config = {})
        {
            LogicEngine logicEngineForSaving{ GetParam() };
            logicEngineForSaving.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param = Type:Int32()
                end
                function run(IN,OUT)
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

    INSTANTIATE_TEST_SUITE_P(
        ALogicEngine_SerializationTests,
        ALogicEngine_Serialization,
        rlogic::internal::GetFeatureLevelTestValues());

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromInvalidFile)
    {
        EXPECT_FALSE(m_logicEngine.loadFromFile("invalid"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Failed to load file 'invalid'"));
    }

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithoutApiObjects)
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
                    builder.CreateString(g_PROJECT_VERSION)),
                0, // missing api objects
                0,
                GetParam()
            );

            const char* fileIdentifier = (GetParam() == EFeatureLevel_01 ? rlogic_serialization::LogicEngineIdentifier() : "rl02");
            builder.Finish(logicEngine, fileIdentifier);

            ASSERT_TRUE(FileUtils::SaveBinary("no_api_objects.bin", builder.GetBufferPointer(), builder.GetSize()));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("no_api_objects.bin"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("doesn't contain API objects"));
    }

    TEST_P(ALogicEngine_Serialization, ProducesErrorWhenProvidingAFolderAsTargetForSaving)
    {
        fs::create_directories("folder");
        EXPECT_FALSE(m_logicEngine.saveToFile("folder"));
        EXPECT_EQ("Failed to save content to path 'folder'!", m_logicEngine.getErrors()[0].message);
    }

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFolder)
    {
        fs::create_directories("folder");
        EXPECT_FALSE(m_logicEngine.loadFromFile("folder"));
        EXPECT_EQ("Failed to load file 'folder'", m_logicEngine.getErrors()[0].message);
    }

    TEST_P(ALogicEngine_Serialization, DeserializesFromMemoryBuffer)
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

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserializedFromCorruptedData)
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

    TEST_P(ALogicEngine_Serialization, PrintsMetadataInfoOnLoad)
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

    TEST_P(ALogicEngine_Serialization, PrintsMetadataInfoOnLoad_NoVersionInfoProvided)
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

    // the special file identifiers in flatbuffers are at bytes 4-7, so a file smaller than 8 bytes is a special case of broken
    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserializedFromFileSmallerThan8Bytes)
    {
        // Emulate data truncation
        {
            std::vector<char> bufferData = CreateTestBuffer();
            ASSERT_GT(bufferData.size(), 60u);
            std::vector<char> truncated(bufferData.begin(), bufferData.begin() + 7);
            SaveBufferToFile(truncated, "LogicEngine.bin");
        }

        // Test with file API
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("(size: 7) contains corrupted data! Data should be at least 8 bytes"));
        }

        // Test with buffer API
        {
            std::vector<char> truncatedMemory = *FileUtils::LoadBinary("LogicEngine.bin");
            EXPECT_FALSE(m_logicEngine.loadFromBuffer(truncatedMemory.data(), truncatedMemory.size()));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("(size: 7) contains corrupted data! Data should be at least 8 bytes"));
        }
    }

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfDeserializedFromTruncatedData)
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
    TEST_P(ALogicEngine_Serialization, CanBeDeserializedFromHardLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_hard_link("testfile.bin", "hardlink");
        EXPECT_TRUE(m_logicEngine.loadFromFile("hardlink"));
    }

    TEST_P(ALogicEngine_Serialization, CanBeDeserializedFromSymLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_symlink("testfile.bin", "symlink");
        EXPECT_TRUE(m_logicEngine.loadFromFile("symlink"));
    }

    TEST_P(ALogicEngine_Serialization, FailsGracefullyWhenTryingToOpenFromDanglingSymLink)
    {
        EXPECT_TRUE(m_logicEngine.saveToFile("testfile.bin"));
        fs::create_symlink("testfile.bin", "dangling_symlink");
        fs::remove("testfile.bin");
        EXPECT_FALSE(m_logicEngine.loadFromFile("dangling_symlink"));
        EXPECT_EQ("Failed to load file 'dangling_symlink'", m_logicEngine.getErrors()[0].message);
    }
#endif

    TEST_P(ALogicEngine_Serialization, ProducesNoErrorIfDeserializedWithNoScriptsAndNoNodeBindings)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());
        }
    }

    TEST_P(ALogicEngine_Serialization, ProducesNoErrorIfDeserializedWithNoScripts)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto rNodeBinding = m_logicEngine.findByName<RamsesNodeBinding>("binding");
                ASSERT_NE(nullptr, rNodeBinding);
            }
        }
    }

    TEST_P(ALogicEngine_Serialization, ProducesNoErrorIfDeserializedWithoutNodeBindings)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            logicEngine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param = Type:Int32()
                end
                function run(IN,OUT)
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

    TEST_P(ALogicEngine_Serialization, ProducesErrorIfSavedWithValidationWarning)
    {
        // Put logic engine to a dirty state (create new object and don't call update)
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");

        std::vector<std::string> messages;
        std::vector<ELogMessageType> messageTypes;
        ScopedLogContextLevel scopedLogs(ELogMessageType::Warn, [&](ELogMessageType msgType, std::string_view message) {
            messages.emplace_back(message);
            messageTypes.emplace_back(msgType);
        });

        // Set a value and save -> causes warning
        nodeBinding->getInputs()->getChild("visibility")->set<bool>(false);
        ASSERT_TRUE(nodeBinding->m_impl.isDirty());
        m_logicEngine.saveToFile("LogicEngine.bin");

        ASSERT_EQ(2u, messages.size());
        EXPECT_EQ("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!", messages[0]);
        EXPECT_EQ("Failed to saveToFile() because validation warnings were encountered! Refer to the documentation of saveToFile() for details how to address these gracefully.", messages[1]);
        EXPECT_EQ(ELogMessageType::Warn, messageTypes[0]);
        EXPECT_EQ(ELogMessageType::Error, messageTypes[1]);

        // Unset custom log handler
        Logger::SetLogHandler([](ELogMessageType msgType, std::string_view message) {
            (void)message;
            (void)msgType;
        });
    }

    TEST_P(ALogicEngine_Serialization, RefusesToSaveTwoNodeBindingsWhichPointToDifferentScenes)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        ramses::Node* node1 = scene1->createNode("node1");
        ramses::Node* node2 = scene2->createNode("node2");

        m_logicEngine.createRamsesNodeBinding(*node1, ERotationType::Euler_XYZ, "binding1");
        rlogic::RamsesNodeBinding* binding2 = m_logicEngine.createRamsesNodeBinding(*node2, ERotationType::Euler_XYZ, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        ASSERT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_P(ALogicEngine_Serialization, RefusesToSaveTwoCameraBindingsWhichPointToDifferentScenes)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        ramses::PerspectiveCamera* camera1 = scene1->createPerspectiveCamera("camera1");
        ramses::PerspectiveCamera* camera2 = scene2->createPerspectiveCamera("camera2");

        m_logicEngine.createRamsesCameraBinding(*camera1, "binding1");
        rlogic::RamsesCameraBinding* binding2 = m_logicEngine.createRamsesCameraBinding(*camera2, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        ASSERT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses camera 'camera2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_P(ALogicEngine_Serialization, RefusesToSaveTwoRenderPassBindingsWhichPointToDifferentScenes)
    {
        if (GetParam() < EFeatureLevel_02)
            GTEST_SKIP();

        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        auto* rp1 = scene1->createRenderPass("rp1");
        auto* rp2 = scene2->createRenderPass("rp2");

        m_logicEngine.createRamsesRenderPassBinding(*rp1, "binding1");
        auto* binding2 = m_logicEngine.createRamsesRenderPassBinding(*rp2, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        ASSERT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses render pass 'rp2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].object);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].object);
    }

    TEST_P(ALogicEngine_Serialization, RefusesToSaveAppearanceBindingWhichIsFromDifferentSceneThanNodeBinding)
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

    TEST_P(ALogicEngine_Serialization, ProducesNoErrorIfDeserilizedSuccessfully)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            logicEngine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param = Type:Int32()
                end
                function run(IN,OUT)
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
            if (GetParam() >= EFeatureLevel_02)
                logicEngine.createRamsesRenderPassBinding(*m_renderPass, "rpbinding");

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
                EXPECT_EQ((GetParam() < EFeatureLevel_02 ? 4u : 5u), inputs->getChildCount());
                EXPECT_FALSE(rNodeBindingByName->m_impl.isDirty());
            }
            {
                auto rCameraBindingByName = m_logicEngine.findByName<RamsesCameraBinding>("camerabinding");
                auto rCameraBindingById = m_logicEngine.findLogicObjectById(5u);
                ASSERT_NE(nullptr, rCameraBindingByName);
                ASSERT_EQ(rCameraBindingById, rCameraBindingByName);
                const auto inputs = rCameraBindingByName->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(2u, inputs->getChildCount());
                EXPECT_FALSE(rCameraBindingByName->m_impl.isDirty());
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
                EXPECT_FALSE(rAppearanceBindingByName->m_impl.isDirty());
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
            if (GetParam() >= EFeatureLevel_02)
            {
                auto rpBindingByName = m_logicEngine.findByName<RamsesRenderPassBinding>("rpbinding");
                auto rpBindingById = m_logicEngine.findLogicObjectById(8u);
                ASSERT_NE(nullptr, rpBindingByName);
                ASSERT_EQ(rpBindingById, rpBindingByName);
                const auto inputs = rpBindingByName->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
                EXPECT_FALSE(rpBindingByName->m_impl.isDirty());
            }
        }
    }

    TEST_P(ALogicEngine_Serialization, ReplacesCurrentStateWithStateFromFile)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            logicEngine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param = Type:Int32()
                end
                function run(IN,OUT)
                end
            )", {}, "luascript");

            logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            m_logicEngine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param2 = Type:Float()
                end
                function run(IN,OUT)
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

    TEST_P(ALogicEngine_Serialization, DeserializesLinks)
    {
        {
            std::string_view scriptSource = R"(
                function interface(IN,OUT)
                    IN.input = Type:Int32()
                    OUT.output = Type:Int32()
                end
                function run(IN,OUT)
                end
            )";

            LogicEngine logicEngine{ GetParam() };
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

    TEST_P(ALogicEngine_Serialization, InternalLinkDataIsDeletedAfterDeserialization)
    {
        std::string_view scriptSource = R"(
            function interface(IN,OUT)
                IN.input = Type:Int32()
                OUT.output = Type:Int32()
            end
            function run(IN,OUT)
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

    TEST_P(ALogicEngine_Serialization, PreviouslyCreatedModulesAreDeletedInSolStateAfterDeserialization)
    {
        {
            LogicEngine logicEngineForSaving{ GetParam() };
            const std::string_view moduleSrc = R"(
                local mymath = {}
                mymath.PI=3.1415
                return mymath
            )";

            std::string_view script = R"(
                modules("mymath")
                function interface(IN,OUT)
                    OUT.pi = Type:Float()
                end
                function run(IN,OUT)
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

    TEST_P(ALogicEngine_Serialization, IDsAfterDeserializationAreUnique)
    {
        uint64_t serializedId = 0u;
        {
            LogicEngine logicEngine{ GetParam() };
            const auto script = logicEngine.createLuaScript(R"(
                function interface(IN,OUT)
                    IN.param = Type:Int32()
                end
                function run(IN,OUT)
                end
            )", {}, "luascript");
            serializedId = script->getId();

            logicEngine.saveToFile("LogicEngine.bin");
        }

        EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());

        auto script = m_logicEngine.findLogicObjectById(serializedId);
        ASSERT_NE(nullptr, script);
        EXPECT_EQ("luascript", script->getName());

        const auto anotherScript = m_logicEngine.createLuaScript(R"(
            function interface(IN,OUT)
            end
            function run(IN,OUT)
            end
        )");
        EXPECT_GT(anotherScript->getId(), script->getId());
    }

    TEST_P(ALogicEngine_Serialization, persistsUserIds)
    {
        {
            LogicEngine logicEngine{ GetParam() };
            auto* luaModule = logicEngine.createLuaModule(m_moduleSourceCode, {}, "module");
            auto* luaScript = logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
            auto* nodeBinding = logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodeBinding");
            auto* appearanceBinding = logicEngine.createRamsesAppearanceBinding(*m_appearance, "appearanceBinding");
            auto* cameraBinding = logicEngine.createRamsesCameraBinding(*m_camera, "cameraBinding");
            auto* dataArray = logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataArray");
            AnimationNodeConfig config;
            config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
            auto* animationNode = logicEngine.createAnimationNode(config, "animNode");
            auto* timerNode = logicEngine.createTimerNode("timerNode");
            auto* luaInterface = logicEngine.createLuaInterface(R"(
                function interface(IN, OUT)
                end
            )", "intf");

            EXPECT_TRUE(luaModule->setUserId(1u, 2u));
            EXPECT_TRUE(luaScript->setUserId(3u, 4u));
            EXPECT_TRUE(nodeBinding->setUserId(5u, 6u));
            EXPECT_TRUE(appearanceBinding->setUserId(7u, 8u));
            EXPECT_TRUE(cameraBinding->setUserId(9u, 10u));
            EXPECT_TRUE(dataArray->setUserId(11u, 12u));
            EXPECT_TRUE(animationNode->setUserId(13u, 14u));
            EXPECT_TRUE(timerNode->setUserId(15u, 16u));
            EXPECT_TRUE(luaInterface->setUserId(17u, 18u));

            if (GetParam() >= EFeatureLevel_02)
            {
                auto* rpBinding = logicEngine.createRamsesRenderPassBinding(*m_renderPass, "rpBinding");
                EXPECT_TRUE(rpBinding->setUserId(19u, 20u));
                auto* anchor = logicEngine.createAnchorPoint(*nodeBinding, *cameraBinding, "anchor");
                EXPECT_TRUE(anchor->setUserId(21u, 22u));
            }

            logicEngine.saveToFile("LogicEngine.bin");
        }

        EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());

        auto obj = m_logicEngine.findByName<LogicObject>("module");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(1u, obj->getUserId().first);
        EXPECT_EQ(2u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("script");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(3u, obj->getUserId().first);
        EXPECT_EQ(4u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("nodeBinding");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(5u, obj->getUserId().first);
        EXPECT_EQ(6u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("appearanceBinding");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(7u, obj->getUserId().first);
        EXPECT_EQ(8u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("cameraBinding");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(9u, obj->getUserId().first);
        EXPECT_EQ(10u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("dataArray");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(11u, obj->getUserId().first);
        EXPECT_EQ(12u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("animNode");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(13u, obj->getUserId().first);
        EXPECT_EQ(14u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("timerNode");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(15u, obj->getUserId().first);
        EXPECT_EQ(16u, obj->getUserId().second);

        obj = m_logicEngine.findByName<LogicObject>("intf");
        ASSERT_NE(nullptr, obj);
        EXPECT_EQ(17u, obj->getUserId().first);
        EXPECT_EQ(18u, obj->getUserId().second);

        if (GetParam() >= EFeatureLevel_02)
        {
            obj = m_logicEngine.findByName<LogicObject>("rpBinding");
            ASSERT_NE(nullptr, obj);
            EXPECT_EQ(19u, obj->getUserId().first);
            EXPECT_EQ(20u, obj->getUserId().second);

            obj = m_logicEngine.findByName<LogicObject>("anchor");
            ASSERT_NE(nullptr, obj);
            EXPECT_EQ(21u, obj->getUserId().first);
            EXPECT_EQ(22u, obj->getUserId().second);
        }
    }
}
