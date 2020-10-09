//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LuaScriptTest_Base.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"

#include "ramses-framework-api/RamsesFramework.h"
#include "ramses-client-api/RamsesClient.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-client-api/Node.h"

namespace rlogic
{
    class ALuaScript_Runtime : public ALuaScript
    {
    };

    // Not testable, because assignment to userdata can't be catched. It's just a replacement of the current value
    TEST_F(ALuaScript_Runtime, DISABLED_GeneratesErrorWhenOverwritingInputsInRunFunction)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end

            function run()
                IN = {}
            end
        )");

        ASSERT_EQ(nullptr, script);

        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Special global symbol 'IN' should not be overwritten with other types in run() function!!");
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfUndefinedInputIsUsedInRun)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                local undefined = IN.undefined
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Tried to access undefined struct property 'undefined'"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfUndefinedOutputIsUsedInRun)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                OUT.undefined = 5
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Tried to access undefined struct property 'undefined'"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenTryingToAccessPropertiesWithNonStringIndexAtRunTime)
    {
        const std::vector<std::string> wrongIndexTypes = {"[1]", "[true]", "[{x=5}]", "[nil]"};

        const std::string expectedErrorMessageIN = "Only strings supported as table key type!";

        const std::string expectedErrorMessageOUT = "Only strings supported as table key type!";


        std::vector<LuaTestError> allErrorCases;
        for (const auto& errorType : wrongIndexTypes)
        {
            allErrorCases.emplace_back(LuaTestError{"IN" + errorType + " = 5", expectedErrorMessageIN});
            allErrorCases.emplace_back(LuaTestError{"OUT" + errorType + " = 5", expectedErrorMessageOUT});
        }

        for (const auto& singleCase : allErrorCases)
        {
            auto script = m_logicEngine.createLuaScriptFromSource("function interface()\n"
                                                                  "end\n"
                                                                  "function run()\n" +
                                                                  singleCase.errorCode +
                                                                  "\n"
                                                                  "end\n");

            ASSERT_NE(nullptr, script);
            m_logicEngine.update();

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, SetsValueOfTopLevelInputSuccessfully_WhenTemplateMatchesDeclaredInputType)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);
        auto inputs = script->getInputs();

        auto speedInt32 = inputs->getChild("speed");
        auto tempFloat = inputs->getChild("temp");
        auto nameString = inputs->getChild("name");
        auto enabledBool = inputs->getChild("enabled");
        auto vec_2f = inputs->getChild("vec2f");
        auto vec_3f = inputs->getChild("vec3f");
        auto vec_4f = inputs->getChild("vec4f");
        auto vec_2i = inputs->getChild("vec2i");
        auto vec_3i = inputs->getChild("vec3i");
        auto vec_4i = inputs->getChild("vec4i");

        EXPECT_TRUE(speedInt32->set<int32_t>(4711));
        EXPECT_EQ(4711, *speedInt32->get<int32_t>());
        EXPECT_TRUE(tempFloat->set<float>(5.5f));
        EXPECT_FLOAT_EQ(5.5f, *tempFloat->get<float>());
        EXPECT_TRUE(nameString->set<std::string>("name"));
        EXPECT_EQ("name", *nameString->get<std::string>());
        EXPECT_TRUE(enabledBool->set<bool>(true));
        EXPECT_EQ(true, *enabledBool->get<bool>());

        vec2f testvalVec2f{ 1.1f, 1.2f };
        vec3f testvalVec3f{ 2.1f, 2.2f, 2.3f };
        vec4f testvalVec4f{ 3.1f, 3.2f, 3.3f, 3.4f };
        vec2i testvalVec2i{ 1, 2 };
        vec3i testvalVec3i{ 3, 4, 5 };
        vec4i testvalVec4i{ 6, 7, 8, 9 };
        EXPECT_TRUE(vec_2f->set<vec2f>(testvalVec2f));
        EXPECT_TRUE(vec_3f->set<vec3f>(testvalVec3f));
        EXPECT_TRUE(vec_4f->set<vec4f>(testvalVec4f));
        EXPECT_TRUE(vec_2i->set<vec2i>(testvalVec2i));
        EXPECT_TRUE(vec_3i->set<vec3i>(testvalVec3i));
        EXPECT_TRUE(vec_4i->set<vec4i>(testvalVec4i));
        EXPECT_EQ(testvalVec2f, *vec_2f->get<vec2f>());
        EXPECT_EQ(testvalVec3f, *vec_3f->get<vec3f>());
        EXPECT_EQ(testvalVec4f, *vec_4f->get<vec4f>());
        EXPECT_EQ(testvalVec2i, *vec_2i->get<vec2i>());
        EXPECT_EQ(testvalVec3i, *vec_3i->get<vec3i>());
        EXPECT_EQ(testvalVec4i, *vec_4i->get<vec4i>());
    }

    TEST_F(ALuaScript_Runtime, ProvidesCalculatedValueAfterExecution)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(

            function interface()
                IN.a = INT
                IN.b = INT
                OUT.result = INT
            end

            function run()
                OUT.result = IN.a + IN.b
            end
        )");

        auto inputs = script->getInputs();
        auto inputA = inputs->getChild("a");
        auto inputB = inputs->getChild("b");

        auto outputs = script->getOutputs();
        auto result = outputs->getChild("result");

        inputA->set(3);
        inputB->set(4);

        m_logicEngine.update();

        EXPECT_EQ(7, *result->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ReadsDataFromVec234Inputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.vec2f = VEC2F
                IN.vec3f = VEC3F
                IN.vec4f = VEC4F
                IN.vec2i = VEC2I
                IN.vec3i = VEC3I
                IN.vec4i = VEC4I
                OUT.sumOfAllFloats = FLOAT
                OUT.sumOfAllInts = INT
            end

            function run()
                OUT.sumOfAllFloats =
                    IN.vec2f[1] + IN.vec2f[2] +
                    IN.vec3f[1] + IN.vec3f[2] + IN.vec3f[3] +
                    IN.vec4f[1] + IN.vec4f[2] + IN.vec4f[3] + IN.vec4f[4]
                OUT.sumOfAllInts =
                    IN.vec2i[1] + IN.vec2i[2] +
                    IN.vec3i[1] + IN.vec3i[2] + IN.vec3i[3] +
                    IN.vec4i[1] + IN.vec4i[2] + IN.vec4i[3] + IN.vec4i[4]
            end
        )");
        auto  inputs = script->getInputs();
        auto  outputs = script->getOutputs();

        inputs->getChild("vec2f")->set<vec2f>({ 1.1f, 1.2f });
        inputs->getChild("vec3f")->set<vec3f>({ 2.1f, 2.2f, 2.3f });
        inputs->getChild("vec4f")->set<vec4f>({ 3.1f, 3.2f, 3.3f, 3.4f });
        inputs->getChild("vec2i")->set<vec2i>({ 1, 2 });
        inputs->getChild("vec3i")->set<vec3i>({ 3, 4, 5 });
        inputs->getChild("vec4i")->set<vec4i>({ 6, 7, 8, 9 });

        ASSERT_TRUE(m_logicEngine.update());

        EXPECT_FLOAT_EQ(21.9f, *outputs->getChild("sumOfAllFloats")->get<float>());
        EXPECT_EQ(45, *outputs->getChild("sumOfAllInts")->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, WritesValuesToVectorTypeOutputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.vec2f = VEC2F
                OUT.vec3f = VEC3F
                OUT.vec4f = VEC4F
                OUT.vec2i = VEC2I
                OUT.vec3i = VEC3I
                OUT.vec4i = VEC4I
                OUT.nested = {
                    vec = VEC3I,
                    float = FLOAT
                }
            end

            function run()
                OUT.vec2f = {0.1, 0.2}
                OUT.vec3f = {1.1, 1.2, 1.3}
                OUT.vec4f = {2.1, 2.2, 2.3, 2.4}
                OUT.vec2i = {1, 2}
                OUT.vec3i = {3, 4, 5}
                OUT.vec4i = {6, 7, 8, 9}

                OUT.nested =
                {
                    vec = {11, 12, 13},
                    float = 15.5
                }
            end
        )");

        EXPECT_TRUE(m_logicEngine.update());

        auto  outputs = script->getOutputs();

        EXPECT_THAT(*outputs->getChild("vec2f")->get<vec2f>(), ::testing::ElementsAre(0.1f, 0.2f));
        EXPECT_THAT(*outputs->getChild("vec3f")->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
        EXPECT_THAT(*outputs->getChild("vec4f")->get<vec4f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f, 2.4f));

        EXPECT_THAT(*outputs->getChild("vec2i")->get<vec2i>(), ::testing::ElementsAre(1, 2));
        EXPECT_THAT(*outputs->getChild("vec3i")->get<vec3i>(), ::testing::ElementsAre(3, 4, 5));
        EXPECT_THAT(*outputs->getChild("vec4i")->get<vec4i>(), ::testing::ElementsAre(6, 7, 8, 9));

        EXPECT_THAT(*outputs->getChild("nested")->getChild("vec")->get<vec3i>(), ::testing::ElementsAre(11, 12, 13));
        EXPECT_FLOAT_EQ(*outputs->getChild("nested")->getChild("float")->get<float>(), 15.5f);
    }

    TEST_F(ALuaScript_Runtime, PermitsAssigningOfVector_FromTable_WithNilsAtTheEnd)
    {
        // Lua+sol seem to not iterate over nil entries when creating a table
        // Still, we test the behavior explicitly
        const std::vector<std::string> allCases =
        {
            "OUT.vec2f = {1, 2, nil} -- single nil",
            "OUT.vec3f = {1, 2, 3, nil}",
            "OUT.vec4f = {1, 2, 3, 4, nil}",
            "OUT.vec2i = {1, 2, nil}",
            "OUT.vec3i = {1, 2, 3, nil}",
            "OUT.vec4i = {1, 2, 3, 4, nil}",
            "OUT.vec2f = {1, 2, nil, nil} -- two nils",
        };

        for (const auto& aCase : allCases)
        {
            auto scriptSource = std::string(R"(
            function interface()
                OUT.vec2f = VEC2F
                OUT.vec3f = VEC3F
                OUT.vec4f = VEC4F
                OUT.vec2i = VEC2I
                OUT.vec3i = VEC3I
                OUT.vec4i = VEC4I
            end

            function run()
            )");
            scriptSource += aCase;
            scriptSource += "\nend\n";

            auto* script = m_logicEngine.createLuaScriptFromSource(scriptSource);

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_TRUE(m_logicEngine.getErrors().empty());
            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Runtime, PermitsAssigningOfVector_FromTable_WithKeyValuePairs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.vec2f = VEC2F
                OUT.vec3i = VEC3I
            end

            function run()
                OUT.vec2f = {[1] = 0.1, [2] = 0.2}
                OUT.vec3i = {[3] = 13, [2] = 12, [1] = 11} -- shuffled
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());

        auto outputs = script->getOutputs();

        EXPECT_THAT(*outputs->getChild("vec2f")->get<vec2f>(), ::testing::ElementsAre(0.1f, 0.2f));

        EXPECT_THAT(*outputs->getChild("vec3i")->get<vec3i>(), ::testing::ElementsAre(11, 12, 13));
    }

    TEST_F(ALuaScript_Runtime, UsesNestedInputsToProduceResult)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.data = {
                    a = INT,
                    b = INT
                }
                OUT.result = INT
            end
            function run()
                OUT.result = IN.data.a + IN.data.b
            end
        )");

        auto inputs = script->getInputs();
        auto inputA = inputs->getChild("data")->getChild("a");
        auto inputB = inputs->getChild("data")->getChild("b");

        auto outputs = script->getOutputs();
        auto result = outputs->getChild("result");

        inputA->set(3);
        inputB->set(4);

        m_logicEngine.update();
        m_logicEngine.update();

        EXPECT_EQ(7, *result->get<int32_t>());
    }


    TEST_F(ALuaScript_Runtime, StoresDataToNestedOutputs_AsWholeStruct)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.data = INT
                OUT.struct = {
                    field1 = INT,
                    field2 = INT
                }
            end
            function run()
                OUT.struct = {
                    field1 = IN.data + IN.data,
                    field2 = IN.data * IN.data
                }
            end
        )");

        auto inputs = script->getInputs();
        auto input = inputs->getChild("data");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("struct")->getChild("field1");
        auto field2 = outputs->getChild("struct")->getChild("field2");

        input->set(5);

        EXPECT_TRUE(m_logicEngine.update());

        EXPECT_EQ(10, *field1->get<int32_t>());
        EXPECT_EQ(25, *field2->get<int32_t>());
    }


    TEST_F(ALuaScript_Runtime, StoresDataToNestedOutputs_Individually)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.data = INT
                OUT.data = {
                    field1 = INT,
                    field2 = INT
                }
            end
            function run()
                OUT.data.field1 = IN.data + IN.data
                OUT.data.field2 = IN.data * IN.data
            end
        )");

        auto inputs = script->getInputs();
        auto input = inputs->getChild("data");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("data")->getChild("field1");
        auto field2 = outputs->getChild("data")->getChild("field2");

        input->set(5);

        EXPECT_TRUE(m_logicEngine.update());

        EXPECT_EQ(10, *field1->get<int32_t>());
        EXPECT_EQ(25, *field2->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_Underspecified)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.data = {
                    field1 = INT,
                    field2 = INT
                }
            end
            function run()
                OUT.data = {
                    field1 = 5
                }
            end
        )");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("data")->getChild("field1");
        auto field2 = outputs->getChild("data")->getChild("field2");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Element size mismatch when assigning struct property 'data'! Expected: 2 Received: 1"));

        EXPECT_EQ(0, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_Overspecified)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.data = {
                    field1 = INT,
                    field2 = INT
                }
            end
            function run()
                OUT.data = {
                    field1 = 5,
                    field2 = 5,
                    not_specified = 5
                }
            end
        )");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("data")->getChild("field1");
        auto field2 = outputs->getChild("data")->getChild("field2");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Element size mismatch when assigning struct property 'data'! Expected: 2 Received: 3"));

        EXPECT_EQ(0, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_WhenFieldHasWrongType)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.data = {
                    field1 = INT,
                    field2 = INT
                }
                OUT.field2 = INT
            end
            function run()
                OUT.field2 = "this is no integer"
                OUT.data = {
                    field1 = 5,
                    field2 = "this is no integer"
                }
            end
        )");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("data")->getChild("field1");
        auto field2 = outputs->getChild("data")->getChild("field2");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Assigning 'INT' to string output 'field2'!"));

        EXPECT_EQ(0, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
    }

    // Seems to be very expensive to check beforehand if output and input do match
    // The current implementation does a simple check on each structured level, but not as a whole
    TEST_F(ALuaScript_Runtime, DISABLED_ProducesErrorWhenAssigningNestedProperties_WhenNestedSubStructDoesNotMatch)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.data = {
                    field1 = INT,
                    field2 = INT,
                    nested = {
                        field = INT
                    }
                }
            end
            function run()
                OUT.data = {
                    field1 = 5,
                    field2 = 5,
                    nested = {}
                }
            end
        )");

        auto outputs = script->getOutputs();
        auto field1 = outputs->getChild("data")->getChild("field1");
        auto field2 = outputs->getChild("data")->getChild("field2");
        auto nestedfield = outputs->getChild("data")->getChild("nested")->getChild("field");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Not possible to partially assign structs!"));

        EXPECT_EQ(0, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
        EXPECT_EQ(0, *nestedfield->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenImplicitlyRoundingNumbers)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.float = FLOAT
                OUT.int = INT
            end
            function run()
                OUT.int = IN.float
            end
        )");

        auto floatInput = script->getInputs()->getChild("float");
        auto intOutput = script->getOutputs()->getChild("int");

        floatInput->set<float>(1.0f);

        EXPECT_TRUE(m_logicEngine.update());
        ASSERT_TRUE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(1, *intOutput->get<int32_t>());

        floatInput->set<float>(2.5f);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Implicit rounding during assignment of integer output 'int' (value: 2.5)!"));
        EXPECT_EQ(1, *intOutput->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNilToIntOutputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.int = INT
            end
            function run()
                OUT.int = nil
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Assigning nil to INT output 'int'!"));
        EXPECT_EQ(0, *script->getOutputs()->getChild("int")->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningBoolToIntOutputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.int = INT
            end
            function run()
                OUT.int = true
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Assigning boolean to 'INT' output 'int' !"));
        EXPECT_EQ(0, *script->getOutputs()->getChild("int")->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningBoolToStringOutputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.str = STRING
            end
            function run()
                OUT.str = "this is quite ok"
                OUT.str = true   -- this is not ok
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Assigning boolean to 'STRING' output 'str' !"));
        EXPECT_EQ("this is quite ok", *script->getOutputs()->getChild("str")->get<std::string>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNumberToStringOutputs)
    {
        m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.str = STRING
            end
            function run()
                OUT.str = 42   -- this is not ok
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Assigning wrong type (number) to output 'str'!"));
    }

    TEST_F(ALuaScript_Runtime, SupportsMultipleLevelsOfNestedInputs_confidenceTest)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.rabbit = {
                    color = {
                        r = FLOAT,
                        g = FLOAT,
                        b = FLOAT
                    },
                    speed = INT
                }
                OUT.result = FLOAT

            end
            function run()
                OUT.result = (IN.rabbit.color.r + IN.rabbit.color.b + IN.rabbit.color.g) * IN.rabbit.speed
            end
        )");

        auto inputs = script->getInputs();
        auto rabbit = inputs->getChild("rabbit");
        auto color = rabbit->getChild("color");
        auto speed = rabbit->getChild("speed");

        auto outputs = script->getOutputs();
        auto result = outputs->getChild("result");

        color->getChild("r")->set(0.5f);
        color->getChild("g")->set(1.0f);
        color->getChild("b")->set(0.75f);
        speed->set(20);

        m_logicEngine.update();

        EXPECT_EQ(45, result->get<float>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenTryingToAccessFieldsWithNonStringIndexAtRuntime)
    {
        const std::vector<LuaTestError> allCases = {
            {"local var = IN[0]",
            "Only strings supported as table key type!"},
            {"var = IN[true]",
            "Only strings supported as table key type!"},
            {"var = IN[{x = 5}]",
            "Only strings supported as table key type!"},
            {"OUT[0] = 5",
            "Only strings supported as table key type!"},
            {"OUT[true] = 5",
            "Only strings supported as table key type!"},
            {"OUT[{x = 5}] = 5",
            "Only strings supported as table key type!"},
        };

        for (const auto& singleCase : allCases)
        {
            auto script = m_logicEngine.createLuaScriptFromSource(
                "function interface()\n"
                "end\n"
                "function run()\n" +
                singleCase.errorCode + "\n"
                "end\n"
            );

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());
            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenTryingToCreatePropertiesAtRuntime)
    {
        const std::vector<LuaTestError> allCases =
        {
            {"IN.cannot_create_inputs_here = 5",
            "Tried to access undefined struct property 'cannot_create_inputs_here'"},
            {"OUT.cannot_create_outputs_here = 5",
            "Tried to access undefined struct property 'cannot_create_outputs_here'"},
        };

        for (const auto& singleCase : allCases)
        {
            auto script = m_logicEngine.createLuaScriptFromSource(
                "function interface()\n"
                "end\n"
                "function run()\n" +
                singleCase.errorCode + "\n"
                "end\n"
            );

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());
            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    // This is truly evil, too! Perhaps more so than the previous test
    // I think this is not catchable, because it's just a normal function call
    TEST_F(ALuaScript_Runtime, DISABLED_ForbidsCallingInterfaceFunctionInsideTheRunFunction)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            do_the_shuffle = false

            function interface()
                if do_the_shuffle then
                    OUT.str = "... go left! A Kansas city shuffle, lol!"
                else
                    OUT.str = STRING
                end
            end
            function run()
                OUT.str = "They look right... ...and you..."

                do_the_shuffle = true
                interface()
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Not allowed to call interface() function inside run() function!");
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
    }


    TEST_F(ALuaScript_Runtime, AbortsAfterFirstRuntimeError)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.float = FLOAT
                OUT.float = FLOAT
            end
            function run()
                error("next line will not be executed")
                OUT.float = IN.float
            end
        )");

        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("float")->set<float>(0.1f);
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_FLOAT_EQ(0.0f, *script->getOutputs()->getChild("float")->get<float>());
    }

    TEST_F(ALuaScript_Runtime, AssignOutputsFromInputsInDifferentWays_ConfidenceTest)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.assignmentType = STRING

                IN.float = FLOAT
                IN.int   = INT
                IN.struct = {
                    float = FLOAT,
                    int   = INT,
                    struct = {
                        float   = FLOAT,
                        int     = INT,
                        bool    = BOOL,
                        string  = STRING,
                        vec2f  = VEC2F,
                        vec3f  = VEC3F,
                        vec4f  = VEC4F,
                        vec2i  = VEC2I,
                        vec3i  = VEC3I,
                        vec4i  = VEC4I
                    }
                }

                OUT.float = FLOAT
                OUT.int   = INT
                OUT.struct = {
                    float = FLOAT,
                    int   = INT,
                    struct = {
                        float   = FLOAT,
                        int     = INT,
                        bool    = BOOL,
                        string  = STRING,
                        vec2f  = VEC2F,
                        vec3f  = VEC3F,
                        vec4f  = VEC4F,
                        vec2i  = VEC2I,
                        vec3i  = VEC3I,
                        vec4i  = VEC4I
                    }
                }
            end
            function run()
                if IN.assignmentType == "nullify" then
                    OUT.float = 0
                    OUT.int   = 0
                    OUT.struct.float = 0
                    OUT.struct.int   = 0
                    OUT.struct.struct.float     = 0
                    OUT.struct.struct.int       = 0
                    OUT.struct.struct.bool      = false
                    OUT.struct.struct.string    = ""
                    OUT.struct.struct.vec2f    = {0, 0}
                    OUT.struct.struct.vec3f    = {0, 0, 0}
                    OUT.struct.struct.vec4f    = {0, 0, 0, 0}
                    OUT.struct.struct.vec2i    = {0, 0}
                    OUT.struct.struct.vec3i    = {0, 0, 0}
                    OUT.struct.struct.vec4i    = {0, 0, 0, 0}
                elseif IN.assignmentType == "mirror_individually" then
                    OUT.float = IN.float
                    OUT.int   = IN.int
                    OUT.struct.float = IN.struct.float
                    OUT.struct.int   = IN.struct.int
                    OUT.struct.struct.float     = IN.struct.struct.float
                    OUT.struct.struct.int       = IN.struct.struct.int
                    OUT.struct.struct.bool      = IN.struct.struct.bool
                    OUT.struct.struct.string    = IN.struct.struct.string
                    OUT.struct.struct.vec2f     = IN.struct.struct.vec2f
                    OUT.struct.struct.vec3f     = IN.struct.struct.vec3f
                    OUT.struct.struct.vec4f     = IN.struct.struct.vec4f
                    OUT.struct.struct.vec2i     = IN.struct.struct.vec2i
                    OUT.struct.struct.vec3i     = IN.struct.struct.vec3i
                    OUT.struct.struct.vec4i     = IN.struct.struct.vec4i
                elseif IN.assignmentType == "assign_constants" then
                    OUT.float = 0.1
                    OUT.int   = 1
                    OUT.struct.float = 0.2
                    OUT.struct.int   = 2
                    OUT.struct.struct.float     = 0.3
                    OUT.struct.struct.int       = 3
                    OUT.struct.struct.bool      = true
                    OUT.struct.struct.string    = "somestring"
                    OUT.struct.struct.vec2f    = { 0.1, 0.2 }
                    OUT.struct.struct.vec3f    = { 1.1, 1.2, 1.3 }
                    OUT.struct.struct.vec4f    = { 2.1, 2.2, 2.3, 2.4 }
                    OUT.struct.struct.vec2i    = { 1, 2 }
                    OUT.struct.struct.vec3i    = { 3, 4, 5 }
                    OUT.struct.struct.vec4i    = { 6, 7, 8, 9 }
                elseif IN.assignmentType == "assign_struct" then
                    OUT.float = IN.float
                    OUT.int   = IN.int
                    OUT.struct = IN.struct
                else
                    error("unsupported assignment type!")
                end
            end
        )");

        ASSERT_NE(nullptr, script);

        script->getInputs()->getChild("float")->set<float>(0.1f);
        script->getInputs()->getChild("int")->set<int32_t>(1);
        script->getInputs()->getChild("struct")->getChild("float")->set<float>(0.2f);
        script->getInputs()->getChild("struct")->getChild("int")->set<int32_t>(2);
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("float")->set<float>(0.3f);
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("int")->set<int32_t>(3);
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("bool")->set<bool>(true);
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("string")->set<std::string>("somestring");
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec2f")->set<vec2f>({ 0.1f, 0.2f });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec3f")->set<vec3f>({ 1.1f, 1.2f, 1.3f });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec4f")->set<vec4f>({ 2.1f, 2.2f, 2.3f, 2.4f });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec2i")->set<vec2i>({ 1, 2 });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec3i")->set<vec3i>({ 3, 4, 5 });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("vec4i")->set<vec4i>({ 6, 7, 8, 9 });

        std::array<std::string, 3> assignmentTypes =
        {
            "mirror_individually",
            "assign_constants",
            "assign_struct",
        };

        auto outputs = script->getOutputs();
        for (const auto& assignmentType : assignmentTypes)
        {
            EXPECT_TRUE(script->getInputs()->getChild("assignmentType")->set<std::string>("nullify"));
            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_TRUE(script->getInputs()->getChild("assignmentType")->set<std::string>(assignmentType));
            EXPECT_TRUE(m_logicEngine.update());
            EXPECT_TRUE(m_logicEngine.getErrors().empty());


            EXPECT_FLOAT_EQ(0.1f, *outputs->getChild("float")->get<float>());
            EXPECT_EQ(1, *outputs->getChild("int")->get<int32_t>());

            auto struct_lvl1 = outputs->getChild("struct");
            EXPECT_FLOAT_EQ(0.2f, *struct_lvl1->getChild("float")->get<float>());
            EXPECT_EQ(2, *struct_lvl1->getChild("int")->get<int32_t>());

            auto struct_lvl2 = struct_lvl1->getChild("struct");
            EXPECT_FLOAT_EQ(0.3f, *struct_lvl2->getChild("float")->get<float>());
            EXPECT_EQ(3, *struct_lvl2->getChild("int")->get<int32_t>());
            EXPECT_EQ(true, *struct_lvl2->getChild("bool")->get<bool>());
            EXPECT_EQ("somestring", *struct_lvl2->getChild("string")->get<std::string>());

            EXPECT_THAT(*struct_lvl2->getChild("vec2f")->get<vec2f>(), ::testing::ElementsAre(0.1f, 0.2f));
            EXPECT_THAT(*struct_lvl2->getChild("vec3f")->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
            EXPECT_THAT(*struct_lvl2->getChild("vec4f")->get<vec4f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f, 2.4f));
            EXPECT_THAT(*struct_lvl2->getChild("vec2i")->get<vec2i>(), ::testing::ElementsAre(1, 2));
            EXPECT_THAT(*struct_lvl2->getChild("vec3i")->get<vec3i>(), ::testing::ElementsAre(3, 4, 5));
            EXPECT_THAT(*struct_lvl2->getChild("vec4i")->get<vec4i>(), ::testing::ElementsAre(6, 7, 8, 9));
        }
    }

    // This is truly evil! But Lua is a script language, so... Lots of possibilities! :D
    // I think this is not catchable, becuse "run" is a function and not a userdata
    // Therefore it is not catchable in c++
    TEST_F(ALuaScript_Runtime, DISABLED_ForbidsOverwritingRunFunctionInsideTheRunFunction)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.str = STRING
            end
            function run()
                OUT.str = "They look right... ...and you..."

                run = function()
                    OUT.str = "... go left! A Kansas city shuffle, lol!"
                end
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Not allowed to overwrite run() function inside of itself!");
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfInvalidOutPropertyIsAccessed)
    {
        auto scriptWithInvalidOutParam = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                OUT.param = 47.11
            end
        )");

        ASSERT_NE(nullptr, scriptWithInvalidOutParam);
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfInvalidNestedOutPropertyIsAccessed)
    {
        auto scriptWithInvalidStructAccess = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                OUT.struct.param = 47.11
            end
        )");

        ASSERT_NE(nullptr, scriptWithInvalidStructAccess);
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfValidestedButInvalidOutPropertyIsAccessed)
    {
        auto scriptWithValidStructButInvalidField = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.struct = {
                    param = INT
                }
            end
            function run()
                OUT.struct.invalid = 47.11
            end
        )");

        ASSERT_NE(nullptr, scriptWithValidStructButInvalidField);
        m_logicEngine.update();
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALuaScript_Runtime, CanAssignInputDirectlyToOutput)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.param_struct = {
                    param1 = FLOAT,
                    param2_struct = {
                        a = INT,
                        b = INT
                    }
                }
                OUT.param_struct = {
                    param1 = FLOAT,
                    param2_struct = {
                        a = INT,
                        b = INT
                    }
                }
            end
            function run()
                OUT.param_struct = IN.param_struct
            end
        )");

        ASSERT_NE(nullptr, script);

        {
            auto inputs = script->getInputs();
            auto param_struct = inputs->getChild("param_struct");
            param_struct->getChild("param1")->set(1.0f);
            auto param2_struct = param_struct->getChild("param2_struct");
            param2_struct->getChild("a")->set(2);
            param2_struct->getChild("b")->set(3);
        }

        m_logicEngine.update();

        {
            auto outputs = script->getOutputs();
            auto param_struct = outputs->getChild("param_struct");
            EXPECT_FLOAT_EQ(1.0f, *param_struct->getChild("param1")->get<float>());
            auto param2_struct = param_struct->getChild("param2_struct");
            EXPECT_EQ(2, param2_struct->getChild("a")->get<int32_t>());
            EXPECT_EQ(3, param2_struct->getChild("b")->get<int32_t>());
        }
    }

    TEST_F(ALuaScript_Runtime, ProducesNoErrorIfOutputIsSetInFunction)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.param = INT
                OUT.struct1 = {
                    param = INT
                }
                OUT.struct2 = {
                    param = INT
                }
            end
            function setPrimitive(output)
                output.param = 42
            end
            function setSubStruct(output)
                output.struct1 = {
                    param = 43
                }
            end
            function setSubStruct2(output)
                output = {
                    param = 44
                }
            end
            function run()
                setPrimitive(OUT)
                setSubStruct(OUT)
                -- setSubStruct2(OUT.struct2) does not work right now
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
        const auto outputs = script->getOutputs();
        ASSERT_NE(nullptr, outputs);

        ASSERT_EQ(3u, outputs->getChildCount());
        const auto param = outputs->getChild(0);
        const auto struct1 = outputs->getChild(1);
        //const auto struct2 = outputs->getChild(2);

        EXPECT_EQ(42, param->get<int32_t>());

        ASSERT_EQ(1u, struct1->getChildCount());
        EXPECT_EQ(43, struct1->getChild(0)->get<int32_t>());

        // TODO Make this variant possible
        //ASSERT_EQ(1u, struct2->getChildCount());
        //EXPECT_EQ(44, struct2->getChild(0)->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, DoesNotSetOutputIfOutputParamIsPassedToFunction)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.param = INT
            end
            function foo(output)
                param = 42
            end
            function run()
                foo(OUT.param)
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
        const auto outputs = script->getOutputs();
        EXPECT_EQ(0, outputs->getChild(0)->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, HasNoInfluenceOnBindingsIfTheyAreNotLinked)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.inFloat = FLOAT
                IN.inVec3  = VEC3F
                OUT.outFloat = FLOAT
                OUT.outVec3  = VEC3F
            end
            function run()
                OUT.outFloat = IN.inFloat
                OUT.outVec3 = IN.inVec3
            end
        )";

        const std::string_view vertexShaderSource = R"(
            #version 300 es

            uniform highp float floatUniform;

            void main()
            {
                gl_Position = floatUniform * vec4(1.0);
            })";

        const std::string_view fragmentShaderSource = R"(
            #version 300 es

            out lowp vec4 color;
            void main(void)
            {
                color = vec4(1.0, 0.0, 0.0, 1.0);
            })";

        auto script1 = logicEngine.createLuaScriptFromSource(scriptSource, "Script1");
        auto script2 = logicEngine.createLuaScriptFromSource(scriptSource, "Script2");
        auto script3 = logicEngine.createLuaScriptFromSource(scriptSource, "Script3");

        auto script1FloatInput  = script1->getInputs()->getChild("inFloat");
        auto script1FloatOutput = script1->getOutputs()->getChild("outFloat");
        auto script1Vec3Input   = script1->getInputs()->getChild("inVec3");
        auto script1Vec3Output  = script1->getOutputs()->getChild("outVec3");
        auto script2FloatInput  = script2->getInputs()->getChild("inFloat");
        auto script2FloatOutput = script2->getOutputs()->getChild("outFloat");
        auto script2Vec3Input   = script2->getInputs()->getChild("inVec3");
        auto script2Vec3Output  = script2->getOutputs()->getChild("outVec3");
        auto script3FloatInput  = script3->getInputs()->getChild("inFloat");
        auto script3FloatOutput = script3->getOutputs()->getChild("outFloat");
        auto script3Vec3Input   = script3->getInputs()->getChild("inVec3");
        auto script3Vec3Output  = script3->getOutputs()->getChild("outVec3");

        auto nodeBinding       = logicEngine.createRamsesNodeBinding("NodeBinding");
        auto appearanceBinding = logicEngine.createRamsesAppearanceBinding("AppearanceBinding");

        ramses::RamsesFramework ramsesFramework;
        auto                    ramsesClient = ramsesFramework.createClient("client");
        auto                    ramsesScene  = ramsesClient->createScene(ramses::sceneId_t(1));

        ramses::EffectDescription ramsesEffectDesc;
        ramsesEffectDesc.setVertexShader(vertexShaderSource.data());
        ramsesEffectDesc.setFragmentShader(fragmentShaderSource.data());
        auto ramsesEffect     = ramsesScene->createEffect(ramsesEffectDesc);
        auto ramsesAppearance = ramsesScene->createAppearance(*ramsesEffect);
        appearanceBinding->setRamsesAppearance(ramsesAppearance);

        logicEngine.update();

        EXPECT_FALSE(*nodeBinding->getInputs()->getChild("visibility")->get<bool>());
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("rotation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("scaling")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_EQ(0.0f, appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());

        logicEngine.link(*script1FloatOutput, *script2FloatInput);
        logicEngine.link(*script2FloatOutput, *script3FloatInput);
        logicEngine.link(*script1Vec3Output,  *script2Vec3Input);
        logicEngine.link(*script2Vec3Output,  *script3Vec3Input);

        logicEngine.update();

        EXPECT_FALSE(*nodeBinding->getInputs()->getChild("visibility")->get<bool>());
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("rotation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("scaling")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_EQ(0.0f, appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());

        logicEngine.link(*script3Vec3Output, *nodeBinding->getInputs()->getChild("translation"));

        script1Vec3Input->set(vec3f{1.f, 2.f, 3.f});

        logicEngine.update();

        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(1.f, 2.f, 3.f));

        logicEngine.link(*script3FloatOutput, *appearanceBinding->getInputs()->getChild("floatUniform"));

        script1FloatInput->set(42.f);

        logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());

        logicEngine.unlink(*script3Vec3Output, *nodeBinding->getInputs()->getChild("translation"));

        script1FloatInput->set(23.f);
        script1Vec3Input->set(vec3f{3.f, 2.f, 1.f});

        logicEngine.update();

        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(1.f, 2.f, 3.f));
        EXPECT_FLOAT_EQ(23.f, *appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());
    }
}
