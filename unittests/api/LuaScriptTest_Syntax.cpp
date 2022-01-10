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

namespace rlogic
{
    class ALuaScript_Syntax : public ALuaScript
    {
    protected:
    };

    TEST_F(ALuaScript_Syntax, ProducesErrorIfNoInterfaceIsPresent)
    {
        LuaScript* scriptNoInterface = m_logicEngine.createLuaScript(R"(
            function run()
            end
        )", {}, "scriptNoInterface");

        ASSERT_EQ(nullptr, scriptNoInterface);
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[scriptNoInterface] No 'interface' function defined!"));
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfNoRunIsPresent)
    {
        LuaScript* scriptNoRun = m_logicEngine.createLuaScript(R"(
            function interface()
            end
        )", {}, "scriptNoRun");

        ASSERT_EQ(nullptr, scriptNoRun);
        EXPECT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[scriptNoRun] No 'run' function defined!"));
    }

    TEST_F(ALuaScript_Syntax, CannotBeCreatedFromSyntacticallyIncorrectScript)
    {
        LuaScript* script = m_logicEngine.createLuaScript("this.is.not.valid.lua.code", {}, "badSyntaxScript");
        ASSERT_EQ(nullptr, script);
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[string \"badSyntaxScript\"]:1: '<name>' expected near 'not'"));
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_FromGlobalScope)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            error("Expect this error!")

            function interface()
            end

            function run()
            end
        )", WithStdModules({EStandardModule::Base}), "scriptWithErrorInGlobalCode");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
            "[string \"scriptWithErrorInGlobalCode\"]:2: Expect this error!\nstack traceback:\n"
            "\t[C]: in function 'error'\n"
            "\t[string \"scriptWithErrorInGlobalCode\"]:2: in main chunk"));
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_DuringInterfaceDeclaration)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                error("Expect this error!")
            end

            function run()
            end
        )", WithStdModules({EStandardModule::Base}), "scriptWithErrorInInterface");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
            "[string \"scriptWithErrorInInterface\"]:3: Expect this error!\nstack traceback:\n"
            "\t[C]: in function 'error'\n"
            "\t[string \"scriptWithErrorInInterface\"]:3: in function <[string \"scriptWithErrorInInterface\"]:2>"));
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_DuringRun)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
            end

            function run()
                error("Expect this error!")
            end
        )", WithStdModules({EStandardModule::Base}), "scriptWithErrorInRun");

        ASSERT_NE(nullptr, script);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
            "[string \"scriptWithErrorInRun\"]:6: Expect this error!\n"
            "stack traceback:\n"
            "\t[C]: in function 'error'\n"
            "\t[string \"scriptWithErrorInRun\"]:6: in function <[string \"scriptWithErrorInRun\"]:5>"));
        EXPECT_EQ(script, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorWhenIndexingVectorPropertiesOutOfRange)
    {
        LuaScript* script  = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.vec2f = VEC2F
                IN.vec3f = VEC3F
                IN.vec4f = VEC4F
                IN.vec2i = VEC2I
                IN.vec3i = VEC3I
                IN.vec4i = VEC4I

                -- Parametrize test in lua, this simplifies test readibility
                IN.propertyName = STRING
                IN.index = INT
            end

            function run()
                local message = "Value of " .. IN.propertyName .. "[" .. tostring(IN.index) .. "]" .. " is " .. IN[IN.propertyName][IN.index]
            end
        )", WithStdModules({EStandardModule::Base}), "scriptOOR");
        Property*  inputs  = script->getInputs();

        inputs->getChild("vec2f")->set<vec2f>({1.1f, 1.2f});
        inputs->getChild("vec3f")->set<vec3f>({2.1f, 2.2f, 2.3f});
        inputs->getChild("vec4f")->set<vec4f>({3.1f, 3.2f, 3.3f, 3.4f});
        inputs->getChild("vec2i")->set<vec2i>({1, 2});
        inputs->getChild("vec3i")->set<vec3i>({3, 4, 5});
        inputs->getChild("vec4i")->set<vec4i>({6, 7, 8, 9});

        Property* index = inputs->getChild("index");
        Property* name = inputs->getChild("propertyName");

        std::map<std::string, int32_t> sizeOfEachType =
        {
            {"vec2f", 2},
            {"vec3f", 3},
            {"vec4f", 4},
            {"vec2i", 2},
            {"vec3i", 3},
            {"vec4i", 4},
        };


        for (const auto& typeSizePair : sizeOfEachType)
        {
            const std::string& typeName = typeSizePair.first;
            int32_t componentCount = typeSizePair.second;
            name->set<std::string>(typeName);

            // Include invalid values -1 and N + 1
            for (int32_t i = -1; i <= componentCount + 1; ++i)
            {
                index->set<int32_t>(i);

                if (i < 1 || i > componentCount)
                {
                    EXPECT_FALSE(m_logicEngine.update());
                    ASSERT_EQ(1u, m_logicEngine.getErrors().size());

                    if (i < 0)
                    {
                        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("Only non-negative integers supported as array index type! Error while extracting integer: expected non-negative number, received '-1'"));
                    }
                    else
                    {
                        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(fmt::format("Bad index '{}', expected 1 <= i <= {}", i, componentCount)));
                    }

                    EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("scriptOOR"));
                    EXPECT_EQ(m_logicEngine.getErrors()[0].object, script);
                }
                else
                {
                    EXPECT_TRUE(m_logicEngine.update());
                    EXPECT_TRUE(m_logicEngine.getErrors().empty());
                }
            }
        }
    }

    TEST_F(ALuaScript_Syntax, CanUseLuaSyntaxForComputingArraySize)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.array = ARRAY(3, INT)
                OUT.array_size = INT
            end

            function run()
                OUT.array_size = #IN.array
            end
        )");

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(3, *script->getOutputs()->getChild("array_size")->get<int32_t>());
    }

    TEST_F(ALuaScript_Syntax, CanUseLuaSyntaxForComputingComplexArraySize)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.array = ARRAY(3,
                    {
                        vec3 = VEC3F,
                        vec4i = VEC4I
                    }
                )
                OUT.array_size = INT
            end

            function run()
                OUT.array_size = #IN.array
            end
        )");

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(3, *script->getOutputs()->getChild("array_size")->get<int32_t>());
    }

    TEST_F(ALuaScript_Syntax, CanUseLuaSyntaxForComputingStructSize)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.struct = {
                    data1 = VEC3F,
                    data2 = VEC4I,
                    data3 = INT
                }
                OUT.struct_size = INT
            end

            function run()
                OUT.struct_size = #IN.struct
            end
        )");

        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(3, *script->getOutputs()->getChild("struct_size")->get<int32_t>());
    }

    TEST_F(ALuaScript_Syntax, CanUseLuaSyntaxForComputingVec234Size)
    {
        m_logicEngine.createLuaScript(R"(
            function interface()
                IN.vec2f = VEC2F
                IN.vec3f = VEC3F
                IN.vec4f = VEC4F
                IN.vec2i = VEC2I
                IN.vec3i = VEC3I
                IN.vec4i = VEC4I
            end

            function run()
                if #IN.vec2i ~= 2 then error("Expected vec2i has size 2!") end
                if #IN.vec2f ~= 2 then error("Expected vec2f has size 2!") end
                if #IN.vec3i ~= 3 then error("Expected vec3i has size 3!") end
                if #IN.vec3f ~= 3 then error("Expected vec3f has size 3!") end
                if #IN.vec4i ~= 4 then error("Expected vec4i has size 4!") end
                if #IN.vec4f ~= 4 then error("Expected vec4f has size 4!") end
            end
        )");

        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_Syntax, CanUseLuaSyntaxForComputingSizeOfStrings)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.string = STRING
                OUT.string_size = INT
            end

            function run()
                OUT.string_size = #IN.string
            end
        )");

        script->getInputs()->getChild("string")->set<std::string>("abcde");
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *script->getOutputs()->getChild("string_size")->get<int32_t>());
    }

    TEST_F(ALuaScript_Syntax, RaisesErrorWhenTryingToGetSizeOfNonArrayTypes)
    {
        LuaScript* script = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.notArray = INT
            end

            function run()
                local size = #IN.notArray
            end
        )", {}, "invalidArraySizeAccess");

        ASSERT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("attempt to get length of field 'notArray' (a number value)"));
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("invalidArraySizeAccess"));
        EXPECT_EQ(m_logicEngine.getErrors()[0].object, script);
    }

    TEST_F(ALuaScript_Syntax, ProdocesErrorWhenIndexingVectorWithNonIntegerIndices)
    {
        LuaScript* script  = m_logicEngine.createLuaScript(R"(
            function interface()
                IN.vec = VEC4I

                IN.errorType = STRING
            end

            function run()
                if IN.errorType == "indexWithNil" then
                    local thisWillFail = IN.vec[nil]
                elseif IN.errorType == "indexIsATable" then
                    local thisWillFail = IN.vec[{1}]
                elseif IN.errorType == "indexIsAString" then
                    local thisWillFail = IN.vec["nope..."]
                elseif IN.errorType == "indexIsAFloat" then
                    local thisWillFail = IN.vec[1.5]
                elseif IN.errorType == "indexIsAUserdata" then
                    local thisWillFail = IN.vec[IN.vec]
                else
                    error("Test problem - check error cases below")
                end
            end
        )", {}, "invalidIndexingScript");
        Property*  inputs = script->getInputs();

        Property* errorType = inputs->getChild("errorType");

        std::vector<std::string> errorTypes =
        {
            "indexWithNil",
            "indexIsATable",
            "indexIsAString",
            "indexIsAFloat",
            "indexIsAUserdata",
        };

        for (const auto& error : errorTypes)
        {
            errorType->set<std::string>(error);
            EXPECT_FALSE(m_logicEngine.update());
            ASSERT_EQ(1u, m_logicEngine.getErrors().size());

            // TODO Violin this is a workaround for a compiler issue with VS 2019 version 16.7 and latest sol
            // Investigate further if not fixed by VS or sol or both
            // Issue summary: function overloads handled differently on MSVC and clang/gcc -> results in different behavior in this test
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::AnyOf(::testing::HasSubstr("Only non-negative integers supported as array index type!"),
                                                                       ::testing::HasSubstr("not a numeric type")));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("invalidIndexingScript"));
            EXPECT_EQ(m_logicEngine.getErrors()[0].object, script);
        }
    }

    TEST_F(ALuaScript_Syntax, ReportsErrorWhenTryingToAssignVectorTypes_WithMismatchedComponentCount)
    {
        const std::vector<LuaTestError> allCases =
        {
            {
                "OUT.vec2f = {}                 -- none at all",
                "Error while assigning output VEC2 property 'vec2f'. Error while extracting array: expected 2 array components in table but got 0 instead!"
            },
            {
                "OUT.vec3f = {1, 2, 3, 4}       -- more than expected",
                "Error while assigning output VEC3 property 'vec3f'. Error while extracting array: expected 3 array components in table but got 4 instead!"
            },
            {
                "OUT.vec4f = {1, 2, 3}          -- fewer than required",
                "Error while assigning output VEC4 property 'vec4f'. Error while extracting array: expected 4 array components in table but got 3 instead!"
            },
            {
                "OUT.vec2i = {1, 2, 'wrong'}    -- extra component of wrong type",
                "Error while assigning output VEC2 property 'vec2i'. Error while extracting array: expected 2 array components in table but got 3 instead!"
            },
            {
                "OUT.vec3i = {1, 2, {}}         -- extra nested table",
                "Error while assigning output VEC3 property 'vec3i'. Error while extracting array: unexpected value (type: 'table') at array element # 3! "
                "Reason: Error while extracting integer: expected a number, received 'table'"
            },
            {
                "OUT.vec4i = {1, 2, nil, 4}     -- wrong size, nil in-between",
                "Error while assigning output VEC4 property 'vec4i'. Error while extracting array: unexpected value (type: 'nil') at array element # 3! "
                "Reason: Error while extracting integer: expected a number, received 'nil'"
            },
            {
                "OUT.vec4i = {1, 2, nil, 3, 4}     -- correct size, nil in-between",
                "Error while assigning output VEC4 property 'vec4i'. Error while extracting array: expected 4 array components in table but got 5 instead!"
            },
        };

        for(const auto& errorCase : allCases)
        {
            std::string scriptSource =(R"(
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
            )");
            scriptSource += errorCase.errorCode;
            scriptSource += "\nend\n";

            LuaScript* script = m_logicEngine.createLuaScript(scriptSource, {}, "mismatchedVecSizes");

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(1u, m_logicEngine.getErrors().size());
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(errorCase.expectedErrorMessage));
            EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("mismatchedVecSizes"));
            EXPECT_EQ(m_logicEngine.getErrors()[0].object, script);

            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfRunFunctionDoesNotEndCorrectly)
    {
        LuaScript* scriptWithWrongEndInRun = m_logicEngine.createLuaScript(R"(
            function interface()
            end
            function run()
            ENDE
        )", {}, "missingEndInScript");

        ASSERT_EQ(nullptr, scriptWithWrongEndInRun);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[string \"missingEndInScript\"]:6: '=' expected near '<eof>'"));
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfInterfaceFunctionDoesNotEndCorrectly)
    {
        LuaScript* scriptWithWrongEndInInterface = m_logicEngine.createLuaScript(R"(
            function interface()
            ENDE
            function run()
            end
        )", {}, "missingEndInScript");

        ASSERT_EQ(nullptr, scriptWithWrongEndInInterface);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[string \"missingEndInScript\"]:4: '=' expected near 'function'"));
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfInterfaceFunctionDoesNotEndAtAll)
    {
        LuaScript* scriptWithNoEndInInterface = m_logicEngine.createLuaScript(R"(
            function interface()
            function run()
            end
        )", {}, "endlessInterface");

        ASSERT_EQ(nullptr, scriptWithNoEndInInterface);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[string \"endlessInterface\"]:5: 'end' expected (to close 'function' at line 2) near '<eof>'"));
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfRunFunctionDoesNotEndAtAll)
    {
        LuaScript* scriptWithNoEndInRun = m_logicEngine.createLuaScript(R"(
            function interface()
            end
            function run()
        )", {}, "endlessRun");

        ASSERT_EQ(nullptr, scriptWithNoEndInRun);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("[string \"endlessRun\"]:5: 'end' expected (to close 'function' at line 4) near '<eof>'"));
    }

}
