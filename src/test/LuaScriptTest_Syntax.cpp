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


namespace rlogic
{
    class ALuaScript_Syntax : public ALuaScript
    {
    protected:
    };

    TEST_F(ALuaScript_Syntax, ProducesErrorIfOnlyInterfaceOrRunIsPresent)
    {
        auto scriptWithRun = m_logicEngine.createLuaScriptFromSource(R"(
            function run()
            end
        )");

        ASSERT_EQ(nullptr, scriptWithRun);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());

        auto scriptWithInterface = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
        )");

        ASSERT_EQ(nullptr, scriptWithInterface);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorWithStackTraceInInterface)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.prop = nil
            end
            function run()
            end
        )", "myscript");

        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
                  "lua: error: Field 'prop' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!\n"
                  "stack traceback:\n"
                  "\t[C]: in ?\n"
                  "\t[string \"myscript\"]:3: in function <[string \"myscript\"]:2>");
    }

    TEST_F(ALuaScript_Syntax, CannotBeCreatedFromSyntacticallyIncorrectScript)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource("this.goes.boom");
        ASSERT_EQ(nullptr, script);
        ASSERT_EQ(1u, m_logicEngine.getErrors().size());
        EXPECT_EQ("[string \"unknown\"]:1: '=' expected near '<eof>'", m_logicEngine.getErrors()[0]);
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_FromGlobalScope)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            error("Expect this error!")

            function interface()
            end

            function run()
            end
        )");
        EXPECT_EQ(nullptr, script);
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
                  "[string \"unknown\"]:2: Expect this error!\nstack traceback:\n\t[C]: in function 'error'\n\t[string \"unknown\"]:2: in main chunk");
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_DuringInterfaceDeclaration)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                error("Expect this error!")
            end

            function run()
            end
        )");
        EXPECT_EQ(nullptr, script);
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
                  "[string \"unknown\"]:3: Expect this error!\nstack traceback:\n\t[C]: in function 'error'\n\t[string \"unknown\"]:3: in function <[string \"unknown\"]:2>");
    }

    TEST_F(ALuaScript_Syntax, PropagatesErrorsEmittedInLua_DuringRun)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end

            function run()
                error("Expect this error!")
            end
        )");

        ASSERT_NE(nullptr, script);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
                  "[string \"unknown\"]:6: Expect this error!\nstack traceback:\n\t[C]: in function 'error'\n\t[string \"unknown\"]:6: in function <[string \"unknown\"]:5>");
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorWhenIndexingVectorPropertiesOutOfRange)
    {
        auto* script  = m_logicEngine.createLuaScriptFromSource(R"(
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
                print("Value of " .. IN.propertyName .. "[" .. tostring(IN.index) .. "]" .. " is " .. IN[IN.propertyName][IN.index])
            end
        )");
        auto  inputs  = script->getInputs();

        inputs->getChild("vec2f")->set<vec2f>({1.1f, 1.2f});
        inputs->getChild("vec3f")->set<vec3f>({2.1f, 2.2f, 2.3f});
        inputs->getChild("vec4f")->set<vec4f>({3.1f, 3.2f, 3.3f, 3.4f});
        inputs->getChild("vec2i")->set<vec2i>({1, 2});
        inputs->getChild("vec3i")->set<vec3i>({3, 4, 5});
        inputs->getChild("vec4i")->set<vec4i>({6, 7, 8, 9});

        auto* index = inputs->getChild("index");
        auto* name = inputs->getChild("propertyName");

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
                        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Only non-negative integers supported as array index type!"));
                    }
                    else
                    {
                        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Index out of range!"));
                    }
                }
                else
                {
                    EXPECT_TRUE(m_logicEngine.update());
                    EXPECT_TRUE(m_logicEngine.getErrors().empty());
                }
            }
        }
    }

    TEST_F(ALuaScript_Syntax, ProdocesErrorWhenIndexingVectorWithNonIntegerIndices)
    {
        auto* script  = m_logicEngine.createLuaScriptFromSource(R"(
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
        )");
        auto  inputs  = script->getInputs();

        auto* errorType = inputs->getChild("errorType");

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
            EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("Only non-negative integers supported as array index type!"));
        }
    }

    TEST_F(ALuaScript_Syntax, ReportsErrorWhenTryingToAssignVectorTypes_WithMismatchedComponentCount)
    {
        const std::vector<LuaTestError> allCases =
        {
            {
                "OUT.vec2f = {}                 -- none at all",
                "lua: error: Expected 2 array components in table but got 0 instead!"
            },
            {
                "OUT.vec3f = {1, 2, 3, 4}       -- more than expected",
                "lua: error: Expected 3 array components in table but got 4 instead!"
            },
            {
                "OUT.vec4f = {1, 2, 3}          -- fewer than required",
                "lua: error: Expected 4 array components in table but got 3 instead!"
            },
            {
                "OUT.vec2i = {1, 2, 'wrong'}    -- extra component of wrong type",
                "lua: error: Expected 2 array components in table but got 3 instead!"
            },
            {
                "OUT.vec3i = {1, 2, {}}         -- extra nested table",
                "lua: error: Unexpected type table at array element # 3!"
            },
            {
                "OUT.vec4i = {1, 2, nil, 4}     -- wrong size, nil in-between",
                "lua: error: Expected 4 array components in table but got 3 instead!"
            },
            {
                "OUT.vec4i = {1, 2, nil, 3, 4}     -- correct size, nil in-between",
                "lua: error: Unexpected type nil at array element # 3!"
            },
        };

        for(const auto& errorCase : allCases)
        {
            auto scriptSource = std::string(R"(
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

            auto* script = m_logicEngine.createLuaScriptFromSource(scriptSource);

            ASSERT_NE(nullptr, script);
            EXPECT_FALSE(m_logicEngine.update());

            ASSERT_EQ(1u, m_logicEngine.getErrors().size());
            EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr(errorCase.expectedErrorMessage));

            EXPECT_TRUE(m_logicEngine.destroy(*script));
        }
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfRunFunctionDoesNotEndCorrectly)
    {
        auto scriptWithWrongEndInRun = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
            ENDE
        )");

        ASSERT_EQ(nullptr, scriptWithWrongEndInRun);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(m_logicEngine.getErrors()[0], "[string \"unknown\"]:6: '=' expected near '<eof>'");
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfInterfaceFunctionDoesNotEndCorrectly)
    {
        auto scriptWithWrongEndInInterface = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            ENDE
            function run()
            end
        )");

        ASSERT_EQ(nullptr, scriptWithWrongEndInInterface);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(m_logicEngine.getErrors()[0], "[string \"unknown\"]:4: '=' expected near 'function'");
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfInterfaceFunctionDoesNotEndAtAll)
    {
        auto scriptWithNoEndInInterface = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            function run()
            end
        )");

        ASSERT_EQ(nullptr, scriptWithNoEndInInterface);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(m_logicEngine.getErrors()[0], "[string \"unknown\"]:5: 'end' expected (to close 'function' at line 2) near '<eof>'");
    }

    TEST_F(ALuaScript_Syntax, ProducesErrorIfRunFunctionDoesNotEndAtAll)
    {
        auto scriptWithNoEndInRun = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
        )");

        ASSERT_EQ(nullptr, scriptWithNoEndInRun);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(m_logicEngine.getErrors()[0], "[string \"unknown\"]:5: 'end' expected (to close 'function' at line 4) near '<eof>'");
    }

}
