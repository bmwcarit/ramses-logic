//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LuaScriptTest_Base.h"
#include "WithTempDirectory.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "LogTestUtils.h"

#include <fstream>

namespace rlogic
{
    class ALuaScript_Debug : public ALuaScript
    {
    protected:
        std::string_view m_scriptWithInterfaceError = R"(
            function interface(IN,OUT)
                IN.prop = nil
            end
            function run(IN,OUT)
            end
        )";

        std::string_view m_scriptWithRuntimeError = R"(
            function interface(IN,OUT)
            end
            function run(IN,OUT)
                IN.prop = nil
            end
        )";

        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
    };

    TEST_F(ALuaScript_Debug, ProducesErrorWithFullStackTrace_WhenErrorsInInterface)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_scriptWithInterfaceError, {}, "errorscript");

        ASSERT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
                    "[errorscript] Error while loading script. Lua stack trace:\n"
                    "lua: error: Invalid type of field 'prop'! Expected Type:T() syntax where T=Float,Int32,... Found a value of type 'nil' instead"));
        // nullptr because no LogicNode was created
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Debug, ProducesErrorWithFullStackTrace_WhenRuntimeErrors)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_scriptWithRuntimeError, {}, "errorscript");

        ASSERT_NE(nullptr, script);
        m_logicEngine.update();
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
            "lua: error: Tried to access undefined struct property 'prop'"));
        EXPECT_EQ(script, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Debug, ErrorStackTraceContainsScriptName_WhenScriptWasNotLoadedFromFile)
    {
        // Script loaded from string, not file
        LuaScript* script = m_logicEngine.createLuaScript(m_scriptWithInterfaceError, {}, "errorscript");

        // Error message contains script name in the stack (file not known)
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr(
            "[errorscript] Error while loading script. Lua stack trace:\n"
            "lua: error: Invalid type of field 'prop'! Expected Type:T() syntax where T=Float,Int32,... Found a value of type 'nil' instead"));
    }
}
