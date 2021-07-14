//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LuaScriptTest_Base.h"
#include "WithTempDirectory.h"

#include "impl/LuaScriptImpl.h"
#include "impl/LogicEngineImpl.h"
#include "impl/PropertyImpl.h"

#include "fmt/format.h"
#include <fstream>

namespace rlogic::internal
{
    class ALuaScript_Lifecycle : public ALuaScript
    {
    protected:
        WithTempDirectory tempFolder;
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

    TEST_F(ALuaScript_Lifecycle, ProducesErrorWhenLoadedFileWithRuntimeErrorsInTheInterfaceFunction)
    {
        std::ofstream ofs;
        ofs.open("script.lua", std::ofstream::out);
        ofs << R"(
            function interface()
                error("This will cause errors when creating the script")
            end
            function run()
            end
        )";
        ofs.close();

        const auto script = m_logicEngine.createLuaScriptFromFile("script.lua");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message,
                "[script.lua] Error while loading script. Lua stack trace:\n"
                "[string \"script.lua\"]:3: This will cause errors when creating the script\n"
                "stack traceback:\n"
                    "\t[C]: in function 'error'\n"
                    "\t[string \"script.lua\"]:3: in function <[string \"script.lua\"]:2>");
    }

    TEST_F(ALuaScript_Lifecycle, KeepsGlobalScopeSymbolsDuringInterfaceAndRunMethods)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(R"(
            -- 'Local' symbols in the global space are global too
            local global1 = "global1"
            global2 = "global2"

            function getGlobalString()
                return global1 .. global2
            end

            function interface()
                -- global symbols are available in interface
                if global1 == "global1" and global2 == "global2" then
                    OUT.result = STRING
                else
                    error("Expected global symbols were not found!")
                end
            end

            function run()
                -- global symbols are available here too
                if global1 == "global1" and global2 == "global2" then
                    OUT.result = getGlobalString()
                else
                    error("Expected global symbols were not found!")
                end
            end
        )");

        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(script->getOutputs()->getChild("result")->get<std::string>(), "global1global2");
    }

    class ALuaScript_LifecycleWithFiles : public ALuaScript_Lifecycle
    {
    };

    TEST_F(ALuaScript_LifecycleWithFiles, NoOutputs)
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
    }

    TEST_F(ALuaScript_LifecycleWithFiles, Arrays)
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
            const LuaScript* loadedScript = m_logicEngine.findScript("MyScript");

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
    }

    TEST_F(ALuaScript_LifecycleWithFiles, NestedArray)
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
            const LuaScript* loadedScript = m_logicEngine.findScript("MyScript");

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
    }

    TEST_F(ALuaScript_LifecycleWithFiles, NestedProperties)
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
    }

    TEST_F(ALuaScript_LifecycleWithFiles, ArrayOfStructs)
    {
        {
            LogicEngine tempLogicEngine;
            auto script = tempLogicEngine.createLuaScriptFromSource(
                R"(
                function interface()
                    local structDecl = {
                        str = STRING,
                        array = ARRAY(2, INT),
                        nested_struct = {
                            int = INT,
                            nested_array = ARRAY(1, FLOAT),
                        }
                    }
                    IN.arrayOfStructs = ARRAY(2, structDecl)
                    OUT.arrayOfStructs = ARRAY(2, structDecl)
                end
                function run()
                    OUT.arrayOfStructs = IN.arrayOfStructs
                end
            )", "MyScript");

            ASSERT_NE(nullptr, script);
            script->getInputs()->getChild("arrayOfStructs")->getChild(1)->getChild("nested_struct")->getChild("nested_array")->getChild(0)->set<float>(42.f);
            EXPECT_TRUE(tempLogicEngine.saveToFile("array_of_structs.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("array_of_structs.bin"));
            LuaScript* loadedScript = *m_logicEngine.scripts().begin();

            ASSERT_NE(nullptr, loadedScript);

            auto inputs = loadedScript->getInputs();
            auto outputs = loadedScript->getOutputs();

            ASSERT_NE(nullptr, inputs);
            ASSERT_NE(nullptr, outputs);

            auto loadedInput = loadedScript->getInputs()->getChild("arrayOfStructs")->getChild(1)->getChild("nested_struct")->getChild("nested_array")->getChild(0);
            EXPECT_FLOAT_EQ(42.f, *loadedInput->get<float>());
            loadedInput->set<float>(100.0f);

            EXPECT_TRUE(m_logicEngine.update());
            auto loadedOutput = loadedScript->getOutputs()->getChild("arrayOfStructs")->getChild(1)->getChild("nested_struct")->getChild("nested_array")->getChild(0);
            EXPECT_FLOAT_EQ(100.0f, *loadedOutput->get<float>());
        }
    }

    // This is a confidence test which tests all property types, both as inputs and outputs, and as arrays
    // The combination of arrays with different sizes, types, and their values, yields a lot of possible error cases, hence this test
    TEST_F(ALuaScript_LifecycleWithFiles, AllPropertyTypes_confidenceTest)
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
            const LuaScript* loadedScript = m_logicEngine.findScript("MyScript");

            auto inputs = loadedScript->getInputs();
            auto outputs = loadedScript->getOutputs();

            // Test both inputs and outputs
            for (auto rootProp : std::initializer_list<const Property*>{ inputs, outputs })
            {
                // One primitive for each type, and one array for each type
                ASSERT_EQ(rootProp->getChildCount(), allPrimitiveTypes.size() * 2);

                size_t expectedArraySize = 1;
                for(const auto primType: allPrimitiveTypes)
                {
                    const auto primitiveChild = rootProp->getChild(GetLuaPrimitiveTypeName(primType));
                    const auto arrayChild = inputs->getChild(std::string("array_") + GetLuaPrimitiveTypeName(primType));

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
    }

    TEST_F(ALuaScript_LifecycleWithFiles, OverwritesCurrentData_WhenLoadedASecondTimeFromTheSameFile)
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
    }
}
