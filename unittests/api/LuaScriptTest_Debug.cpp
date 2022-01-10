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
            function interface()
                IN.prop = nil
            end
            function run()
            end
        )";

        std::string_view m_scriptWithRuntimeError = R"(
            function interface()
            end
            function run()
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
        EXPECT_EQ(m_logicEngine.getErrors()[0].message,
                  "[errorscript] Error while loading script. Lua stack trace:\n"
                  "lua: error: Field 'prop' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!\n"
                  "stack traceback:\n"
                  "\t[C]: in ?\n"
                  "\t[string \"errorscript\"]:3: in function <[string \"errorscript\"]:2>");
        // nullptr because no LogicNode was created
        EXPECT_EQ(nullptr, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Debug, ProducesErrorWithFullStackTrace_WhenRuntimeErrors)
    {
        LuaScript* script = m_logicEngine.createLuaScript(m_scriptWithRuntimeError, {}, "errorscript");

        ASSERT_NE(nullptr, script);
        m_logicEngine.update();
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message,
            "lua: error: Tried to access undefined struct property 'prop'\n"
            "stack traceback:\n"
            "\t[C]: in ?\n"
            "\t[string \"errorscript\"]:5: in function <[string \"errorscript\"]:4>");
        EXPECT_EQ(script, m_logicEngine.getErrors()[0].object);
    }

    TEST_F(ALuaScript_Debug, ErrorStackTraceContainsScriptName_WhenScriptWasNotLoadedFromFile)
    {
        // Script loaded from string, not file
        LuaScript* script = m_logicEngine.createLuaScript(m_scriptWithInterfaceError, {}, "errorscript");

        // Error message contains script name in the stack (file not known)
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message,
            "[errorscript] Error while loading script. Lua stack trace:\n"
            "lua: error: Field 'prop' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!\n"
            "stack traceback:\n"
            "\t[C]: in ?\n"
            "\t[string \"errorscript\"]:3: in function <[string \"errorscript\"]:2>");
    }
}
