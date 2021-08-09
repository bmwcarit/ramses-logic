//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LuaScriptTest_Base.h"

#include "ramses-logic/Property.h"
#include "impl/PropertyImpl.h"
#include "LogTestUtils.h"


namespace rlogic
{
    class ALuaScript_Interface : public ALuaScript
    {
    protected:
        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
    };

    // Not testable, because assignment to userdata can't be catched. It's just a replacement of the current value
    TEST_F(ALuaScript_Interface, DISABLED_GeneratesErrorWhenOverwritingInputsInInterfaceFunction)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN = {}
            end

            function run()
            end
        )");

        ASSERT_EQ(nullptr, script);

        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Special global symbol 'IN' should not be overwritten with other types in interface() function!!");
    }

    TEST_F(ALuaScript_Interface, GlobalSymbolsNotAvailable)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            globalVar = "not visible"

            function interface()
                if globalVar == nil then
                    error("globalVar is not available!")
                end
            end

            function run()
            end
        )", "noGlobalSymbols");

        ASSERT_EQ(nullptr, script);
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("globalVar is not available!"));
    }

    TEST_F(ALuaScript_Interface, ProducesErrorsIfARuntimeErrorOccursInInterface)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                error("emits error")
            end

            function run()
            end
        )", "errorInInterface");

        ASSERT_EQ(nullptr, script);
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message,
            "[errorInInterface] Error while loading script. Lua stack trace:\n"
            "[string \"errorInInterface\"]:3: emits error\n"
            "stack traceback:\n"
            "\t[C]: in function 'error'\n"
            "\t[string \"errorInInterface\"]:3: in function <[string \"errorInInterface\"]:2>");
    }

    TEST_F(ALuaScript_Interface, ReturnsItsTopLevelOutputsByIndex_OrderedLexicographically)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithOutputs);

        auto outputs = script->getOutputs();

        ASSERT_EQ(10u, outputs->getChildCount());
        EXPECT_EQ("enabled", outputs->getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Bool, outputs->getChild(0)->getType());
        EXPECT_EQ("name", outputs->getChild(1)->getName());
        EXPECT_EQ(EPropertyType::String, outputs->getChild(1)->getType());
        EXPECT_EQ("speed", outputs->getChild(2)->getName());
        EXPECT_EQ(EPropertyType::Int32, outputs->getChild(2)->getType());
        EXPECT_EQ("temp", outputs->getChild(3)->getName());
        EXPECT_EQ(EPropertyType::Float, outputs->getChild(3)->getType());

        // Vec2/3/4 f/i
        EXPECT_EQ("vec2f", outputs->getChild(4)->getName());
        EXPECT_EQ(EPropertyType::Vec2f, outputs->getChild(4)->getType());
        EXPECT_EQ("vec2i", outputs->getChild(5)->getName());
        EXPECT_EQ(EPropertyType::Vec2i, outputs->getChild(5)->getType());
        EXPECT_EQ("vec3f", outputs->getChild(6)->getName());
        EXPECT_EQ(EPropertyType::Vec3f, outputs->getChild(6)->getType());
        EXPECT_EQ("vec3i", outputs->getChild(7)->getName());
        EXPECT_EQ(EPropertyType::Vec3i, outputs->getChild(7)->getType());
        EXPECT_EQ("vec4f", outputs->getChild(8)->getName());
        EXPECT_EQ(EPropertyType::Vec4f, outputs->getChild(8)->getType());
        EXPECT_EQ("vec4i", outputs->getChild(9)->getName());
        EXPECT_EQ(EPropertyType::Vec4i, outputs->getChild(9)->getType());
    }

    TEST_F(ALuaScript_Interface, ReturnsNestedOutputsByIndex_OrderedLexicographically_whenDeclaredOneByOne)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.struct = {}
                OUT.struct.field3 = {}
                OUT.struct.field2 = FLOAT
                OUT.struct.field1 = INT
                OUT.struct.field3.subfield2 = FLOAT
                OUT.struct.field3.subfield1 = INT
            end

            function run()
            end
        )");

        ASSERT_NE(nullptr, script);

        auto outputs = script->getOutputs();
        ASSERT_EQ(1u, outputs->getChildCount());
        auto structField = outputs->getChild(0);
        EXPECT_EQ("struct", structField->getName());
        EXPECT_EQ(EPropertyType::Struct, structField->getType());

        ASSERT_EQ(3u, structField->getChildCount());
        auto field1 = structField->getChild(0);
        auto field2 = structField->getChild(1);
        auto field3 = structField->getChild(2);

        EXPECT_EQ("field1", field1->getName());
        EXPECT_EQ(EPropertyType::Int32, field1->getType());
        EXPECT_EQ("field2", field2->getName());
        EXPECT_EQ(EPropertyType::Float, field2->getType());
        EXPECT_EQ("field3", field3->getName());
        EXPECT_EQ(EPropertyType::Struct, field3->getType());

        ASSERT_EQ(2u, field3->getChildCount());
        auto subfield1 = field3->getChild(0);
        auto subfield2 = field3->getChild(1);

        EXPECT_EQ("subfield1", subfield1->getName());
        EXPECT_EQ(EPropertyType::Int32, subfield1->getType());
        EXPECT_EQ("subfield2", subfield2->getName());
        EXPECT_EQ(EPropertyType::Float, subfield2->getType());
    }

    TEST_F(ALuaScript_Interface, CanDeclarePropertiesProgramatically)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                OUT.root = {}
                local lastStruct = OUT.root
                for i=1,2 do
                    lastStruct["sub" .. tostring(i)] = {}
                    lastStruct = lastStruct["sub" .. tostring(i)]
                end
            end

            function run()
            end
        )");

        ASSERT_NE(nullptr, script);

        auto outputs = script->getOutputs();

        ASSERT_EQ(1u, outputs->getChildCount());
        auto root = outputs->getChild(0);
        EXPECT_EQ("root", root->getName());
        EXPECT_EQ(EPropertyType::Struct, root->getType());

        ASSERT_EQ(1u, root->getChildCount());
        auto sub1 = root->getChild(0);

        EXPECT_EQ("sub1", sub1->getName());
        EXPECT_EQ(EPropertyType::Struct, sub1->getType());

        ASSERT_EQ(1u, sub1->getChildCount());
        auto sub2 = sub1->getChild(0);
        EXPECT_EQ("sub2", sub2->getName());
        EXPECT_EQ(EPropertyType::Struct, sub2->getType());

        EXPECT_EQ(0u, sub2->getChildCount());
    }

    TEST_F(ALuaScript_Interface, MarksInputsAsInput)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);
        auto  inputs = script->getInputs();
        const auto inputCount = inputs->getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            EXPECT_EQ(internal::EPropertySemantics::ScriptInput, inputs->getChild(i)->m_impl->getPropertySemantics());
        }
    }

    TEST_F(ALuaScript_Interface, MarksOutputsAsOutput)
    {
        auto*      script     = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithOutputs);
        auto       outputs     = script->getOutputs();
        const auto outputCount = outputs->getChildCount();
        for (size_t i = 0; i < outputCount; ++i)
        {
            EXPECT_EQ(internal::EPropertySemantics::ScriptOutput, outputs->getChild(i)->m_impl->getPropertySemantics());
        }
    }

    TEST_F(ALuaScript_Interface, AssignsDefaultValuesToItsInputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);
        auto  inputs = script->getInputs();

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
        EXPECT_EQ(0, *speedInt32->get<int32_t>());
        EXPECT_FLOAT_EQ(0.0f, *tempFloat->get<float>());
        EXPECT_EQ("", *nameString->get<std::string>());
        EXPECT_TRUE(enabledBool->get<bool>());
        EXPECT_FALSE(*enabledBool->get<bool>());
        EXPECT_THAT(*vec_2f->get<vec2f>(), ::testing::ElementsAre(0.0f, 0.0f));
        EXPECT_THAT(*vec_3f->get<vec3f>(), ::testing::ElementsAre(0.0f, 0.0f, 0.0f));
        EXPECT_THAT(*vec_4f->get<vec4f>(), ::testing::ElementsAre(0.0f, 0.0f, 0.0f, 0.0f));
        EXPECT_THAT(*vec_2i->get<vec2i>(), ::testing::ElementsAre(0, 0));
        EXPECT_THAT(*vec_3i->get<vec3i>(), ::testing::ElementsAre(0, 0, 0));
        EXPECT_THAT(*vec_4i->get<vec4i>(), ::testing::ElementsAre(0, 0, 0, 0));
    }

    TEST_F(ALuaScript_Interface, AssignsDefaultValuesToItsOutputs)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithOutputs);
        auto  outputs = script->getOutputs();

        auto speedInt32 = outputs->getChild("speed");
        auto tempFloat = outputs->getChild("temp");
        auto nameString = outputs->getChild("name");
        auto enabledBool = outputs->getChild("enabled");
        auto vec_2f = outputs->getChild("vec2f");
        auto vec_3f = outputs->getChild("vec3f");
        auto vec_4f = outputs->getChild("vec4f");
        auto vec_2i = outputs->getChild("vec2i");
        auto vec_3i = outputs->getChild("vec3i");
        auto vec_4i = outputs->getChild("vec4i");

        EXPECT_TRUE(speedInt32->get<int32_t>());
        EXPECT_EQ(0, speedInt32->get<int32_t>().value());
        EXPECT_TRUE(tempFloat->get<float>());
        EXPECT_EQ(0.0f, tempFloat->get<float>().value());
        EXPECT_TRUE(nameString->get<std::string>());
        EXPECT_EQ("", nameString->get<std::string>().value());
        EXPECT_TRUE(enabledBool->get<bool>());
        EXPECT_EQ(false, *enabledBool->get<bool>());

        EXPECT_TRUE(vec_2f->get<vec2f>());
        EXPECT_TRUE(vec_3f->get<vec3f>());
        EXPECT_TRUE(vec_4f->get<vec4f>());
        EXPECT_TRUE(vec_2i->get<vec2i>());
        EXPECT_TRUE(vec_3i->get<vec3i>());
        EXPECT_TRUE(vec_4i->get<vec4i>());
        vec2f zeroVec2f{ 0.0f, 0.0f };
        vec3f zeroVec3f{ 0.0f, 0.0f, 0.0f };
        vec4f zeroVec4f{ 0.0f, 0.0f, 0.0f, 0.0f };
        vec2i zeroVec2i{ 0, 0 };
        vec3i zeroVec3i{ 0, 0, 0 };
        vec4i zeroVec4i{ 0, 0, 0, 0 };
        EXPECT_EQ(zeroVec2f, *vec_2f->get<vec2f>());
        EXPECT_EQ(zeroVec3f, *vec_3f->get<vec3f>());
        EXPECT_EQ(zeroVec4f, *vec_4f->get<vec4f>());
        EXPECT_EQ(zeroVec2i, *vec_2i->get<vec2i>());
        EXPECT_EQ(zeroVec3i, *vec_3i->get<vec3i>());
        EXPECT_EQ(zeroVec4i, *vec_4i->get<vec4i>());
    }

    TEST_F(ALuaScript_Interface, AssignsDefaultValuesToArrays)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.array_int = ARRAY(3, INT)
                IN.array_float = ARRAY(3, FLOAT)
                IN.array_vec2f = ARRAY(3, VEC2F)
                OUT.array_int = ARRAY(3, INT)
                OUT.array_float = ARRAY(3, FLOAT)
                OUT.array_vec2f = ARRAY(3, VEC2F)
            end

            function run()
            end
        )");

        std::initializer_list<const Property*> rootProperties = { script->getInputs(), script->getOutputs() };

        for (auto rootProp : rootProperties)
        {
            auto array_int = rootProp->getChild("array_int");
            auto array_float = rootProp->getChild("array_float");
            auto array_vec2f = rootProp->getChild("array_vec2f");

            for (size_t i = 0; i < 3; ++i)
            {
                EXPECT_TRUE(array_int->getChild(i)->get<int32_t>());
                EXPECT_EQ(0, *array_int->getChild(i)->get<int32_t>());
                EXPECT_TRUE(array_float->getChild(i)->get<float>());
                EXPECT_FLOAT_EQ(0.0f, *array_float->getChild(i)->get<float>());
                EXPECT_TRUE(array_vec2f->getChild(i)->get<vec2f>());
                EXPECT_THAT(*array_vec2f->getChild(i)->get<vec2f>(), testing::ElementsAre(0.0f, 0.0f));
            }
        }
    }

}
