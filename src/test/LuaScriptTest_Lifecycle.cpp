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

#include "generated/luascript_gen.h"
#include "generated/logicnode_gen.h"

#include <fstream>

namespace rlogic
{
    class ALuaScript_Lifecycle : public ALuaScript
    {
    protected:
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
        std::remove("script.bin");
    }

    TEST_F(ALuaScript_Lifecycle, OverwritesCurrentData_WhenLoadedASecondTimeFromTheSameFile)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                my_var = 5
                function interface()
                    IN.in_param = INT
                    OUT.out_param = INT
                end
                function run()
                    OUT.out_param = OUT.out_param + my_var
                end
            )", "MyScript");

            ASSERT_NE(nullptr, script);
            EXPECT_TRUE(tempLogicEngine.saveToFile("script.bin"));
        }
        for(size_t i = 0; i < 2; ++i)
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("script.bin"));
            const LuaScript* loadedScript = *m_logicEngine.scripts().begin();

            ASSERT_NE(nullptr, loadedScript);
            EXPECT_TRUE(m_logicEngine.update());
            EXPECT_EQ(5, *loadedScript->getOutputs()->getChild("out_param")->get<int32_t>());
            EXPECT_TRUE(m_logicEngine.update());
            EXPECT_EQ(10, *loadedScript->getOutputs()->getChild("out_param")->get<int32_t>());
        }
        std::remove("script.bin");
    }

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
            std::vector<std::string> errors;
            internal::LuaStateImpl   state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ("Error during deserialization of inputs", errors[0]);
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
            std::vector<std::string> errors;
            internal::LuaStateImpl   state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ("Error during deserialization of outputs", errors[0]);
        }
    }

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
            std::vector<std::string> errors;
            internal::LuaStateImpl   state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.size());
            EXPECT_THAT(errors[0], ::testing::HasSubstr("'=' expected near '<eof>'"));
        }
    }

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
            std::vector<std::string> errors;
            internal::LuaStateImpl   state;
            auto                     script = internal::LuaScriptImpl::Create(state, rlogic_serialization::GetLuaScript(builder.GetBufferPointer()), errors);

            EXPECT_EQ(nullptr, script);
            ASSERT_EQ(1u, errors.size());
            EXPECT_EQ("Error during execution of main function of deserialized script", errors[0]);
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
