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

#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Scene.h"

#include "generated/logicengine_gen.h"
#include "flatbuffers/util.h"
#include <fstream>

namespace rlogic
{
    class ALogicEngine_Serialization : public ALogicEngine
    {
    protected:
    };

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromInvalidFile)
    {
        EXPECT_FALSE(m_logicEngine.loadFromFile("invalid"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Failed to load file 'invalid'", errors[0]);
    }

    TEST_F(ALogicEngine_Serialization, ProducesErrorIfDeserilizedFromFileWithWrongVersion)
    {
        {
            // This is a bit hacky, but an easy way to fake a file with wrong version without
            // doing magic with Git or modifying the serialization code itself
            flatbuffers::FlatBufferBuilder builder;
            auto logicEngine = rlogic_serialization::CreateLogicEngine(
                builder,
                rlogic_serialization::CreateVersion(builder,
                    100, 200, 9000, builder.CreateString("100.200.9000-suffix")),
                0,
                0,
                0
            );

            builder.Finish(logicEngine);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) No way around this reinterpret-cast
            ASSERT_TRUE(flatbuffers::SaveFile("wrong_version.bin", reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize(), true));
        }

        EXPECT_FALSE(m_logicEngine.loadFromFile("wrong_version.bin"));
        const auto& errors = m_logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Version mismatch while loading file 'wrong_version.bin'! Expected version 0.2.x but found 100.200.9000 (full string: 100.200.9000-suffix)", errors[0]);

        std::remove("wrong_version.bin");
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
        std::remove("LogicEngine.bin");
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
                auto rNodeBinding = findRamsesNodeBindingByName("binding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
            }
        }
        std::remove("LogicEngine.bin");
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
                auto script = findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());
            }
        }
        std::remove("LogicEngine.bin");
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

            const ramses::Effect* effect = scene->createEffect(effectDesc, ramses::ResourceCacheFlag_DoNotCache, "glsl shader");
            ramses::Appearance* appearance = scene->createAppearance(*effect, "triangle appearance");

            auto appearanceBinding = logicEngine.createRamsesAppearanceBinding("appearancebinding");
            appearanceBinding->setRamsesAppearance(appearance);

            logicEngine.createRamsesNodeBinding("nodebinding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("LogicEngine.bin", scene));
            EXPECT_TRUE(m_logicEngine.getErrors().empty());

            {
                auto script = findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());

            }
            {
                auto rNodeBinding = findRamsesNodeBindingByName("nodebinding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
            }
            {
                auto rAppearanceBinding = findRamsesAppearanceBindingByName("appearancebinding");
                ASSERT_NE(nullptr, rAppearanceBinding);
                const auto inputs = rAppearanceBinding->getInputs();
                ASSERT_NE(nullptr, inputs);

                ASSERT_EQ(1u, inputs->getChildCount());
                auto floatUniform = inputs->getChild(0);
                ASSERT_NE(nullptr, floatUniform);
                EXPECT_EQ("floatUniform", floatUniform->getName());
                EXPECT_EQ(EPropertyType::Float, floatUniform->getType());
            }
        }
        std::remove("LogicEngine.bin");
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
                ASSERT_EQ(nullptr, findLuaScriptByName("luascript2"));
                ASSERT_EQ(nullptr, findRamsesNodeBindingByName("binding2"));

                auto script = findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                auto rNodeBinding = findRamsesNodeBindingByName("binding");
                ASSERT_NE(nullptr, rNodeBinding);
            }
        }
        std::remove("LogicEngine.bin");
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

            auto sourceScript    = findLuaScriptByName("SourceScript");
            auto targetScript    = findLuaScriptByName("TargetScript");
            auto notLinkedScript = findLuaScriptByName("NotLinkedScript");

            EXPECT_TRUE(m_logicEngine.isLinked(*sourceScript));
            EXPECT_TRUE(m_logicEngine.isLinked(*targetScript));
            EXPECT_FALSE(m_logicEngine.isLinked(*notLinkedScript));
        }
        std::remove("LogicEngine.bin");
    }
}

