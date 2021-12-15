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
#include "fmt/format.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses-framework-api/RamsesFramework.h"
#include "ramses-client-api/RamsesClient.h"
#include "ramses-client-api/Effect.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/EffectDescription.h"
#include "ramses-client-api/UniformInput.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/PerspectiveCamera.h"

#include <fstream>

namespace rlogic
{
    class ALuaScript_Runtime : public ALuaScript
    {
    protected:
    };

    // Not testable, because assignment to userdata can't be catched. It's just a replacement of the current value
    TEST_F(ALuaScript_Runtime, DISABLED_GeneratesErrorWhenOverwritingInputsInRunFunction)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function interface()
            end

            function run()
                IN = {}
            end
        )");

        ASSERT_EQ(nullptr, script);

        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Special global symbol 'IN' should not be overwritten with other types in run() function!!"));
    }

    TEST_F(ALuaScript_Runtime, ReportsErrorWhenAssigningVectorComponentsIndividually)
    {
        m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.vec3f = VEC3F
            end

            function run()
                OUT.vec3f[1] = 1.0
            end
        )");

        m_logicEngine.update();

        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Error while writing to 'vec3f'. Can't assign individual components of vector types, must assign the whole vector"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfUndefinedInputIsUsedInRun)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
            end
            function run()
                local undefined = IN.undefined
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Tried to access undefined struct property 'undefined'"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfUndefinedOutputIsUsedInRun)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
            end
            function run()
                OUT.undefined = 5
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Tried to access undefined struct property 'undefined'"));
    }

    TEST_F(ALuaScript_Runtime, ReportsSourceNodeOnRuntimeError)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
            end
            function run()
                error("this causes an error")
            end
        )", WithStdModules({ EStandardModule::Base }));

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("this causes an error"));
        EXPECT_EQ(script, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenTryingToWriteInputValues)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.value = FLOAT
            end

            function run()
                IN.value = 5
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());

        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("lua: error: Error while writing to 'value'. Writing input values is not allowed, only outputs!"));
        EXPECT_EQ(script, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenTryingToAccessPropertiesWithNonStringIndexAtRunTime)
    {
        const std::vector<std::string> wrongIndexTypes = {"[1]", "[true]", "[{x=5}]", "[nil]"};

        std::vector<LuaTestError> allErrorCases;
        for (const auto& errorType : wrongIndexTypes)
        {
            allErrorCases.emplace_back(LuaTestError{"IN" + errorType + " = 5", "lua: error: Bad access to property 'IN'! Expected a string but got object of type"});
            allErrorCases.emplace_back(LuaTestError{"OUT" + errorType + " = 5", "lua: error: Bad access to property 'OUT'! Expected a string but got object of type"});
        }

        for (const auto& singleCase : allErrorCases)
        {
            auto script = m_logicEngine.createLuaScript(
                "function interface()\n"
                "end\n"
                "function run()\n" +
                singleCase.errorCode +
                "\n"
                "end\n");

            ASSERT_NE(nullptr, script);
            m_logicEngine.update();

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, SetsValueOfTopLevelInputSuccessfully_WhenTemplateMatchesDeclaredInputType)
    {
        auto* script = m_logicEngine.createLuaScript(m_minimalScriptWithInputs);
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
        auto* script = m_logicEngine.createLuaScript(R"(

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
        auto* script = m_logicEngine.createLuaScript(R"(
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
        auto* script = m_logicEngine.createLuaScript(R"(
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

            auto* script = m_logicEngine.createLuaScript(scriptSource);

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_TRUE(m_logicEngine.getErrors().empty());
            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Runtime, PermitsAssigningOfVector_FromTable_WithKeyValuePairs)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
        auto* script = m_logicEngine.createLuaScript(R"(
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
        auto* script = m_logicEngine.createLuaScript(R"(
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
        auto* script = m_logicEngine.createLuaScript(R"(
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
        auto* script = m_logicEngine.createLuaScript(R"(
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
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Element size mismatch when assigning struct property 'data'! Expected: 2 Received: 1"));

        EXPECT_EQ(5, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_Overspecified)
    {
        m_logicEngine.createLuaScript(R"(
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

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Unexpected property 'not_specified' while assigning values to struct 'data'!"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_WhenFieldHasWrongType)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.data = {
                    field1 = INT32,
                    field2 = INT32
                }
                OUT.field2 = INT32
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
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Assigning string to 'INT32' output 'field2'!"));

        EXPECT_EQ(0, *field1->get<int32_t>());
        EXPECT_EQ(0, *field2->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNestedProperties_WhenNestedSubStructDoesNotMatch)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
                    nested = {
                        wrong_field = 5
                    }
                }
            end
        )");

        auto outputs = script->getOutputs();
        auto nestedfield = outputs->getChild("data")->getChild("nested")->getChild("field");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Unexpected property 'wrong_field' while assigning values to struct 'nested'"));

        // TODO Violin don't assign other fields on type mismatch - and re-enable this check that values were not updated
        //EXPECT_EQ(0, *field1->get<int32_t>());
        //EXPECT_EQ(0, *field2->get<int32_t>());
        EXPECT_EQ(0, *nestedfield->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, AssignsValuesToArrays)
    {
        std::string_view scriptWithArrays = R"(
            function interface()
                IN.array_int = ARRAY(2, INT)
                IN.array_int64 = ARRAY(2, INT64)
                IN.array_float = ARRAY(3, FLOAT)
                OUT.array_int = ARRAY(2, INT)
                OUT.array_int64 = ARRAY(2, INT64)
                OUT.array_float = ARRAY(3, FLOAT)
            end

            function run()
                OUT.array_int = IN.array_int
                OUT.array_int[2] = 5
                OUT.array_int64 = IN.array_int64
                OUT.array_int64[2] = 5
                OUT.array_float = IN.array_float
                OUT.array_float[1] = 1.5
            end
        )";

        auto* script = m_logicEngine.createLuaScript(scriptWithArrays);

        auto inputs = script->getInputs();
        auto in_array_int = inputs->getChild("array_int");
        auto in_array_int64 = inputs->getChild("array_int64");
        auto in_array_float = inputs->getChild("array_float");
        in_array_int->getChild(0)->set<int32_t>(1);
        in_array_int->getChild(1)->set<int32_t>(2);
        in_array_int64->getChild(0)->set<int64_t>(3);
        in_array_int64->getChild(1)->set<int64_t>(4);
        in_array_float->getChild(0)->set<float>(0.1f);
        in_array_float->getChild(1)->set<float>(0.2f);
        in_array_float->getChild(2)->set<float>(0.3f);

        EXPECT_TRUE(m_logicEngine.update());

        auto outputs = script->getOutputs();
        auto out_array_int = outputs->getChild("array_int");
        auto out_array_int64 = outputs->getChild("array_int64");
        auto out_array_float = outputs->getChild("array_float");

        EXPECT_EQ(1, *out_array_int->getChild(0)->get<int32_t>());
        EXPECT_EQ(5, *out_array_int->getChild(1)->get<int32_t>());

        EXPECT_EQ(3, *out_array_int64->getChild(0)->get<int64_t>());
        EXPECT_EQ(5, *out_array_int64->getChild(1)->get<int64_t>());

        EXPECT_FLOAT_EQ(1.5f, *out_array_float->getChild(0)->get<float>());
        EXPECT_FLOAT_EQ(0.2f, *out_array_float->getChild(1)->get<float>());
        EXPECT_FLOAT_EQ(0.3f, *out_array_float->getChild(2)->get<float>());
    }

    // TODO Violin refactor other tests which test 'unexpected type' to also list all invalid types like this one
    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAccessingArrayWithNonIntegerIndex)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                IN.array = ARRAY(2, INT)
                OUT.array = ARRAY(2, INT)
            end
            function run()
                {}
            end
        )");

        const std::vector<std::string> invalidStatements
        {
            "IN.array.name = 5",
            "OUT.array.name = 5",
            "IN.array[true] = 5",
            "OUT.array[true] = 5",
            "IN.array[{x=5}] = 5",
            "OUT.array[{x=5}] = 5",
            "IN.array[nil] = 5",
            "OUT.array[nil] = 5",
            "IN.array[IN] = 5",
            "OUT.array[IN] = 5",
        };

        for (const auto& invalidStatement : invalidStatements)
        {
            auto script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, invalidStatement));

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Bad access to property 'array'! Error while extracting integer: expected a number, received"));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAccessingArrayOutOfRange)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                IN.array = ARRAY(2, INT)
                OUT.array = ARRAY(2, INT)
            end
            function run()
                {}
            end
        )");

        std::vector<LuaTestError> allErrorCases;
        for (auto idx : std::initializer_list<int>{ -1, 0, 3})
        {
            std::string errorTemplate =
                (idx < 0) ?
                "Bad access to property 'array'! Error while extracting integer: expected non-negative number, received '-1'" :
                "Index out of range! Expected 0 < index <= 2 but received index == {}";

            for (const auto& prop : std::vector<std::string>{ "IN", "OUT" })
            {
                allErrorCases.emplace_back(LuaTestError{
                    fmt::format("{}.array[{}] = 5", prop, idx),
                    fmt::format(errorTemplate, idx)
                    });
            }
        }

        for (const auto& singleCase : allErrorCases)
        {
            auto script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, singleCase.errorCode));

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, AssignArrayValuesFromLuaTable)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int_array = ARRAY(2, INT)
                OUT.int64_array = ARRAY(2, INT64)
                OUT.float_array = ARRAY(2, FLOAT)
                OUT.vec2i_array = ARRAY(2, VEC2I)
                OUT.vec3f_array = ARRAY(2, VEC3F)
            end
            function run()
                OUT.int_array = {1, 2}
                OUT.int64_array = {3, 4}
                OUT.float_array = {0.1, 0.2}
                OUT.vec2i_array = {{11, 12}, {21, 22}}
                OUT.vec3f_array = {{0.11, 0.12, 0.13}, {0.21, 0.22, 0.23}}
            end
        )");

        ASSERT_NE(nullptr, script);

        EXPECT_TRUE(m_logicEngine.update());

        const auto int_array = script->getOutputs()->getChild("int_array");
        const auto int64_array = script->getOutputs()->getChild("int64_array");
        const auto float_array = script->getOutputs()->getChild("float_array");
        const auto vec2i_array = script->getOutputs()->getChild("vec2i_array");
        const auto vec3f_array = script->getOutputs()->getChild("vec3f_array");

        EXPECT_EQ(1, *int_array->getChild(0)->get<int32_t>());
        EXPECT_EQ(2, *int_array->getChild(1)->get<int32_t>());
        EXPECT_EQ(3, *int64_array->getChild(0)->get<int64_t>());
        EXPECT_EQ(4, *int64_array->getChild(1)->get<int64_t>());
        EXPECT_FLOAT_EQ(0.1f, *float_array->getChild(0)->get<float>());
        EXPECT_FLOAT_EQ(0.2f, *float_array->getChild(1)->get<float>());
        EXPECT_THAT(*vec2i_array->getChild(0)->get<vec2i>(), ::testing::ElementsAre(11, 12));
        EXPECT_THAT(*vec2i_array->getChild(1)->get<vec2i>(), ::testing::ElementsAre(21, 22));
        EXPECT_THAT(*vec3f_array->getChild(0)->get<vec3f>(), ::testing::ElementsAre(0.11f, 0.12f, 0.13f));
        EXPECT_THAT(*vec3f_array->getChild(1)->get<vec3f>(), ::testing::ElementsAre(0.21f, 0.22f, 0.23f));
    }

    TEST_F(ALuaScript_Runtime, AssignArrayValuesFromLuaTable_WithExplicitKeys)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int_array = ARRAY(3, INT)
            end
            function run()
                OUT.int_array = {[1] = 11, [2] = 12, [3] = 13}
            end
        )");

        ASSERT_NE(nullptr, script);

        EXPECT_TRUE(m_logicEngine.update());

        auto int_array = script->getOutputs()->getChild("int_array");

        EXPECT_EQ(11, *int_array->getChild(0)->get<int32_t>());
        EXPECT_EQ(12, *int_array->getChild(1)->get<int32_t>());
        EXPECT_EQ(13, *int_array->getChild(2)->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningArrayWithFewerElementsThanRequired_UsingExplicitIndices)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int_array = ARRAY(3, INT)
            end
            function run()
                OUT.int_array = {[1] = 11, [2] = 12}
            end
        )");

        ASSERT_NE(nullptr, script);

        EXPECT_FALSE(m_logicEngine.update());

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Error during assignment of array property 'int_array'! Expected a value at index 3"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningArrayFromLuaTableWithCorrectSizeButWrongIndices)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int_array = ARRAY(3, INT)
            end
            function run()
                -- 3 values, but use [1, 3, 4] instead of [1, 2, 3]
                OUT.int_array = {[1] = 11, [3] = 13, [4] = 14}
            end
        )");

        ASSERT_NE(nullptr, script);

        EXPECT_FALSE(m_logicEngine.update());

        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Error during assignment of array property 'int_array'! Expected a value at index 2"));
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningArraysWrongValues)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                OUT.array_int = ARRAY(2, INT32)
                OUT.array_int64 = ARRAY(2, INT64)
                OUT.array_string = ARRAY(2, STRING)
                OUT.array_vec2f = ARRAY(2, VEC2F)
            end
            function run()
                {}
            end
        )");

        // This is a subset of all possible permutations, but should cover most types and cases
        const std::vector<LuaTestError> allErrorCases = {
            {"OUT.array_int = {}", "Error during assignment of array property 'array_int'! Expected a value at index 1"},
            {"OUT.array_int = {1}", "Error during assignment of array property 'array_int'! Expected a value at index 2"},
            {"OUT.array_int = {1, 2, 3}", "Element size mismatch when assigning array property 'array_int'! Expected array size: 2"},
            {"OUT.array_int = {1, 2.2}", "Error while extracting integer: implicit rounding (fractional part '0.20000000000000018' is not negligible)"},
            {"OUT.array_int = {1, true}", "Assigning bool to 'INT32' output ''"},
            {"OUT.array_int = {nil, 1, 3}", "Error during assignment of array property 'array_int'! Expected a value at index 1"},
            {"OUT.array_int = {1, nil, 3}", "Error during assignment of array property 'array_int'! Expected a value at index 2"},
            {"OUT.array_int64 = {}", "Error during assignment of array property 'array_int64'! Expected a value at index 1"},
            {"OUT.array_int64 = {1}", "Error during assignment of array property 'array_int64'! Expected a value at index 2"},
            {"OUT.array_int64 = {1, 2, 3}", "Element size mismatch when assigning array property 'array_int64'! Expected array size: 2"},
            {"OUT.array_int64 = {1, 2.2}", "Error while extracting integer: implicit rounding (fractional part '0.20000000000000018' is not negligible)"},
            {"OUT.array_int64 = {1, true}", "Assigning bool to 'INT64' output ''"},
            {"OUT.array_int64 = {nil, 1, 3}", "Error during assignment of array property 'array_int64'! Expected a value at index 1"},
            {"OUT.array_int64 = {1, nil, 3}", "Error during assignment of array property 'array_int64'! Expected a value at index 2"},
            // TODO Violin the messages below are a bit misleading now ... They could contain info which array field failed to be assigned. Need to refactor the code and fix them
            {"OUT.array_string = {'somestring', 2}", "Assigning number to 'STRING' output ''"},
            {"OUT.array_string = {'somestring', {}}", "Assigning table to 'STRING' output ''"},
            {"OUT.array_string = {'somestring', OUT.array_int}", "Can't assign property 'array_int' (type ARRAY) to property '' (type STRING)"},
            {"OUT.array_vec2f = {1, 2}", "Error while assigning output VEC2 property ''. Expected a Lua table with 2 entries but got object of type number instead!"},
            {"OUT.array_vec2f = {{1, 2}, {5}}", "Error while assigning output VEC2 property ''. Error while extracting array: expected 2 array components in table but got 1 instead!"},
            {"OUT.array_vec2f = {{1, 2}, {}}", "Error while assigning output VEC2 property ''. Error while extracting array: expected 2 array components in table but got 0 instead!"},
            {"OUT.array_int = OUT", "Can't assign property 'OUT' (type STRUCT) to property 'array_int' (type ARRAY)"},
            {"OUT.array_int = IN", "Can't assign property 'IN' (type STRUCT) to property 'array_int' (type ARRAY)"},
        };

        for (const auto& singleCase : allErrorCases)
        {
            auto script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, singleCase.errorCode));

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, AssignsValuesArraysInVariousLuaSyntaxStyles)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.array = ARRAY(3, VEC2I)
                OUT.array = ARRAY(3, VEC2I)
            end
            function run()
                -- assign from "everything" towards "just one value" to cover as many cases as possible
                OUT.array = IN.array
                OUT.array[2] = IN.array[2]
                OUT.array[3] = {5, 6}
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(script->getInputs()->getChild("array")->getChild(0)->set<vec2i>({ 1, 2 }));
        EXPECT_TRUE(script->getInputs()->getChild("array")->getChild(1)->set<vec2i>({ 3, 4 }));
        EXPECT_TRUE(script->getInputs()->getChild("array")->getChild(2)->set<vec2i>({ 5, 6 }));
        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_THAT(*script->getOutputs()->getChild("array")->getChild(0)->get<vec2i>(), ::testing::ElementsAre(1, 2));
        EXPECT_THAT(*script->getOutputs()->getChild("array")->getChild(1)->get<vec2i>(), ::testing::ElementsAre(3, 4));
        EXPECT_THAT(*script->getOutputs()->getChild("array")->getChild(2)->get<vec2i>(), ::testing::ElementsAre(5, 6));
    }

    TEST_F(ALuaScript_Runtime, AssignsValuesArraysInVariousLuaSyntaxStyles_InNestedStruct)
    {
        auto script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.struct = {
                    array1  = ARRAY(1, VEC2F),
                    array2  = ARRAY(2, VEC3F)
                }
                OUT.struct = {
                    array1  = ARRAY(1, VEC2F),
                    array2  = ARRAY(2, VEC3F)
                }
            end
            function run()
                -- assign from "everything" towards "just one value" to cover as many cases as possible
                OUT.struct = IN.struct
                OUT.struct.array1    = IN.struct.array1
                OUT.struct.array2[1]    = {1.1, 1.2, 1.3}
                OUT.struct.array2[2]    = IN.struct.array2[2]
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(script->getInputs()->getChild("struct")->getChild("array1")->getChild(0)->set<vec2f>({ 0.1f, 0.2f }));
        EXPECT_TRUE(script->getInputs()->getChild("struct")->getChild("array2")->getChild(0)->set<vec3f>({ 1.1f, 1.2f, 1.3f }));
        EXPECT_TRUE(script->getInputs()->getChild("struct")->getChild("array2")->getChild(1)->set<vec3f>({ 2.1f, 2.2f, 2.3f }));
        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_THAT(*script->getOutputs()->getChild("struct")->getChild("array1")->getChild(0)->get<vec2f>(), ::testing::ElementsAre(0.1f, 0.2f));
        EXPECT_THAT(*script->getOutputs()->getChild("struct")->getChild("array2")->getChild(0)->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
        EXPECT_THAT(*script->getOutputs()->getChild("struct")->getChild("array2")->getChild(1)->get<vec3f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f));
    }

    TEST_F(ALuaScript_Runtime, AllowsAssigningArraysFromTableWithNilAtTheEnd)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                OUT.array_2ints = ARRAY(2, INT)
                OUT.array_3ints = ARRAY(3, INT)
                OUT.array_4ints = ARRAY(4, INT)
                OUT.array_vec2i = ARRAY(1, VEC2I)
            end

            function run()
                {}
            end
        )");

        // Lua+sol seem to not iterate over nil entries when creating a table
        // Still, we test the behavior explicitly
        const std::vector<std::string> allCases =
        {
            "OUT.array_2ints = {1, 2, nil} -- single nil",
            "OUT.array_2ints = {1, 2, nil, nil} -- two nils",
            "OUT.array_3ints = {1, 2, 3, nil}",
            "OUT.array_4ints = {1, 2, 3, 4, nil}",
            "OUT.array_vec2i = {{1, 2}, nil}",
        };

        for (const auto& aCase : allCases)
        {
            auto* script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, aCase));

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(m_logicEngine.update());

            EXPECT_TRUE(m_logicEngine.getErrors().empty());
            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Runtime, ReportsErrorWhenAssigningArraysWithMismatchedSizes)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                IN.array_float2 = ARRAY(2, FLOAT)
                IN.array_float4 = ARRAY(4, FLOAT)
                IN.array_vec3f = ARRAY(1, VEC3F)
                OUT.array_float3 = ARRAY(3, FLOAT)
            end

            function run()
                {}
            end
        )");

        const std::vector<LuaTestError> allCases =
        {
            {"OUT.array_float3 = IN.array_float2", "Can't assign property 'array_float2' (#fields=2) to property 'array_float3' (#fields=3)"},
            {"OUT.array_float3 = IN.array_float4", "Can't assign property 'array_float4' (#fields=4) to property 'array_float3' (#fields=3)!"},
            {"OUT.array_float3 = IN.array_vec3f", "Can't assign property 'array_vec3f' (#fields=1) to property 'array_float3' (#fields=3)"},
            {"OUT.array_float3 = {0.1, 0.2}", "Error during assignment of array property 'array_float3'! Expected a value at index 3"},
            {"OUT.array_float3 = {0.1, 0.2, 0.3, 0.4}", "Element size mismatch when assigning array property 'array_float3'! Expected array size: 3"},
            {"OUT.array_float3 = {}", "Error during assignment of array property 'array_float3'! Expected a value at index 1"},
        };

        for (const auto& aCase : allCases)
        {
            auto* script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, aCase.errorCode));

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(aCase.expectedErrorMessage));
            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Runtime, ReportsErrorWhenAssigningUserdataArraysWithMismatchedTypes)
    {
        const std::string_view scriptTemplate = (R"(
            function interface()
                IN.array_float = ARRAY(2, FLOAT)
                IN.array_vec2f = ARRAY(2, VEC2F)
                IN.array_vec2i = ARRAY(2, VEC2I)
                OUT.array_int = ARRAY(2, INT32)
                OUT.array_int64 = ARRAY(2, INT64)
            end

            function run()
                {}
            end
        )");

        const std::vector<LuaTestError> allCases =
        {
            {"OUT.array_int = IN.array_float", "Can't assign property '' (type FLOAT) to property '' (type INT32)!"},
            {"OUT.array_int = IN.array_vec2f", "Can't assign property '' (type VEC2F) to property '' (type INT32)!"},
            {"OUT.array_int = IN.array_vec2i", "Can't assign property '' (type VEC2I) to property '' (type INT32)!"},
            {"OUT.array_int64 = IN.array_float", "Can't assign property '' (type FLOAT) to property '' (type INT64)!"},
            {"OUT.array_int64 = IN.array_vec2f", "Can't assign property '' (type VEC2F) to property '' (type INT64)!"},
            {"OUT.array_int64 = IN.array_vec2i", "Can't assign property '' (type VEC2I) to property '' (type INT64)!"},
        };

        for (const auto& aCase : allCases)
        {
            auto* script = m_logicEngine.createLuaScript(fmt::format(scriptTemplate, aCase.errorCode));

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(aCase.expectedErrorMessage));
            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenImplicitlyRoundingNumbers)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.float1 = FLOAT
                IN.float2 = FLOAT
                OUT.int = INT
                OUT.int64 = INT64
            end
            function run()
                OUT.int = IN.float1
                OUT.int64 = IN.float2
            end
        )");

        auto float1Input = script->getInputs()->getChild("float1");
        auto float2Input = script->getInputs()->getChild("float2");
        auto intOutput = script->getOutputs()->getChild("int");
        auto int64Output = script->getOutputs()->getChild("int64");

        float1Input->set<float>(1.0f);
        float2Input->set<float>(1.0f);

        EXPECT_TRUE(m_logicEngine.update());
        ASSERT_TRUE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(1, *intOutput->get<int32_t>());
        EXPECT_EQ(1, *int64Output->get<int64_t>());

        float1Input->set<float>(2.5f);
        float2Input->set<float>(1.f);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message,
            ::testing::HasSubstr("Error during assignment of property 'int'! Error while extracting integer: implicit rounding (fractional part '0.5' is not negligible)"));
        EXPECT_EQ(1, *intOutput->get<int32_t>());
        EXPECT_EQ(1, *int64Output->get<int64_t>());

        float1Input->set<float>(1.f);
        float2Input->set<float>(2.5f);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message,
            ::testing::HasSubstr("Error during assignment of property 'int64'! Error while extracting integer: implicit rounding (fractional part '0.5' is not negligible)"));
        EXPECT_EQ(1, *intOutput->get<int32_t>());
        EXPECT_EQ(1, *int64Output->get<int64_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNilToIntOutputs)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int = INT32
            end
            function run()
                OUT.int = nil
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Assigning nil to 'INT32' output 'int'!"));
        EXPECT_EQ(0, *script->getOutputs()->getChild("int")->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningBoolToIntOutputs)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.int = INT32
            end
            function run()
                OUT.int = true
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Assigning bool to 'INT32' output 'int'!"));
        EXPECT_EQ(0, *script->getOutputs()->getChild("int")->get<int32_t>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningBoolToStringOutputs)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Assigning bool to 'STRING' output 'str'!"));
        EXPECT_EQ("this is quite ok", *script->getOutputs()->getChild("str")->get<std::string>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorWhenAssigningNumberToStringOutputs)
    {
        m_logicEngine.createLuaScript(R"(
            function interface()
                OUT.str = STRING
            end
            function run()
                OUT.str = 42   -- this is not ok
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Assigning number to 'STRING' output 'str'!"));
    }

    TEST_F(ALuaScript_Runtime, SupportsMultipleLevelsOfNestedInputs_confidenceTest)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
            "Bad access to property 'IN'! Expected a string but got object of type number instead!"},
            {"var = IN[true]",
            "Bad access to property 'IN'! Expected a string but got object of type bool instead!"},
            {"var = IN[{x = 5}]",
            "Bad access to property 'IN'! Expected a string but got object of type table instead!"},
            {"OUT[0] = 5",
            "Bad access to property 'OUT'! Expected a string but got object of type number instead!"},
            {"OUT[true] = 5",
            "Bad access to property 'OUT'! Expected a string but got object of type bool instead!"},
            {"OUT[{x = 5}] = 5",
            "Bad access to property 'OUT'! Expected a string but got object of type table instead!"},
        };

        for (const auto& singleCase : allCases)
        {
            auto script = m_logicEngine.createLuaScript(
                "function interface()\n"
                "end\n"
                "function run()\n" +
                singleCase.errorCode + "\n"
                "end\n"
            );

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());
            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(singleCase.expectedErrorMessage));
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
            auto script = m_logicEngine.createLuaScript(
                "function interface()\n"
                "end\n"
                "function run()\n" +
                singleCase.errorCode + "\n"
                "end\n"
            );

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());
            ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(singleCase.expectedErrorMessage));
            m_logicEngine.destroy(*script);
        }
    }

    TEST_F(ALuaScript_Runtime, AssignsValuesToArraysWithStructs)
    {
        std::string_view scriptWithArrays = R"(
            function interface()
                IN.array_structs = ARRAY(2, {name = STRING, age = INT})
                OUT.array_structs = ARRAY(2, {name = STRING, age = INT})
            end

            function run()
                OUT.array_structs = IN.array_structs
                OUT.array_structs[2] = {name = "joe", age = 99}
                OUT.array_structs[2].age = 78
            end
        )";

        auto* script = m_logicEngine.createLuaScript(scriptWithArrays);

        auto inputs = script->getInputs();
        auto IN_array = inputs->getChild("array_structs");
        IN_array->getChild(0)->getChild("name")->set<std::string>("donald");

        EXPECT_TRUE(m_logicEngine.update());

        auto outputs = script->getOutputs();
        auto OUT_array = outputs->getChild("array_structs");

        EXPECT_EQ("donald", *OUT_array->getChild(0)->getChild("name")->get<std::string>());
        EXPECT_EQ("joe", *OUT_array->getChild(1)->getChild("name")->get<std::string>());
        EXPECT_EQ(78, *OUT_array->getChild(1)->getChild("age")->get<int32_t>());
    }

    // This is truly evil, too! Perhaps more so than the previous test
    // I think this is not catchable, because it's just a normal function call
    TEST_F(ALuaScript_Runtime, DISABLED_ForbidsCallingInterfaceFunctionInsideTheRunFunction)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Not allowed to call interface() function inside run() function!");
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
    }


    TEST_F(ALuaScript_Runtime, AbortsAfterFirstRuntimeError)
    {
        auto script = m_logicEngine.createLuaScript(R"(
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
        auto script = m_logicEngine.createLuaScript(R"(
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
                        vec4i  = VEC4I,
                        array  = ARRAY(2, VEC2I)
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
                        vec4i  = VEC4I,
                        array  = ARRAY(2, VEC2I)
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
                    OUT.struct.struct.array    = {{0, 0}, {0, 0}}
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
                    OUT.struct.struct.array[1]  = IN.struct.struct.array[1]
                    OUT.struct.struct.array[2]  = IN.struct.struct.array[2]
                elseif IN.assignmentType == "assign_constants" then
                    OUT.float = 0.1
                    OUT.int   = 1
                    OUT.struct.float = 0.2
                    OUT.struct.int   = 2
                    OUT.struct.struct.float     = 0.3
                    OUT.struct.struct.int       = 3
                    OUT.struct.struct.bool      = true
                    OUT.struct.struct.string    = "somestring"
                    OUT.struct.struct.vec2f     = { 0.1, 0.2 }
                    OUT.struct.struct.vec3f     = { 1.1, 1.2, 1.3 }
                    OUT.struct.struct.vec4f     = { 2.1, 2.2, 2.3, 2.4 }
                    OUT.struct.struct.vec2i     = { 1, 2 }
                    OUT.struct.struct.vec3i     = { 3, 4, 5 }
                    OUT.struct.struct.vec4i     = { 6, 7, 8, 9 }
                    OUT.struct.struct.array     = { {11, 12}, {13, 14} }
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
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("array")->getChild(0)->set<vec2i>({ 11, 12 });
        script->getInputs()->getChild("struct")->getChild("struct")->getChild("array")->getChild(1)->set<vec2i>({ 13, 14 });

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
            EXPECT_THAT(*struct_lvl2->getChild("array")->getChild(0)->get<vec2i>(), ::testing::ElementsAre(11, 12));
            EXPECT_THAT(*struct_lvl2->getChild("array")->getChild(1)->get<vec2i>(), ::testing::ElementsAre(13, 14));
        }
    }

    // This is truly evil! But Lua is a script language, so... Lots of possibilities! :D
    // I think this is not catchable, becuse "run" is a function and not a userdata
    // Therefore it is not catchable in c++
    TEST_F(ALuaScript_Runtime, DISABLED_ForbidsOverwritingRunFunctionInsideTheRunFunction)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
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
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Not allowed to overwrite run() function inside of itself!");
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
        EXPECT_FALSE(m_logicEngine.update());
        EXPECT_EQ("They look right... ...and you...", *script->getOutputs()->getChild("str")->get<std::string>());
    }

    TEST_F(ALuaScript_Runtime, ProducesErrorIfInvalidOutPropertyIsAccessed)
    {
        auto scriptWithInvalidOutParam = m_logicEngine.createLuaScript(R"(
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
        auto scriptWithInvalidStructAccess = m_logicEngine.createLuaScript(R"(
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
        auto scriptWithValidStructButInvalidField = m_logicEngine.createLuaScript(R"(
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
        auto script = m_logicEngine.createLuaScript(R"(
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
        auto script = m_logicEngine.createLuaScript(R"(
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
        auto script = m_logicEngine.createLuaScript(R"(
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

        auto script1 = m_logicEngine.createLuaScript(scriptSource);
        auto script2 = m_logicEngine.createLuaScript(scriptSource);
        auto script3 = m_logicEngine.createLuaScript(scriptSource);

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

        ramses::RamsesFramework ramsesFramework;
        auto                    ramsesClient = ramsesFramework.createClient("client");
        auto                    ramsesScene  = ramsesClient->createScene(ramses::sceneId_t(1));

        ramses::EffectDescription ramsesEffectDesc;
        ramsesEffectDesc.setVertexShader(vertexShaderSource.data());
        ramsesEffectDesc.setFragmentShader(fragmentShaderSource.data());
        auto ramsesEffect     = ramsesScene->createEffect(ramsesEffectDesc);
        auto ramsesAppearance = ramsesScene->createAppearance(*ramsesEffect);
        ramses::PerspectiveCamera* camera = ramsesScene->createPerspectiveCamera();

        auto nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        auto appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*ramsesAppearance, "AppearanceBinding");
        auto cameraBinding = m_logicEngine.createRamsesCameraBinding(*camera, "CameraBinding");

        m_logicEngine.update();

        EXPECT_TRUE(*nodeBinding->getInputs()->getChild("visibility")->get<bool>());
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("rotation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("scaling")->get<vec3f>(), ::testing::ElementsAre(1.f, 1.f, 1.f));
        EXPECT_EQ(0.0f, appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());
        EXPECT_EQ(camera->getViewportX(), 0);
        EXPECT_EQ(camera->getViewportY(), 0);
        EXPECT_EQ(camera->getViewportWidth(), 16u);
        EXPECT_EQ(camera->getViewportHeight(), 16u);
        EXPECT_NEAR(camera->getVerticalFieldOfView(), 168.579f, 0.001f);
        EXPECT_EQ(camera->getAspectRatio(), 1.f);
        EXPECT_EQ(camera->getNearPlane(), 0.1f);
        EXPECT_EQ(camera->getFarPlane(), 1.f);

        m_logicEngine.link(*script1FloatOutput, *script2FloatInput);
        m_logicEngine.link(*script2FloatOutput, *script3FloatInput);
        m_logicEngine.link(*script1Vec3Output,  *script2Vec3Input);
        m_logicEngine.link(*script2Vec3Output,  *script3Vec3Input);

        m_logicEngine.update();

        EXPECT_TRUE(*nodeBinding->getInputs()->getChild("visibility")->get<bool>());
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("rotation")->get<vec3f>(), ::testing::ElementsAre(0.f, 0.f, 0.f));
        EXPECT_THAT(*nodeBinding->getInputs()->getChild("scaling")->get<vec3f>(), ::testing::ElementsAre(1.f, 1.f, 1.f));
        EXPECT_EQ(0.0f, appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());
        EXPECT_EQ(camera->getViewportX(), 0);
        EXPECT_EQ(camera->getViewportY(), 0);
        EXPECT_EQ(camera->getViewportWidth(), 16u);
        EXPECT_EQ(camera->getViewportHeight(), 16u);
        EXPECT_NEAR(camera->getVerticalFieldOfView(), 168.579f, 0.001f);
        EXPECT_EQ(camera->getAspectRatio(), 1.f);
        EXPECT_EQ(camera->getNearPlane(), 0.1f);
        EXPECT_EQ(camera->getFarPlane(), 1.f);

        m_logicEngine.link(*script3Vec3Output, *nodeBinding->getInputs()->getChild("translation"));

        script1Vec3Input->set(vec3f{1.f, 2.f, 3.f});

        m_logicEngine.update();

        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(1.f, 2.f, 3.f));

        m_logicEngine.link(*script3FloatOutput, *appearanceBinding->getInputs()->getChild("floatUniform"));
        m_logicEngine.link(*script3FloatOutput, *cameraBinding->getInputs()->getChild("frustum")->getChild("farPlane"));

        script1FloatInput->set(42.f);

        m_logicEngine.update();

        EXPECT_FLOAT_EQ(42.f, *appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());
        EXPECT_EQ(42.f, *cameraBinding->getInputs()->getChild("frustum")->getChild("farPlane")->get<float>());

        m_logicEngine.unlink(*script3Vec3Output, *nodeBinding->getInputs()->getChild("translation"));

        script1FloatInput->set(23.f);
        script1Vec3Input->set(vec3f{3.f, 2.f, 1.f});

        m_logicEngine.update();

        EXPECT_THAT(*nodeBinding->getInputs()->getChild("translation")->get<vec3f>(), ::testing::ElementsAre(1.f, 2.f, 3.f));
        EXPECT_FLOAT_EQ(23.f, *appearanceBinding->getInputs()->getChild("floatUniform")->get<float>());
        EXPECT_EQ(23.f, *cameraBinding->getInputs()->getChild("frustum")->getChild("farPlane")->get<float>());
    }


    TEST_F(ALuaScript_Runtime, IncludesStandardLibraries_WhenConfiguredWithThem)
    {
        const std::string_view scriptSrc = R"(
            function debug_func(arg)
                print(arg)
            end

            function interface()
                OUT.floored_float = INT
                OUT.string_gsub = STRING
                OUT.table_maxn = INT
                OUT.language_of_debug_func = STRING
            end
            function run()
                -- test math lib
                OUT.floored_float = math.floor(42.7)
                -- test string lib
                OUT.string_gsub = string.gsub("This is the text", "the text", "the modified text")
                -- test table lib
                OUT.table_maxn = table.maxn ({11, 12, 13})
                -- test debug lib
                local debuginfo = debug.getinfo (debug_func)
                OUT.language_of_debug_func = debuginfo.what
            end
        )";
        auto script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({EStandardModule::Base, EStandardModule::String, EStandardModule::Table, EStandardModule::Debug, EStandardModule::Math}));
        ASSERT_NE(nullptr, script);

        m_logicEngine.update();

        EXPECT_EQ(42, *script->getOutputs()->getChild("floored_float")->get<int32_t>());
        EXPECT_EQ("This is the modified text", *script->getOutputs()->getChild("string_gsub")->get<std::string>());
        EXPECT_EQ(3, *script->getOutputs()->getChild("table_maxn")->get<int32_t>());
        EXPECT_EQ("Lua", *script->getOutputs()->getChild("language_of_debug_func")->get<std::string>());
    }

    class ALuaScript_RuntimeIterators : public ALuaScript_Runtime
    {
    protected:
    };

    TEST_F(ALuaScript_RuntimeIterators, ComputesSizeOfCustomPropertiesUsingCustomLengthFunction)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.array_int = ARRAY(2, INT)
                OUT.struct = {a=INT, b={c = INT}}
                OUT.array_struct = ARRAY(3, {a=INT, b=FLOAT})
            end

            function run()
                if rl_len(IN) ~= 1 then
                    error("Wrong IN size!")
                end

                if rl_len(IN.array_int) ~= 2 then
                    error("Wrong array size!")
                end

                if rl_len(OUT) ~= 2 then
                    error("Wrong OUT size!")
                end

                if rl_len(OUT.struct) ~= 2 then
                    error("Wrong struct size!")
                end

                if rl_len(OUT.struct.b) ~= 1 then
                    error("Wrong nested struct size!")
                end

                if rl_len(OUT.array_struct) ~= 3 then
                    error("Wrong array struct size!")
                end

                if rl_len(OUT.array_struct[1]) ~= 2 then
                    error("Wrong array struct element size!")
                end
            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({EStandardModule::Base}));
        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, CallingCustomLengthFunctionOnNormalLuaTables_YieldsSameResultAsBuiltInSizeOperator)
    {
        std::string_view scriptSrc = R"(
            function interface()
            end

            function run()
                local emptyTable = {}
                assert(rl_len(emptyTable) == #emptyTable)
                local numericTable = {1, 2, 3}
                assert(rl_len(numericTable) == #numericTable)
                local nonNumericTable = {a=5, b=6}
                assert(rl_len(nonNumericTable) == #nonNumericTable)
                local nonNumericTable = {a=5, b=6}
                assert(rl_len(nonNumericTable) == #nonNumericTable)
            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base }));
        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, CustomRlNextFunctionWorksLikeItsBuiltInCounterpart_Structs)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.struct = {a = INT, b = INT}
                IN.nested = {
                    struct = {a = INT, b = INT}
                }
                OUT.struct = {a = INT, b = INT}
                OUT.nested = {
                    struct = {a = INT, b = INT}
                }
            end

            function run()
                -- propagate data to OUT so that we can test both further down
                OUT.struct = IN.struct
                OUT.nested = IN.nested

                local objectsToCheck = {IN.struct, IN.nested.struct, OUT.struct, OUT.nested.struct}

                for unused, container in pairs(objectsToCheck) do
                    ---- no index specified is the same as providing nil (see below)
                    k, v = rl_next(container)
                    assert(k == 'a')
                    assert(v == 11)
                    -- index=nil -> yields first element of container and its index
                    k, v = rl_next(container, nil)
                    assert(k == 'a')
                    assert(v == 11)
                    -- index==N -> yields element N+1 and its index
                    k, v = rl_next(container, 'a')
                    assert(k == 'b')
                    assert(v == 12)
                    k, v = rl_next(container, 'b')
                    assert(k == nil)
                    assert(v == nil)
                end
            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base, EStandardModule::String }));
        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("struct")->getChild("a")->set<int32_t>(11);
        script->getInputs()->getChild("struct")->getChild("b")->set<int32_t>(12);
        script->getInputs()->getChild("nested")->getChild("struct")->getChild("a")->set<int32_t>(11);
        script->getInputs()->getChild("nested")->getChild("struct")->getChild("b")->set<int32_t>(12);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, CustomRlNextFunctionWorksLikeItsBuiltInCounterpart_Arrays)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.array_int = ARRAY(2, INT)
                IN.nested = {
                    array_int = ARRAY(2, INT)
                }
                OUT.array_int = ARRAY(2, INT)
                OUT.nested = {
                    array_int = ARRAY(2, INT)
                }
            end

            function run()
                -- propagate data to OUT so that we can test both further down
                OUT.array_int = IN.array_int
                OUT.nested = IN.nested

                local objectsToCheck = {IN.array_int, IN.nested.array_int, OUT.array_int, OUT.nested.array_int}

                for k, container in pairs(objectsToCheck) do
                    -- no index specified is the same as providing nil (see below)
                    a, b = rl_next(container)
                    assert(a == 1)
                    assert(b == 11)
                    -- index=nil -> yields first element of container and its index
                    a, b = rl_next(container, nil)
                    assert(a == 1)
                    assert(b == 11)
                    -- index==N -> yields element N+1 and its index
                    a, b = rl_next(container, 1)
                    assert(a == 2)
                    assert(b == 12)
                    a, b = rl_next(container, 2)
                    assert(a == nil)
                    assert(b == nil)
                end
            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base, EStandardModule::String }));
        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("array_int")->getChild(1)->set<int32_t>(12);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(1)->set<int32_t>(12);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, Custom_IPairs_BehavesTheSameAsStandard_IPairs_Function_ForArrays)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.array_int = ARRAY(2, INT)
                IN.nested = {
                    array_int = ARRAY(2, INT)
                }
                OUT.array_int = ARRAY(2, INT)
                OUT.nested = {
                    array_int = ARRAY(2, INT)
                }
            end

            function run()
                -- propagate data to OUT so that we can test both further down
                OUT.array_int = IN.array_int
                OUT.nested = IN.nested

                -- compare iteration results to a static reference table
                local refTable = {[1] = 11, [2] = 12}

                -- test multiple containers (which all have the same contents)
                local objectsToCheck = {IN.array_int, IN.nested.array_int, OUT.array_int, OUT.nested.array_int}
                for k, container in pairs(objectsToCheck) do
                    -- iterate manually over reference table...
                    local refKey = 1
                    for key, value in rl_ipairs(container) do
                        if type(key) ~= 'number' then
                            error('Key should be of type number!')
                        end

                        if key ~= refKey then
                            error("Expected key==refKey, but found " .. tostring(key) .. " != " .. tostring(refKey))
                        end

                        local refValue = refTable[refKey]
                        if value ~= refValue then
                            error("Expected value==refValue, but found " .. tostring(value) .. " != " .. tostring(refValue))
                        end
                        -- progress refTable manually
                        refKey = refKey + 1
                    end

                    -- make sure there were exactly as many elements in refTable by checking no element is left to iterate
                    assert(refKey == 3)
                    assert(refValue == nil)
                end

            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base, EStandardModule::String }));
        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("array_int")->getChild(1)->set<int32_t>(12);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(1)->set<int32_t>(12);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, Custom_Pairs_BehavesTheSameAsStandard_Pairs_Function_ForArrays)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.array_int = ARRAY(2, INT)
                IN.nested = {
                    array_int = ARRAY(2, INT)
                }
                OUT.array_int = ARRAY(2, INT)
                OUT.nested = {
                    array_int = ARRAY(2, INT)
                }
            end

            function run()
                -- propagate data to OUT so that we can test both further down
                OUT.array_int = IN.array_int
                OUT.nested = IN.nested

                -- compare iteration results to a static reference table
                local refTable = {[1] = 11, [2] = 12}

                -- test multiple containers (which all have the same contents)
                local objectsToCheck = {IN.array_int, IN.nested.array_int, OUT.array_int, OUT.nested.array_int}
                for k, container in pairs(objectsToCheck) do
                    -- iterate manually over reference table...
                    local refKey,refValue = next(refTable)
                    -- ...and compare to rl_pairs results
                    for key, value in rl_pairs(container) do
                        if type(key) ~= 'number' then
                            error('Key should be of type number!')
                        end

                        if key ~= refKey then
                            error("Expected key==refKey, but found " .. tostring(key) .. " != " .. tostring(refKey))
                        end
                        if value ~= refValue then
                            error("Expected value==refValue, but found " .. tostring(value) .. " != " .. tostring(refValue))
                        end
                        -- progress refTable manually
                        refKey,refValue = next(refTable, refKey)
                    end

                    -- make sure there were exactly as many elements in refTable by checking no element is left to iterate
                    assert(refKey == nil)
                    assert(refValue == nil)
                end

            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base, EStandardModule::String }));
        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("array_int")->getChild(1)->set<int32_t>(12);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(0)->set<int32_t>(11);
        script->getInputs()->getChild("nested")->getChild("array_int")->getChild(1)->set<int32_t>(12);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_RuntimeIterators, Custom_Pairs_BehavesTheSameAsStandard_Pairs_Function_ForStructs)
    {
        std::string_view scriptSrc = R"(
            function interface()
                IN.int = INT
                IN.bool = BOOL
                IN.nested = {
                    int = INT,
                    bool = BOOL,
                    nested = {
                        notUsed = FLOAT
                    }
                }
                OUT.int = INT
                OUT.bool = BOOL
                OUT.nested = {
                    int = INT,
                    bool = BOOL,
                    nested = {
                        notUsed = FLOAT
                    }
                }
            end

            function run()
                -- propagate data to OUT so that we can test both further down
                OUT.int = IN.int
                OUT.bool = IN.bool
                OUT.nested = IN.nested

                -- compare iteration results to a static reference table
                local refTable = {int = 42, bool = false, nested = {int = 42, bool = false, nested = {}}}

                -- test multiple containers (which all have the same contents)
                local objectsToCheck = {IN, IN.nested, OUT, OUT.nested}
                for k, container in pairs(objectsToCheck) do
                    -- iterate manually over reference table...
                    local refKey,refValue = next(refTable)
                    -- ...and compare to rl_pairs results
                    for key, value in rl_pairs(container) do
                        if type(key) ~= 'string' then
                            error('Key should be of type string!')
                        end

                        if key ~= refKey then
                            error("Expected key==refKey, but found " .. tostring(key) .. " != " .. tostring(refKey))
                        end
                        -- compare all values except 'nested', because no value comparison semantics for tables/userdata
                        if key ~= "nested" and value ~= refValue then
                            error("Expected value==refValue, but found " .. tostring(value) .. " != " .. tostring(refValue))
                        end
                        -- progress refTable manually
                        refKey,refValue = next(refTable, refKey)
                    end

                    -- make sure there are no leftover elements in refTable
                    assert(refKey == nil)
                    assert(refValue == nil)
                end

            end
        )";
        auto* script = m_logicEngine.createLuaScript(scriptSrc, WithStdModules({ EStandardModule::Base, EStandardModule::String }));
        ASSERT_NE(nullptr, script);
        script->getInputs()->getChild("int")->set<int32_t>(42);
        script->getInputs()->getChild("bool")->set<bool>(false);
        script->getInputs()->getChild("nested")->getChild("int")->set<int32_t>(42);
        script->getInputs()->getChild("nested")->getChild("bool")->set<bool>(false);
        EXPECT_TRUE(m_logicEngine.update());
    }
}
