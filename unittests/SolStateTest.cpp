//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "internals/SolState.h"
#include "internals/SolWrapper.h"
#include "internals/LuaCompilationUtils.h"
#include "internals/ErrorReporting.h"

namespace rlogic::internal
{
    class ASolState : public ::testing::Test
    {
        protected:
            SolState m_solState;

            const std::string_view m_valid_empty_script = R"(
                function interface()
                end
                function run()
                end
            )";
    };

    TEST_F(ASolState, DoesNotHaveErrorsAfterLoadingEmptyScript)
    {

        auto load_result = m_solState.loadScript("", "emptyScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST_F(ASolState, HasNoErrorsAfterLoadingValidScript)
    {
        auto load_result = m_solState.loadScript(m_valid_empty_script, "validEmptryScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST_F(ASolState, DoesNotLoadAScriptWithErrors)
    {
        auto load_result = m_solState.loadScript("this.does.not.compile", "cantCompileScript");
        EXPECT_FALSE(load_result.valid());
        sol::error error = load_result;
        EXPECT_THAT(error.what(), ::testing::HasSubstr("'<name>' expected near 'not'"));
    }

    TEST_F(ASolState, CreatesNewEnvironment)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        EXPECT_TRUE(env.valid());
    }

    // TODO Violin currently exposed so that init() can access them. Try if we can isolate even more
    // to have nothing in the global environment
    TEST_F(ASolState, NewEnvironment_ExposesTypeSymbols)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_TRUE(env["INT"].valid());
        EXPECT_TRUE(env["FLOAT"].valid());
        EXPECT_TRUE(env["STRING"].valid());
        EXPECT_TRUE(env["BOOL"].valid());
        EXPECT_TRUE(env["ARRAY"].valid());

        EXPECT_FALSE(env["IN"].valid());
        EXPECT_FALSE(env["OUT"].valid());
    }

    TEST_F(ASolState, CreatesCustomMethods)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_TRUE(env["modules"].valid());
        EXPECT_TRUE(env["rl_len"].valid());
    }

    // Those are created on-demand in the interface() function and during runtime
    TEST_F(ASolState, NewEnvironment_HasNo_IN_OUT_globals)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_FALSE(env["IN"].valid());
        EXPECT_FALSE(env["OUT"].valid());
    }

    TEST_F(ASolState, NewEnvironment_HidesGlobalStandardModulesByDefault)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_FALSE(env["print"].valid());
        EXPECT_FALSE(env["debug"].valid());
        EXPECT_FALSE(env["string"].valid());
        EXPECT_FALSE(env["table"].valid());
        EXPECT_FALSE(env["error"].valid());
        EXPECT_FALSE(env["math"].valid());
    }

    TEST_F(ASolState, NewEnvironment_ExposesOnlyRequestedGlobalStandardModules)
    {
        sol::environment env = m_solState.createEnvironment({EStandardModule::Math}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_TRUE(env["math"].valid());

        EXPECT_FALSE(env["print"].valid());
        EXPECT_FALSE(env["debug"].valid());
        EXPECT_FALSE(env["string"].valid());
        EXPECT_FALSE(env["table"].valid());
        EXPECT_FALSE(env["error"].valid());
    }

    TEST_F(ASolState, NewEnvironment_ExposesRequestedGlobalStandardModules_TwoModules)
    {
        sol::environment env = m_solState.createEnvironment({ EStandardModule::String, EStandardModule::Table }, {});
        ASSERT_TRUE(env.valid());

        EXPECT_TRUE(env["string"].valid());
        EXPECT_TRUE(env["table"].valid());

        EXPECT_FALSE(env["math"].valid());
        EXPECT_FALSE(env["print"].valid());
        EXPECT_FALSE(env["debug"].valid());
        EXPECT_FALSE(env["error"].valid());
    }

    TEST_F(ASolState, NewEnvironment_ExposesRequestedGlobalStandardModules_BaseLib)
    {
        sol::environment env = m_solState.createEnvironment({ EStandardModule::Base }, {});
        ASSERT_TRUE(env.valid());

        EXPECT_TRUE(env["error"].valid());
        EXPECT_TRUE(env["tostring"].valid());
        EXPECT_TRUE(env["print"].valid());

        EXPECT_FALSE(env["table"].valid());
        EXPECT_FALSE(env["math"].valid());
        EXPECT_FALSE(env["debug"].valid());
        EXPECT_FALSE(env["string"].valid());
    }

    TEST_F(ASolState, NewEnvironment_HasNoFunctionsExpectedByUserScript)
    {
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());

        EXPECT_FALSE(env["interface"].valid());
        EXPECT_FALSE(env["run"].valid());
    }

    TEST_F(ASolState, NewEnvironment_TwoEnvironmentsShareNoData)
    {
        sol::environment env1 = m_solState.createEnvironment({}, {});
        sol::environment env2 = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env1.valid());
        ASSERT_TRUE(env2.valid());

        env1["thisBelongsTo"] = "env1";
        env2["thisBelongsTo"] = "env2";

        const std::string data1 = env1["thisBelongsTo"];
        const std::string data2 = env2["thisBelongsTo"];

        EXPECT_EQ(data1, "env1");
        EXPECT_EQ(data2, "env2");
    }

    TEST_F(ASolState, NewlyCreatedEnvironment_HasNoAccessToPreviouslyDeclaredGlobalSymbols)
    {
        const std::string_view script = R"(
            global= "this is global"
            function func()
                return global
            end
            return func
        )";

        // Execute the script and obtain the func pointer 'func'
        sol::protected_function loadedScript = m_solState.loadScript(script, "test script");
        sol::function func = loadedScript();

        // Apply fresh environment to func
        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());
        env.set_on(func);

        // Func has no access to 'global' because it was defined _before_ applying the new environment
        sol::object result = func();
        EXPECT_EQ(result, sol::nil);
    }

    // Similar to NewlyCreatedEnvironment_HasNoAccessToPreviouslyDeclaredGlobalSymbols
    // But here the environment is applied before global symbols are declared -> access to those is available
    TEST_F(ASolState, NewlyCreatedEnvironment_HasAccessToGlobalSymbols_DeclaredAfterApplyingTheEnvironment)
    {
        const std::string_view script = R"(
            global = "this is global"
            function func()
                return global
            end
            return func
        )";

        sol::protected_function loadedScript = m_solState.loadScript(script, "test script");

        // Apply a fresh environments to loaded script _before_ executing it
        sol::environment env = m_solState.createEnvironment({}, {});
        env.set_on(loadedScript);
        sol::function func = loadedScript();

        // Can access global symbol, because it lives in the new environment
        const std::string result = func();
        EXPECT_EQ(result, "this is global");
    }

    TEST_F(ASolState, NewEnvironment_OverridesEnvironmentOfScript_AfterAppliedOnIt)
    {
        const std::string_view reportData = R"(
                if data ~= nil then
                    return "data: " .. data
                else
                    return "no data"
                end
            )";

        sol::protected_function script = m_solState.loadScript(reportData, "test script");

        std::string dataStatus = script();
        EXPECT_EQ(dataStatus, "no data");

        sol::environment env = m_solState.createEnvironment({}, {});
        ASSERT_TRUE(env.valid());
        env["data"] = "a lot of data!";

        env.set_on(script);

        dataStatus = script();
        EXPECT_EQ(dataStatus, "data: a lot of data!");
    }
}
