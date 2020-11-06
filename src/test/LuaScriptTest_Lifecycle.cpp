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

#include "internals/impl/LuaScriptImpl.h"
#include "internals/impl/LogicEngineImpl.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/ErrorReporting.h"

#include "generated/luascript_gen.h"
#include "generated/logicnode_gen.h"

#include "fmt/format.h"
#include <fstream>

namespace rlogic::internal
{
    class ALuaScript_Lifecycle : public ALuaScript
    {
    protected:
        void TearDown() override {
            std::remove("script.bin");
            std::remove("script.lua");
            std::remove("arrays.bin");
            std::remove("nested_array.bin");
        }
    };

    TEST_F(ALuaScript_Lifecycle, HasEmptyFilenameWhenCreatedFromSource)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScript);
        ASSERT_NE(nullptr, script);
        EXPECT_EQ("", script->getFilename());
    }

    TEST_F(ALuaScript_Lifecycle, ProducesNoErrorsWhenCreatedFromMinimalScript)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScript);
        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALuaScript_Lifecycle, ProvidesNameAsPassedDuringCreation)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScript, "script name");
        EXPECT_EQ("script name", script->getName());
        EXPECT_EQ("", script->getFilename());
    }

    TEST_F(ALuaScript_Lifecycle, CanBeSerializedAndDeserialized_NoOutputs)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                    function interface()
                        IN.param = INT
                    end
                    function run()
                    end
                )", "MyScript");

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(tempLogicEngine.saveToFile("script.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("script.bin"));
            const LuaScript* loadedScript = *m_logicEngine.scripts().begin();

            ASSERT_NE(nullptr, loadedScript);
            EXPECT_EQ("MyScript", loadedScript->getName());
            EXPECT_EQ("", loadedScript->getFilename());

            auto inputs = loadedScript->getInputs();
            auto outputs = loadedScript->getOutputs();

            ASSERT_NE(nullptr, inputs);
            ASSERT_NE(nullptr, outputs);

            ASSERT_EQ(inputs->getChildCount(), 1u);
            ASSERT_EQ(outputs->getChildCount(), 0u);

            EXPECT_EQ("param", inputs->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Int32, inputs->getChild(0u)->getType());

            EXPECT_TRUE(m_logicEngine.update());
        }
        std::remove("script.lua");
    }

    TEST_F(ALuaScript_Lifecycle, CanBeSerializedAndDeserialized_FromEmptySourceFile)
    {
        std::ofstream ofs;
        ofs.open("script.lua", std::ofstream::out);
        ofs << R"(
            function interface()
            end
            function run()
            end
        )";
        ofs.close();

        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromFile("script.lua", "MyScript");

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(tempLogicEngine.saveToFile("script.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("script.bin"));
            const LuaScript* loadedScript = *m_logicEngine.scripts().begin();

            ASSERT_NE(nullptr, loadedScript);
            EXPECT_EQ("MyScript", loadedScript->getName());
            EXPECT_EQ("script.lua", loadedScript->getFilename());

            EXPECT_TRUE(m_logicEngine.update());
        }
        std::remove("script.lua");
        std::remove("script.bin");
    }

    TEST_F(ALuaScript_Lifecycle, ProducesErrorWhenLoadedFromFaultyFile)
    {
        std::ofstream ofs;
        ofs.open("script.lua", std::ofstream::out);
        ofs << R"(
            function interface()
                IN.prop = nil
            end
            function run()
            end
        )";
        ofs.close();

        const auto script = m_logicEngine.createLuaScriptFromFile("script.lua");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("\"script.lua\""));

        std::remove("script.lua");
    }

    TEST_F(ALuaScript_Lifecycle, CanBeSerializedAndDeserialized_Arrays)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                function interface()
                    IN.array = ARRAY(2, FLOAT)
                end
                function run()
                end
            )", "MyScript");

            ASSERT_NE(nullptr, script);


            script->getInputs()->getChild("array")->getChild(0)->set<float>(0.1f);
            script->getInputs()->getChild("array")->getChild(1)->set<float>(0.2f);
            EXPECT_TRUE(tempLogicEngine.saveToFile("script.bin"));
        }
        {
            m_logicEngine.loadFromFile("script.bin");
            const LuaScript* loadedScript = findLuaScriptByName("MyScript");

            const auto inputs = loadedScript->getInputs();

            ASSERT_NE(nullptr, inputs);

            ASSERT_EQ(inputs->getChildCount(), 1u);

            // Full type inspection of array type, children and values
            EXPECT_EQ("array", inputs->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Array, inputs->getChild(0u)->getType());
            EXPECT_EQ(EPropertyType::Float, inputs->getChild(0u)->getChild(0u)->getType());
            EXPECT_EQ(EPropertyType::Float, inputs->getChild(0u)->getChild(1u)->getType());
            EXPECT_EQ("", inputs->getChild(0u)->getChild(0u)->getName());
            EXPECT_EQ("", inputs->getChild(0u)->getChild(1u)->getName());
            EXPECT_EQ(0u, inputs->getChild(0u)->getChild(0u)->getChildCount());
            EXPECT_EQ(0u, inputs->getChild(0u)->getChild(1u)->getChildCount());
            EXPECT_FLOAT_EQ(0.1f, *inputs->getChild(0u)->getChild(0u)->get<float>());
            EXPECT_FLOAT_EQ(0.2f, *inputs->getChild(0u)->getChild(1u)->get<float>());

        }
        std::remove("script.bin");
    }

    TEST_F(ALuaScript_Lifecycle, CanBeSerializedAndDeserialized_NestedArray)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                function interface()
                    IN.nested =
                    {
                        array = ARRAY(1, VEC3F)
                    }
                end
                function run()
                end
            )", "MyScript");

            script->getInputs()->getChild("nested")->getChild("array")->getChild(0)->set<vec3f>({1.1f, 1.2f, 1.3f});
            EXPECT_TRUE(tempLogicEngine.saveToFile("arrays.bin"));
        }
        {
            m_logicEngine.loadFromFile("arrays.bin");
            const LuaScript* loadedScript = findLuaScriptByName("MyScript");

            const auto inputs = loadedScript->getInputs();

            ASSERT_EQ(inputs->getChildCount(), 1u);

            // Type inspection on nested array
            const auto nested = inputs->getChild(0u);
            EXPECT_EQ("nested", nested->getName());
            auto nested_array = nested->getChild(0u);
            EXPECT_EQ("array", nested_array->getName());

            // Check children of nested array, also values
            EXPECT_EQ(1u, nested_array->getChildCount());
            EXPECT_EQ("", nested_array->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Vec3f, nested_array->getChild(0u)->getType());
            EXPECT_EQ(0u, nested_array->getChild(0u)->getChildCount());
            EXPECT_THAT(*nested_array->getChild(0u)->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));

        }
        std::remove("arrays.bin");
    }

    TEST_F(ALuaScript_Lifecycle, CanBeSerializedAndDeserialized_NestedProperties)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                function interface()
                    IN.int_param = INT
                    IN.nested_param = {
                        int_param = INT
                    }
                    OUT.float_param = FLOAT
                    OUT.nested_param = {
                        float_param = FLOAT
                    }
                end
                function run()
                    OUT.float_param = 47.11
                end
            )", "MyScript");

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(tempLogicEngine.saveToFile("nested_array.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("nested_array.bin"));
            const LuaScript* loadedScript = *m_logicEngine.scripts().begin();

            ASSERT_NE(nullptr, loadedScript);
            EXPECT_EQ("MyScript", loadedScript->getName());
            EXPECT_EQ("", loadedScript->getFilename());

            auto inputs = loadedScript->getInputs();
            auto outputs = loadedScript->getOutputs();

            ASSERT_NE(nullptr, inputs);
            ASSERT_NE(nullptr, outputs);

            ASSERT_EQ(inputs->getChildCount(), 2u);
            ASSERT_EQ(outputs->getChildCount(), 2u);

            EXPECT_EQ("int_param", inputs->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Int32, inputs->getChild(0u)->getType());
            EXPECT_EQ("float_param", outputs->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Float, outputs->getChild(0u)->getType());

            auto in_child = inputs->getChild(1u);
            auto out_child = outputs->getChild(1u);

            EXPECT_EQ("nested_param", in_child->getName());
            EXPECT_EQ(EPropertyType::Struct, in_child->getType());
            EXPECT_EQ("nested_param", out_child->getName());
            EXPECT_EQ(EPropertyType::Struct, out_child->getType());

            ASSERT_EQ(in_child->getChildCount(), 1u);
            ASSERT_EQ(out_child->getChildCount(), 1u);

            auto in_nested_child = in_child->getChild(0u);
            auto out_nested_child = out_child->getChild(0u);

            EXPECT_EQ("int_param", in_nested_child->getName());
            EXPECT_EQ(EPropertyType::Int32, in_nested_child->getType());
            EXPECT_EQ("float_param", out_nested_child->getName());
            EXPECT_EQ(EPropertyType::Float, out_nested_child->getType());

            EXPECT_TRUE(m_logicEngine.update());
            EXPECT_FLOAT_EQ(47.11f, *outputs->getChild(0u)->get<float>());
        }
        std::remove("nested_array.bin");
    }

    // This is a confidence test which tests all property types, both as inputs and outputs, and as arrays
    // The combination of arrays with different sizes, types, and their values, yields a lot of possible error cases, hence this test
    TEST_F(ALuaScript_Lifecycle, CanSaveAndLoadAllPropertyTypesToFile_confidenceTest)
    {
        const std::vector<EPropertyType> allPrimitiveTypes =
        {
            EPropertyType::Float,
            EPropertyType::Vec2f,
            EPropertyType::Vec3f,
            EPropertyType::Vec4f,
            EPropertyType::Int32,
            EPropertyType::Vec2i,
            EPropertyType::Vec3i,
            EPropertyType::Vec4i,
            EPropertyType::String,
            EPropertyType::Bool
        };

        std::string scriptSrc = "function interface()\n";
        size_t arraySize = 1;
        // For each type, create an input, output, and an array version of it with various sizes
        for (auto primType : allPrimitiveTypes)
        {
            const std::string typeName = GetLuaPrimitiveTypeName(primType);
            scriptSrc += fmt::format("IN.{} =  {}\n", typeName, typeName);
            scriptSrc += fmt::format("IN.array_{} = ARRAY({}, {})\n", typeName, arraySize, typeName);
            scriptSrc += fmt::format("OUT.{} = {}\n", typeName, typeName);
            scriptSrc += fmt::format("OUT.array_{} = ARRAY({}, {})\n", typeName, arraySize, typeName);
            ++arraySize;
        }

        scriptSrc += R"(
                end
                function run()
                end
            )";

        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(scriptSrc, "MyScript");

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(tempLogicEngine.saveToFile("arrays.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("arrays.bin"));
            const LuaScript* loadedScript = findLuaScriptByName("MyScript");

            auto inputs = loadedScript->getInputs();
            auto outputs = loadedScript->getOutputs();

            // Test both inputs and outputs
            for (auto rootProp : std::initializer_list<const Property*>{ inputs, outputs })
            {
                // One primitive for each type, and one array for each type
                ASSERT_EQ(rootProp->getChildCount(), allPrimitiveTypes.size() * 2);

                size_t expectedArraySize = 1;
                for (size_t i = 0; i < allPrimitiveTypes.size(); ++i)
                {
                    auto primType = allPrimitiveTypes[i];
                    const auto primitiveChild = rootProp->getChild(i * 2);
                    const auto arrayChild = inputs->getChild(i * 2 + 1);

                    const std::string typeName = GetLuaPrimitiveTypeName(primType);

                    EXPECT_EQ(primType, primitiveChild->getType());
                    EXPECT_EQ(typeName, primitiveChild->getName());
                    EXPECT_EQ(0u, primitiveChild->getChildCount());

                    EXPECT_EQ("array_" + typeName, arrayChild->getName());
                    EXPECT_EQ(EPropertyType::Array, arrayChild->getType());
                    EXPECT_EQ(expectedArraySize, arrayChild->getChildCount());

                    for (size_t a = 0; a < expectedArraySize; ++a)
                    {
                        const auto arrayElement = arrayChild->getChild(a);
                        EXPECT_EQ("", arrayElement->getName());
                        EXPECT_EQ(primType, arrayElement->getType());
                        EXPECT_EQ(0u, arrayElement->getChildCount());
                    }
                    ++expectedArraySize;
                }
            }
        }
        std::remove("arrays.bin");
    }

    TEST_F(ALuaScript_Lifecycle, OverwritesCurrentData_WhenLoadedASecondTimeFromTheSameFile)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                function interface()
                    IN.data = INT
                end
                function run()
                end
            )", "MyScript");

            ASSERT_NE(nullptr, script);
            script->getInputs()->getChild("data")->set<int32_t>(42);
            EXPECT_TRUE(tempLogicEngine.saveToFile("script.bin"));
        }

        EXPECT_TRUE(m_logicEngine.loadFromFile("script.bin"));
        auto loadedScript = *m_logicEngine.scripts().begin();
        loadedScript->getInputs()->getChild("data")->set<int32_t>(5);

        EXPECT_TRUE(m_logicEngine.loadFromFile("script.bin"));
        loadedScript = *m_logicEngine.scripts().begin();
        EXPECT_EQ(42, *loadedScript->getInputs()->getChild("data")->get<int32_t>());

        std::remove("script.bin");
    }

    // TODO Violin this test does not make sense - this code path can't be triggered by user. Rework!
    // What we can test (and should) is that a real script without inputs/outputs can be deserialized properly
    TEST_F(ALuaScript_Lifecycle, ProducesErrorIfDeserializedWithoutInputs)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto script = rlogic_serialization::CreateLuaScript(
                builder, rlogic_serialization::CreateLogicNode(
                    builder,
                    builder.CreateString("StcriptName")
                ),
                builder.CreateString("Filename"),
                builder.CreateString("")
            );

            builder.Finish(script);
        }
        {
            ErrorReporting errors;
            internal::SolState   state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.getErrors().size());
            EXPECT_EQ("Error during deserialization of inputs", errors.getErrors()[0]);
        }
    }

    TEST_F(ALuaScript_Lifecycle, ProducesErrorIfDeserializedWithoutOutputs)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            internal::PropertyImpl input("Input", EPropertyType::Int32, internal::EInputOutputProperty::Input);
            auto script = rlogic_serialization::CreateLuaScript(builder,
                rlogic_serialization::CreateLogicNode(
                    builder,
                    builder.CreateString("ScriptName"),
                    input.serialize(builder)

                ),
                builder.CreateString("Filename"),
                builder.CreateString(""),
                0
            );

            builder.Finish(script);
        }
        {
            ErrorReporting errors;
            internal::SolState       state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.getErrors().size());
            EXPECT_EQ("Error during deserialization of outputs", errors.getErrors()[0]);
        }
    }

    // TODO Violin this test does not make sense - should be deleted, and check if there is related code which can be improved
    TEST_F(ALuaScript_Lifecycle, ProducesErrorIfDeserializedWithScriptWithCompileTimeError)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto                   source = R"(
                this.goes.boom
            )";
            internal::PropertyImpl input("Input", EPropertyType::Int32, internal::EInputOutputProperty::Input);
            internal::PropertyImpl output("Output", EPropertyType::Int32, internal::EInputOutputProperty::Output);

            auto script = rlogic_serialization::CreateLuaScript(
                builder,
                rlogic_serialization::CreateLogicNode(
                    builder,
                    builder.CreateString("ScriptName"),
                    input.serialize(builder),
                    output.serialize(builder)
                ),
                builder.CreateString("Filename"),
                builder.CreateString(source),
                0
            );

            builder.Finish(script);
        }
        {
            ErrorReporting errors;
            internal::SolState       state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.getErrors().size());
            EXPECT_THAT(errors.getErrors()[0], ::testing::HasSubstr("'=' expected near '<eof>'"));
        }
    }

    // TODO Violin this test does not make sense - should be deleted, and check if there is related code which can be improved
    TEST_F(ALuaScript_Lifecycle, ProducesErrorIfDeserializedWithScriptWithRuntimeError)
    {
        flatbuffers::FlatBufferBuilder builder;
        {
            auto                   source = R"(
                function add(a,b)
                    return a+b
                end
                add(2)
            )";
            internal::PropertyImpl input("Input", EPropertyType::Int32, internal::EInputOutputProperty::Input);
            internal::PropertyImpl output("Output", EPropertyType::Int32, internal::EInputOutputProperty::Output);

            auto script = rlogic_serialization::CreateLuaScript(builder,
                rlogic_serialization::CreateLogicNode(
                    builder,
                    builder.CreateString("ScriptName"),
                    input.serialize(builder),
                    output.serialize(builder)
                ),
                builder.CreateString("Filename"),
                builder.CreateString(source),
                0);

            builder.Finish(script);
        }
        {
            ErrorReporting errors;
            internal::SolState       state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.getErrors().size());
            EXPECT_EQ("Error during execution of main function of deserialized script", errors.getErrors()[0]);
        }
    }

    TEST_F(ALuaScript_Lifecycle, KeepsGlobalScopeSymbolsDuringInterfaceAndRunMethods)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            local test = "test"
            test2 = "string"

            function my_concat(str1, str2)
                return str1 .. str2
            end

            function interface()
                if test == "test" then
                    OUT.test = STRING
                end
            end

            function run()
                OUT.test = my_concat(test, test2)
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(script->getOutputs()->getChild("test")->get<std::string>(), "teststring");
    }
}
