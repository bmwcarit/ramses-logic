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

#include <fstream>

namespace rlogic
{
    class ALuaScript_Debug : public ALuaScript
    {
    protected:
        void TearDown() override
        {
            std::remove("script.lua");
        }
    };

    TEST_F(ALuaScript_Debug, ProducesErrorWithStackTraceInRun)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                IN.prop = nil
            end
        )", "myscript");

        ASSERT_NE(nullptr, script);
        m_logicEngine.update();
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
                  "lua: error: Tried to access undefined struct property 'prop'\n"
                  "stack traceback:\n"
                  "\t[C]: in ?\n"
                  "\t[string \"myscript\"]:5: in function <[string \"myscript\"]:4>");
    }

    TEST_F(ALuaScript_Debug, ErrorMessageContainsFilenameAndScriptnameWithSemicolonWhenBothAvailable)
    {
        std::ofstream ofs;
        ofs.open("script.lua", std::ofstream::out);
        ofs << R"(
            function interface()
                IN.prop = nil
            end
            function run()
            end
        )";
        ofs.close();

        auto script = m_logicEngine.createLuaScriptFromFile("script.lua", "TheScript");
        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_THAT(m_logicEngine.getErrors()[0], ::testing::HasSubstr("\"script.lua:TheScript\""));
    }

    TEST_F(ALuaScript_Debug, ErrorMessageContainsScriptnameOnly_WhenNotLoadedFromFile)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(R"(
            function interface()
                IN.prop = nil
            end
            function run()
            end
        )", "TheScript");

        EXPECT_EQ(nullptr, script);
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0],
            "lua: error: Field 'prop' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!\n"
            "stack traceback:\n"
            "\t[C]: in ?\n"
            "\t[string \"TheScript\"]:3: in function <[string \"TheScript\"]:2>");
    }

    TEST_F(ALuaScript_Debug, OverridesLuaPrintFunction)
    {
        LogicEngine logicEngine;

        auto script = logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                print("Nice message", "Another message")
            end
        )", "PrintingScript");

        ASSERT_NE(nullptr, script);

        // This is hard to test, but it should at leas not crash
        EXPECT_TRUE(logicEngine.update());
    }

    TEST_F(ALuaScript_Debug, OverridesLuaPrintFunctionWithCustomFunction)
    {
        LogicEngine logicEngine;

        std::vector<std::string> messages;

        auto script = logicEngine.createLuaScriptFromSource(R"(
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

        EXPECT_TRUE(logicEngine.update());

        ASSERT_EQ(4u, messages.size());
        EXPECT_EQ("PrintingScript", messages[0]);
        EXPECT_EQ("Nice message", messages[1]);
        EXPECT_EQ("PrintingScript", messages[2]);
        EXPECT_EQ("Another message", messages[3]);
    }

    TEST_F(ALuaScript_Debug, ProducesErrorIfPrintFunctionIsCalledWithWrongArgument)
    {
        LogicEngine logicEngine;

        std::vector<std::string> messages;

        auto script = logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
                print(42)
            end
        )", "PrintingScript");

        ASSERT_NE(nullptr, script);
        EXPECT_FALSE(logicEngine.update());
        const auto& errors = logicEngine.getErrors();
        EXPECT_EQ(1u, errors.size());
        EXPECT_THAT(errors[0], ::testing::HasSubstr("Called 'print' with wrong argument type 'number'. Only string is allowed"));
    }
}
