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
        sol::environment env = m_solState.createEnvironment();
        EXPECT_TRUE(env.valid());
    }

    TEST_F(ASolState, NewEnvironment_InheritsGlobals)
    {
        sol::environment env = m_solState.createEnvironment();
        ASSERT_TRUE(env.valid());

        // Libs
        EXPECT_TRUE(env["print"].valid());
        EXPECT_TRUE(env["math"].valid());

        // Our defined symbols
        EXPECT_TRUE(env["INT"].valid());
        EXPECT_TRUE(env["STRING"].valid());
    }

    // Those are created at a later point of the script lifecycle
    TEST_F(ASolState, NewEnvironment_HasNo_IN_OUT_globalsYet)
    {
        sol::environment env = m_solState.createEnvironment();
        ASSERT_TRUE(env.valid());

        EXPECT_FALSE(env["IN"].valid());
        EXPECT_FALSE(env["OUT"].valid());
    }

    TEST_F(ASolState, NewEnvironment_HasNoFunctionsExpectedByUserScript)
    {
        sol::environment env = m_solState.createEnvironment();
        ASSERT_TRUE(env.valid());

        EXPECT_FALSE(env["interface"].valid());
        EXPECT_FALSE(env["run"].valid());
    }

    TEST_F(ASolState, NewEnvironment_TwoEnvironmentsShareNoData)
    {
        sol::environment env1 = m_solState.createEnvironment();
        sol::environment env2 = m_solState.createEnvironment();
        ASSERT_TRUE(env1.valid());
        ASSERT_TRUE(env2.valid());

        env1["thisBelongsTo"] = "env1";
        env2["thisBelongsTo"] = "env2";

        const std::string data1 = env1["thisBelongsTo"];
        const std::string data2 = env2["thisBelongsTo"];

        EXPECT_EQ(data1, "env1");
        EXPECT_EQ(data2, "env2");
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

        sol::environment env = m_solState.createEnvironment();
        ASSERT_TRUE(env.valid());
        env["data"] = "a lot of data!";

        env.set_on(script);

        dataStatus = script();
        EXPECT_EQ(dataStatus, "data: a lot of data!");
    }
}
