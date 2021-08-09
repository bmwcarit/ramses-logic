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
        static std::vector<char> CreateTestBuffer()
        {
            LogicEngine logicEngineForSaving;
            logicEngineForSaving.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngineForSaving.saveToFile("tempfile.bin");

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
            auto script = m_logicEngine.findScript("luascript");
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
            logicEngine.createRamsesNodeBinding(*m_node, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto rNodeBinding = m_logicEngine.findNodeBinding("binding");
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
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto script = m_logicEngine.findScript("luascript");
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
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, "binding");
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

        m_logicEngine.createRamsesNodeBinding(*node1, "binding1");
        rlogic::RamsesNodeBinding* binding2 = m_logicEngine.createRamsesNodeBinding(*node2, "binding2");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].node);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].node);
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
        EXPECT_EQ(binding2, m_logicEngine.getErrors()[0].node);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].node);
    }

    TEST_F(ALogicEngine_Serialization, RefusesToSaveAppearanceBindingWhichIsFromDifferentSceneThanNodeBinding)
    {
        ramses::Scene* scene2 = m_ramses.createScene(ramses::sceneId_t(2));

        m_logicEngine.createRamsesNodeBinding(*scene2->createNode(), "node binding");
        rlogic::RamsesAppearanceBinding* appBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "app binding");

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses appearance 'test appearance' is from scene with id:1 but other objects are from scene with id:2!", m_logicEngine.getErrors()[0].message);
        EXPECT_EQ(appBinding, m_logicEngine.getErrors()[0].node);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1].message);
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[1].node);
    }

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserilizedSuccessfully)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngine.createRamsesAppearanceBinding(*m_appearance, "appearancebinding");

            logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");
            logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto script = m_logicEngine.findScript("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());
                EXPECT_TRUE(script->m_impl.get().isDirty());
            }
            {
                auto rNodeBinding = m_logicEngine.findNodeBinding("nodebinding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
                EXPECT_TRUE(rNodeBinding->m_impl.get().isDirty());
            }
            {
                auto rCameraBinding = m_logicEngine.findCameraBinding("camerabinding");
                ASSERT_NE(nullptr, rCameraBinding);
                const auto inputs = rCameraBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(2u, inputs->getChildCount());
                EXPECT_TRUE(rCameraBinding->m_impl.get().isDirty());
            }
            {
                auto rAppearanceBinding = m_logicEngine.findAppearanceBinding("appearancebinding");
                ASSERT_NE(nullptr, rAppearanceBinding);
                const auto inputs = rAppearanceBinding->getInputs();
                ASSERT_NE(nullptr, inputs);

                ASSERT_EQ(1u, inputs->getChildCount());
                auto floatUniform = inputs->getChild(0);
                ASSERT_NE(nullptr, floatUniform);
                EXPECT_EQ("floatUniform", floatUniform->getName());
                EXPECT_EQ(EPropertyType::Float, floatUniform->getType());
                EXPECT_TRUE(rAppearanceBinding->m_impl.get().isDirty());
            }
        }
    }

    TEST_F(ALogicEngine_Serialization, ReplacesCurrentStateWithStateFromFile)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngine.createRamsesNodeBinding(*m_node, "binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            m_logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param2 = FLOAT
                end
                function run()
                end
            )", "luascript2");

            m_logicEngine.createRamsesNodeBinding(*m_node, "binding2");
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", m_scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                ASSERT_EQ(nullptr, m_logicEngine.findScript("luascript2"));
                ASSERT_EQ(nullptr, m_logicEngine.findNodeBinding("binding2"));

                auto script = m_logicEngine.findScript("luascript");
                ASSERT_NE(nullptr, script);
                auto rNodeBinding = m_logicEngine.findNodeBinding("binding");
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
            auto sourceScript = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
            auto targetScript = logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");
            logicEngine.createLuaScriptFromSource(scriptSource, "NotLinkedScript");

            auto output = sourceScript->getOutputs()->getChild("output");
            auto input  = targetScript->getInputs()->getChild("input");

            logicEngine.link(*output, *input);

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            auto sourceScript    = m_logicEngine.findScript("SourceScript");
            auto targetScript    = m_logicEngine.findScript("TargetScript");
            auto notLinkedScript = m_logicEngine.findScript("NotLinkedScript");

            EXPECT_TRUE(m_logicEngine.isLinked(*sourceScript));
            EXPECT_TRUE(m_logicEngine.isLinked(*targetScript));
            EXPECT_FALSE(m_logicEngine.isLinked(*notLinkedScript));

            const internal::LogicNodeDependencies& internalNodeDependencies = m_logicEngine.m_impl->getApiObjects().getLogicNodeDependencies();

            // script without links is not in the internal "LogicNodeConnector"
            EXPECT_EQ(nullptr, internalNodeDependencies.getLinkedOutput(*notLinkedScript->getInputs()->getChild("input")->m_impl));

            // internal "LogicNodeConnector" has pointers from input -> output after deserialization
            EXPECT_EQ(sourceScript->getOutputs()->getChild("output")->m_impl.get(), internalNodeDependencies.getLinkedOutput(*targetScript->getInputs()->getChild("input")->m_impl));

            EXPECT_TRUE(internalNodeDependencies.isLinked(sourceScript->m_impl));
            EXPECT_TRUE(internalNodeDependencies.isLinked(targetScript->m_impl));
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

        auto sourceScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto targetScript = m_logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");

        // Save logic engine state without links to file
        m_logicEngine.saveToFile("LogicEngine.bin");

        // Create link (should be wiped after loading from file)
        auto output = sourceScript->getOutputs()->getChild("output");
        auto input = targetScript->getInputs()->getChild("input");
        m_logicEngine.link(*output, *input);

        EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));

        auto sourceScriptAfterLoading = m_logicEngine.findScript("SourceScript");
        auto targetScriptAfterLoading = m_logicEngine.findScript("TargetScript");

        // Make a copy of the object so that we can call non-const methods on it too (getTopologicallySortedNodes())
        // This can't happen in user code, we only do this to test internal data
        internal::LogicNodeDependencies internalNodeDependencies = m_logicEngine.m_impl->getApiObjects().getLogicNodeDependencies();
        ASSERT_TRUE(internalNodeDependencies.getTopologicallySortedNodes().has_value());

        // New objects are not linked (because they weren't before saving)
        EXPECT_FALSE(m_logicEngine.isLinked(*sourceScriptAfterLoading));
        EXPECT_FALSE(m_logicEngine.isLinked(*targetScriptAfterLoading));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));

        // "Connector" class has no links
        EXPECT_EQ(0u, internalNodeDependencies.getLinks().size());

        // Internal topological graph has two unsorted nodes, before and after update()
        EXPECT_EQ(2u, (*internalNodeDependencies.getTopologicallySortedNodes()).size());
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(2u, (*internalNodeDependencies.getTopologicallySortedNodes()).size());
    }

    class ALogicEngine_Serialization_Compatibility : public ALogicEngine
    {
    protected:
        void saveTestLogicContentToFile(std::string_view fileName) const
        {
            LogicEngine logicEngine;
            LuaScript* script1 = logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.floatInput = FLOAT
                    OUT.floatOutput = FLOAT
                    OUT.nodeTranslation = VEC3F
                end
                function run()
                    OUT.floatOutput = IN.floatInput
                    OUT.nodeTranslation = {IN.floatInput, 2, 3}
                end
            )", "script1");
            LuaScript* script2 = logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.floatInput = FLOAT

                    OUT.cameraViewport = {
                        offsetX = INT,
                        offsetY = INT,
                        width = INT,
                        height = INT
                    }
                    OUT.floatUniform = FLOAT
                end
                function run()
                    OUT.floatUniform = IN.floatInput + 5.0
                    local roundedFloat = math.ceil(IN.floatInput)
                    OUT.cameraViewport = {
                        offsetX = 2 + roundedFloat,
                        offsetY = 4 + roundedFloat,
                        width = 100 + roundedFloat,
                        height = 200 + roundedFloat
                    }
                end
            )", "script2");


            RamsesNodeBinding* nodeBinding = logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");
            RamsesCameraBinding* camBinding = logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
            RamsesAppearanceBinding* appBinding = logicEngine.createRamsesAppearanceBinding(*m_appearance, "appearancebinding");

            logicEngine.link(*script1->getOutputs()->getChild("floatOutput"), *script2->getInputs()->getChild("floatInput"));
            logicEngine.link(*script1->getOutputs()->getChild("nodeTranslation"), *nodeBinding->getInputs()->getChild("translation"));
            logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("offsetX"), *camBinding->getInputs()->getChild("viewport")->getChild("offsetX"));
            logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("offsetY"), *camBinding->getInputs()->getChild("viewport")->getChild("offsetY"));
            logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("width"), *camBinding->getInputs()->getChild("viewport")->getChild("width"));
            logicEngine.link(*script2->getOutputs()->getChild("cameraViewport")->getChild("height"), *camBinding->getInputs()->getChild("viewport")->getChild("height"));
            logicEngine.link(*script2->getOutputs()->getChild("floatUniform"), *appBinding->getInputs()->getChild("floatUniform"));

            logicEngine.update();
            logicEngine.saveToFile(fileName);
        }

        void createFlatLogicEngineData_v07()
        {
            ramses::RamsesVersion ramses105{ "27.0.105", 27, 0, 105 };
            RamsesLogicVersion logic07{ "0.7.0", 0, 7, 0 };
            // Had no explicit file version numbers yet -> simulate by providing 0 (default value)
            createFlatLogicEngineData(ramses105, logic07, 0u);
        }

        void createFlatLogicEngineData_v062()
        {
            ramses::RamsesVersion ramses105{ "27.0.105", 27, 0, 105 };
            RamsesLogicVersion logic062{ "0.6.2", 0, 6, 2 };
            // Had no explicit file version numbers yet -> simulate by providing 0 (default value)
            createFlatLogicEngineData(ramses105, logic062, 0u);
        }

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

    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesErrorIfDeserilizedFromFileVersionFromTheFuture)
    {
        // Format was changed
        constexpr uint32_t versionFromFuture = g_FileFormatVersion + 1;
        createFlatLogicEngineData(ramses::GetRamsesVersion(), GetRamsesLogicVersion(), versionFromFuture);

        ASSERT_TRUE(FileUtils::SaveBinary("compatible_logic_version_from_future.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        EXPECT_FALSE(m_logicEngine.loadFromFile("compatible_logic_version_from_future.bin"));
        auto errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("is too new! Expected runtime version 2 but loaded with 1"));
    }

    // We can remove this test once we stop supporting version 0.7.0
    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesDebugMessageIfDeserilizedFromFileVersion070InCompatibilityMode)
    {
        createFlatLogicEngineData_v07();

        ASSERT_TRUE(FileUtils::SaveBinary("logic_version_before_07.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        std::string debugMessage;
        ScopedLogContextLevel scopedLogs(ELogMessageType::Debug, [&debugMessage](ELogMessageType type, std::string_view message) {
            debugMessage = message;
            (void)type;
            });

        EXPECT_TRUE(m_logicEngine.loadFromFile("logic_version_before_07.bin"));

        EXPECT_EQ("Loading file version exported with Logic Engine '0.7.0' using runtime version '0.8.0' in compatibility mode", debugMessage);
    }

    TEST_F(ALogicEngine_Serialization_Compatibility, ProducesErrorIfDeserilizedFromFilePriorVersion070)
    {
        createFlatLogicEngineData_v062();

        ASSERT_TRUE(FileUtils::SaveBinary("logic_version_before_07.bin", m_fbBuilder.GetBufferPointer(), m_fbBuilder.GetSize()));

        EXPECT_FALSE(m_logicEngine.loadFromFile("logic_version_before_07.bin"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Version mismatch while loading file 'logic_version_before_07.bin' (size: "));
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr(fmt::format("Expected version 0.7.x but found 0.6.2", g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR)));
    }

    TEST_F(ALogicEngine_Serialization_Compatibility, CanLoadAndUpdateABinaryFileExportedWithLastCompatibleVersionOfEngine)
    {
        // Uncomment to re-export with newer version on file breaking changes
        // Then copy the result to /res/unittests folder
        //saveTestLogicContentToFile("binary_file.bin");

        {
            // Load and update works
            ASSERT_TRUE(m_logicEngine.loadFromFile("res/unittests/binary_file.bin", m_scene));

            // Contains objects and their input/outputs
            EXPECT_NE(nullptr, m_logicEngine.findScript("script1")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script1")->getOutputs()->getChild("floatOutput"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script1")->getOutputs()->getChild("nodeTranslation"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getInputs()->getChild("floatInput"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getOutputs()->getChild("cameraViewport")->getChild("offsetX"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getOutputs()->getChild("cameraViewport")->getChild("offsetY"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getOutputs()->getChild("cameraViewport")->getChild("width"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getOutputs()->getChild("cameraViewport")->getChild("height"));
            EXPECT_NE(nullptr, m_logicEngine.findScript("script2")->getOutputs()->getChild("floatUniform"));

            EXPECT_NE(nullptr, m_logicEngine.findNodeBinding("nodebinding"));
            EXPECT_NE(nullptr, m_logicEngine.findCameraBinding("camerabinding"));
            EXPECT_NE(nullptr, m_logicEngine.findAppearanceBinding("appearancebinding"));

            // Can set new value and update()
            m_logicEngine.findScript("script1")->getInputs()->getChild("floatInput")->set<float>(42.5f);
            EXPECT_TRUE(m_logicEngine.update());

            // Values on Ramses are updated according to expectations
            vec3f translation;
            m_node->getTranslation(translation[0], translation[1], translation[2]);
            EXPECT_THAT(translation, ::testing::ElementsAre(42.5f, 2.f, 3.f));

            EXPECT_EQ(m_camera->getViewportX(), 45);
            EXPECT_EQ(m_camera->getViewportY(), 47);
            EXPECT_EQ(m_camera->getViewportWidth(), 143u);
            EXPECT_EQ(m_camera->getViewportHeight(), 243u);

            ramses::UniformInput uniform;
            m_appearance->getEffect().getUniformInput(0, uniform);
            float floatValue = 0.f;
            m_appearance->getInputValueFloat(uniform, floatValue);
            EXPECT_FLOAT_EQ(floatValue, 47.5);
        }
    }
}

