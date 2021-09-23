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
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_scriptWithInterfaceError, "errorscript");

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
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_scriptWithRuntimeError, "errorscript");

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

    TEST_F(ALuaScript_Debug, ErrorMessageContainsFilenameAndScriptnameWithSemicolonWhenBothAvailable)
    {
        WithTempDirectory tempFolder;

        std::ofstream ofs;
        ofs.open("script.lua", std::ofstream::out);
        ofs << m_scriptWithInterfaceError;
        ofs.close();

        auto script = m_logicEngine.createLuaScriptFromFile("script.lua", "errorscript");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0].message, ::testing::HasSubstr("\t[string \"script.lua:errorscript\"]:3: in function <[string \"script.lua:errorscript\"]:2>"));
    }

    TEST_F(ALuaScript_Debug, ErrorStackTraceContainsScriptName_WhenScriptWasNotLoadedFromFile)
    {
        // Script loaded from string, not file
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_scriptWithInterfaceError, "errorscript");

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

    // Logic engine always overrides the print function internally - test that it doesn't cause crashes
    TEST_F(ALuaScript_Debug, DefaultOverrideOfLuaPrintFunctionDoesNotCrash)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                print("Nice message", "Another message")
            end
        )", "PrintingScript");

        ASSERT_NE(nullptr, script);

        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ALuaScript_Debug, OverridesLuaPrintFunctionWithCustomFunction)
    {
        std::vector<std::string> messages;

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                print("Nice message", "Another message")
            end
        )", "PrintingScript");

        ASSERT_NE(nullptr, script);

        script->overrideLuaPrint([&messages](std::string_view scriptName, std::string_view message) {
            messages.emplace_back(scriptName);
            messages.emplace_back(message);
            });

        EXPECT_TRUE(m_logicEngine.update());

        ASSERT_EQ(4u, messages.size());
        EXPECT_EQ("PrintingScript", messages[0]);
        EXPECT_EQ("Nice message", messages[1]);
        EXPECT_EQ("PrintingScript", messages[2]);
        EXPECT_EQ("Another message", messages[3]);
    }

    TEST_F(ALuaScript_Debug, ProducesErrorIfPrintFunctionIsCalledWithWrongArgument)
    {
        std::vector<std::string> messages;

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                print(42)
            end
        )", "PrintingScript");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(m_logicEngine.update());
        const auto& errors = m_logicEngine.getErrors();
        EXPECT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0].message, ::testing::HasSubstr("Called 'print' with wrong argument type 'number'. Only string is allowed"));
        EXPECT_EQ(script, errors[0].object);
    }
}
