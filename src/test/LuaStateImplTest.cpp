//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "internals/impl/LuaStateImpl.h"
#include "internals/SolWrapper.h"

namespace rlogic::internal
{
    static std::string_view valid_empty_script = R"(
            function interface()
            end
            function run()
            end
        )";

    TEST(ALuaState, DoesNotHaveErrorsAfterLoadingEmptyScript)
    {
        LuaStateImpl state;

        auto load_result = state.loadScript("", "emptyScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST(ALuaState, HasNoErrorsAfterLoadingValidScript)
    {
        LuaStateImpl state;

        auto load_result = state.loadScript(valid_empty_script, "validEmptryScript");
        EXPECT_TRUE(load_result.valid());
    }

    TEST(ALuaState, DoesNotLoadAScriptWithErrors)
    {
        // TODO consider if we want to make the LuaState a logic engine object and create scripts explicitly on it
        // Use case: when e.g. loading debug symbols which are supposed to be visible to all scripts
        LuaStateImpl state;

        auto load_result = state.loadScript("this.does.not.compile", "cantCompileScript");
        EXPECT_FALSE(load_result.valid());
        sol::error error = load_result;
        EXPECT_THAT(error.what(), ::testing::HasSubstr("'<name>' expected near 'not'"));
    }

    TEST(ALuaState, CanCreateAnEnvironmentOnValidScript)
    {
        LuaStateImpl state;

        auto load_result = state.loadScript(R"(
            function interface()
            end
            function run()
            end
        )", "invalidscript");

        EXPECT_TRUE(load_result.valid());
        sol::protected_function func = load_result;
        auto                    env  = state.createEnvironment(func);
        EXPECT_TRUE(env);
    }

    TEST(ALuaState, CantCreateEnvironmentOnInvalidSCript)
    {
        LuaStateImpl state;
        sol::protected_function func;
        auto                    env  = state.createEnvironment(func);

        EXPECT_FALSE(env);
    }
}
