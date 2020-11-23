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
    };

    TEST_F(ASolState, DoesNotHaveErrorsAfterLoadingEmptyScript)
    {

        auto load_result = m_solState.loadScript("", "emptyScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST_F(ASolState, HasNoErrorsAfterLoadingValidScript)
    {
        const std::string_view valid_empty_script = R"(
                function interface()
                end
                function run()
                end
            )";

        auto load_result = m_solState.loadScript(valid_empty_script, "validEmptryScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST_F(ASolState, DoesNotLoadAScriptWithErrors)
    {
        auto load_result = m_solState.loadScript("this.does.not.compile", "cantCompileScript");
        EXPECT_FALSE(load_result.valid());
        sol::error error = load_result;
        EXPECT_THAT(error.what(), ::testing::HasSubstr("'<name>' expected near 'not'"));
    }

    TEST_F(ASolState, CanCreateAnEnvironmentOnValidScript)
    {
        auto load_result = m_solState.loadScript(R"(
            function interface()
            end
            function run()
            end
        )", "invalidscript");

        EXPECT_TRUE(load_result.valid());
        sol::protected_function func = load_result;
        auto                    env  = m_solState.createEnvironment(func);
        EXPECT_TRUE(env);
    }

    TEST_F(ASolState, CantCreateEnvironmentOnInvalidSCript)
    {
        sol::protected_function func;
        auto                    env  = m_solState.createEnvironment(func);

        EXPECT_FALSE(env);
    }
}
