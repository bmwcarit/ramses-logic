//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "RamsesTestUtils.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/Logger.h"

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Scene.h"
#include "ramses-framework-api/RamsesVersion.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LogicEngineImpl.h"
#include "internals/FileUtils.h"
#include "LogTestUtils.h"

#include "generated/logicengine_gen.h"
#include "ramses-logic-build-config.h"
#include "fmt/format.h"

#include <fstream>

namespace rlogic::internal
{
    class ALogicEngine_Serialization : public ALogicEngine
    {
    protected:
        static ramses::Appearance* CreateTestAppearance(ramses::Scene& scene)
        {
            ramses::EffectDescription effectDesc;
            effectDesc.setFragmentShader(R"(
            #version 100

            void main(void)
            {
                gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            })");

            effectDesc.setVertexShader(R"(
            #version 100

            uniform highp float floatUniform;
            attribute vec3 a_position;

            void main()
            {
                gl_Position = floatUniform * vec4(a_position, 1.0);
            })");

            const ramses::Effect* effect = scene.createEffect(effectDesc);
            return scene.createAppearance(*effect, "test appearance");
        }

        void TearDown() override
        {
            std::remove("no_version.bin");
            std::remove("wrong_ramses_version.bin");
            std::remove("wrong_version.bin");
            std::remove("LogicEngine.bin");
        }
    };

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromInvalidFile)
    {
        EXPECT_FALSE(m_logicEngine.loadFromFile("invalid"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Failed to load file 'invalid'", errors[0]);
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithoutVersionInfo)
    {
        {
            flatbuffers::FlatBufferBuilder builder;
            auto logicEngine = rlogic_serialization::CreateLogicEngine(
                builder
            );

            builder.Finish(logicEngine);
            ASSERT_TRUE(FileUtils::SaveBinary("no_version.bin", builder.GetBufferPointer(), builder.GetSize()));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("no_version.bin"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("File 'no_version.bin' doesn't contain logic engine data with readable version specifiers", errors[0]);
    }


    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithWrongRamsesVersion)
    {
        {
            flatbuffers::FlatBufferBuilder builder;
            auto logicEngine = rlogic_serialization::CreateLogicEngine(
                builder,
                rlogic_serialization::CreateVersion(builder,
                    10, 20, 900, builder.CreateString("10.20.900-suffix")),
                rlogic_serialization::CreateVersion(builder,
                    100, 200, 9000, builder.CreateString("100.200.9000-suffix"))
            );

            builder.Finish(logicEngine);
            ASSERT_TRUE(FileUtils::SaveBinary("wrong_ramses_version.bin", builder.GetBufferPointer(), builder.GetSize()));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("wrong_ramses_version.bin"));
        const std::string expectedErrorMessage(fmt::format("Version mismatch while loading file 'wrong_ramses_version.bin'! Expected Ramses version {}.x.x but found 10.20.900-suffix in file", ramses::GetRamsesVersion().major));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ(expectedErrorMessage, errors[0]);
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithWrongVersion)
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
                    100, 200, 9000, builder.CreateString("100.200.9000-suffix"))
            );

            builder.Finish(logicEngine);
            ASSERT_TRUE(FileUtils::SaveBinary("wrong_version.bin", builder.GetBufferPointer(), builder.GetSize()));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("wrong_version.bin"));
        const std::string expectedErrorMessage(fmt::format("Version mismatch while loading file 'wrong_version.bin'! Expected version {}.{}.x but found 100.200.9000-suffix in file", g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ(expectedErrorMessage, errors[0]);
    }

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
            logicEngine.createRamsesNodeBinding("binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
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
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding("binding");
        ASSERT_TRUE(m_logicEngine.m_impl->isDirty());

        std::string warningMessage;
        ELogMessageType messageType;
        ScopedLogContextLevel scopedLogs(ELogMessageType::WARNING, [&warningMessage, &messageType](ELogMessageType msgType, std::string_view message) {
            warningMessage = message;
            messageType = msgType;
        });

        // Set a valud and save -> causes warning
        nodeBinding->getInputs()->getChild("visibility")->set<bool>(false);
        m_logicEngine.saveToFile("LogicEngine.bin");

        EXPECT_EQ("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!", warningMessage);
        EXPECT_EQ(ELogMessageType::WARNING, messageType);

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

        rlogic::RamsesNodeBinding* binding1 = m_logicEngine.createRamsesNodeBinding("binding1");
        rlogic::RamsesNodeBinding* binding2 = m_logicEngine.createRamsesNodeBinding("binding2");

        binding1->setRamsesNode(node1);
        binding2->setRamsesNode(node2);

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0]);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1]);
    }

    TEST_F(ALogicEngine_Serialization, RefusesToSaveAppearanceBindingWhichIsFromDifferentSceneThanNodeBinding)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene1 = testSetup.createScene(ramses::sceneId_t(1));
        ramses::Scene* scene2 = testSetup.createScene(ramses::sceneId_t(2));

        ramses::Node* node = scene1->createNode("node");
        ramses::Appearance* appearance = CreateTestAppearance(*scene2);

        rlogic::RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding("node binding");
        rlogic::RamsesAppearanceBinding* appBinding = m_logicEngine.createRamsesAppearanceBinding("app binding");

        nodeBinding->setRamsesNode(node);
        appBinding->setRamsesAppearance(appearance);

        EXPECT_FALSE(m_logicEngine.saveToFile("will_not_be_written.logic"));
        EXPECT_EQ(2u, m_logicEngine.getErrors().size());
        EXPECT_EQ("Ramses appearance 'test appearance' is from scene with id:2 but other objects are from scene with id:1!", m_logicEngine.getErrors()[0]);
        EXPECT_EQ("Can't save a logic engine to file while it has references to more than one Ramses scene!", m_logicEngine.getErrors()[1]);
    }

    TEST_F(ALogicEngine_Serialization, ProducesNoErrorIfDeserilizedSuccessfully)
    {
        RamsesTestSetup testSetup;
        ramses::Scene* scene = testSetup.createScene();

        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            auto appearanceBinding = logicEngine.createRamsesAppearanceBinding("appearancebinding");
            appearanceBinding->setRamsesAppearance(CreateTestAppearance(*scene));

            logicEngine.createRamsesNodeBinding("nodebinding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", scene));
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

            logicEngine.createRamsesNodeBinding("binding");
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

            m_logicEngine.createRamsesNodeBinding("binding2");
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                ASSERT_EQ(nullptr, m_logicEngine.findScript("luascript2"));
                ASSERT_EQ(nullptr, m_logicEngine.findNodeBinding("binding2"));

                auto script = m_logicEngine.findScript("luascript");
                ASSERT_NE(nullptr, script);
                auto rNodeBinding = m_logicEngine.findNodeBinding("binding");
                ASSERT_NE(nullptr, rNodeBinding);
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

            const internal::LogicNodeDependencies& internalNodeDependencies = m_logicEngine.m_impl->getLogicNodeDependencies();

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

        // Make a copy of the object so that we can call non-const methods on it too (updateTopologicalSorting())
        // This can't happen in user code, we only do this to test internal data
        internal::LogicNodeDependencies internalNodeDependencies = m_logicEngine.m_impl->getLogicNodeDependencies();
        ASSERT_TRUE(internalNodeDependencies.updateTopologicalSorting());

        // New objects are not linked (because they weren't before saving)
        EXPECT_FALSE(m_logicEngine.isLinked(*sourceScriptAfterLoading));
        EXPECT_FALSE(m_logicEngine.isLinked(*targetScriptAfterLoading));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));
        EXPECT_FALSE(internalNodeDependencies.isLinked(sourceScriptAfterLoading->m_impl));

        // "Connector" class has no links
        EXPECT_EQ(0u, internalNodeDependencies.getLinks().size());

        // Internal topological graph has two unsorted nodes, before and after update()
        EXPECT_EQ(2u, internalNodeDependencies.getOrderedNodesCache().size());
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(2u, internalNodeDependencies.getOrderedNodesCache().size());
    }
}

