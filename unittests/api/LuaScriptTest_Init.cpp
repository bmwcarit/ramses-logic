//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LuaScriptTest_Base.h"

#include "ramses-logic/Property.h"
#include "LogTestUtils.h"
#include "WithTempDirectory.h"


namespace rlogic
{
    class ALuaScript_Init : public ALuaScript
    {
    protected:
        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
    };

    TEST_F(ALuaScript_Init, CreatesGlobals)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function init()
                GLOBAL.number = 5
                GLOBAL.string = "foo"
                GLOBAL.bool = false
            end

            function interface()
                OUT.number = INT
                OUT.string = STRING
                OUT.bool = BOOL
            end

            function run()
                OUT.number = GLOBAL.number
                OUT.string = GLOBAL.string
                OUT.bool = GLOBAL.bool
            end
        )");

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *script->getOutputs()->getChild("number")->get<int32_t>());
        EXPECT_EQ("foo", *script->getOutputs()->getChild("string")->get<std::string>());
        EXPECT_EQ(false, *script->getOutputs()->getChild("bool")->get<bool>());
    }

    TEST_F(ALuaScript_Init, CanUseGlobalsInInterface)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function init()
                GLOBAL.inputNames = {"foo", "bar"}
            end

            function interface()
                for key,value in pairs(GLOBAL.inputNames) do
                    OUT[value] = FLOAT
                end
            end

            function run()
                for key,value in pairs(GLOBAL.inputNames) do
                    OUT[value] = 4.2
                end
            end
        )", WithStdModules({EStandardModule::Base}));

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_FLOAT_EQ(4.2f, *script->getOutputs()->getChild("foo")->get<float>());
        EXPECT_FLOAT_EQ(4.2f, *script->getOutputs()->getChild("bar")->get<float>());
    }

    TEST_F(ALuaScript_Init, CanModifyGlobalsAsIfTheyWereGlobalVariables)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function init()
                GLOBAL.number = 5
            end

            function interface()
                IN.setGlobal = INT
                OUT.getGlobal = INT
            end

            function run()
                if IN.setGlobal ~= 0 then
                    GLOBAL.number = IN.setGlobal
                end
                OUT.getGlobal = GLOBAL.number
            end
        )");

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *script->getOutputs()->getChild("getGlobal")->get<int32_t>());

        EXPECT_TRUE(script->getInputs()->getChild("setGlobal")->set<int32_t>(42));
        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(42, *script->getOutputs()->getChild("getGlobal")->get<int32_t>());
    }

    TEST_F(ALuaScript_Init, CanDeclareFunctions)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function init()
                function getNumber() return 42 end
                GLOBAL.fun = getNumber
            end

            function interface()
                OUT.getGlobal = INT
            end

            function run()
                OUT.getGlobal = GLOBAL.fun()
            end
        )");

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(42, *script->getOutputs()->getChild("getGlobal")->get<int32_t>());
    }

    TEST_F(ALuaScript_Init, CanUseStandardModules)
    {
        auto* script = m_logicEngine.createLuaScript(R"(
            function init()
                GLOBAL.number = math.floor(4.2)
            end

            function interface()
                OUT.getGlobal = INT
            end

            function run()
                OUT.getGlobal = GLOBAL.number
            end
        )", WithStdModules({ EStandardModule::Math }));

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_EQ(4, *script->getOutputs()->getChild("getGlobal")->get<int32_t>());
    }

    TEST_F(ALuaScript_Init, CanUseCustomModules)
    {
        const std::string_view moduleSourceCode = R"(
            local mymath = {}
            function mymath.add(a,b)
                return a+b
            end
            mymath.PI=3.1415
            return mymath
        )";

        LuaConfig config;
        config.addDependency("mymath", *m_logicEngine.createLuaModule(moduleSourceCode));

        auto* script = m_logicEngine.createLuaScript(R"(
            modules("mymath")

            function init()
                GLOBAL.number = mymath.add(5, mymath.PI)
            end
            function interface()
                OUT.getGlobal = FLOAT
            end
            function run()
                OUT.getGlobal = GLOBAL.number
            end
        )", config);

        ASSERT_TRUE(m_logicEngine.update());
        EXPECT_FLOAT_EQ(8.1415f, *script->getOutputs()->getChild("getGlobal")->get<float>());
    }

    // TODO Violin re-enable this test after fixing isolation of modules
    TEST_F(ALuaScript_Init, DISABLED_IssuesErrorWhenUsingUndeclaredStandardModule)
    {
        m_logicEngine.createLuaScript(R"(
            function init()
                GLOBAL.number = math.floor(4.2)
            end
            function interface()
            end
            function run()
            end
        )");

        EXPECT_FALSE(m_logicEngine.update());
        // Expect error
    }

    TEST_F(ALuaScript_Init, InitializesAfterDeserilization)
    {
        WithTempDirectory tmpFolder;

        {
            LogicEngine tmpLogicEngine;
            auto* script = tmpLogicEngine.createLuaScript(R"(
                function init()
                    GLOBAL.number = 5
                end

                function interface()
                    OUT.globalValueBefore = INT
                    OUT.globalValueAfter = INT
                end

                function run()
                    OUT.globalValueBefore = GLOBAL.number
                    GLOBAL.number = 42
                    OUT.globalValueAfter = GLOBAL.number
                end
            )", WithStdModules({EStandardModule::Base}), "withGlobals");

            ASSERT_TRUE(tmpLogicEngine.update());
            ASSERT_EQ(5, *script->getOutputs()->getChild("globalValueBefore")->get<int32_t>());
            ASSERT_EQ(42, *script->getOutputs()->getChild("globalValueAfter")->get<int32_t>());
            tmpLogicEngine.saveToFile("withGlobals.bin");
        }

        ASSERT_TRUE(m_logicEngine.loadFromFile("withGlobals.bin"));
        EXPECT_TRUE(m_logicEngine.update());
        EXPECT_EQ(5, *m_logicEngine.findByName<LuaScript>("withGlobals")->getOutputs()->getChild("globalValueBefore")->get<int32_t>());
        EXPECT_EQ(42, *m_logicEngine.findByName<LuaScript>("withGlobals")->getOutputs()->getChild("globalValueAfter")->get<int32_t>());
    }
}
