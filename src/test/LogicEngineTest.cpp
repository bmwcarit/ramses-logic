//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-logic/LogicEngine.h"
#include "internals/impl/LogicEngineImpl.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"

#include "generated/logicengine_gen.h"

#include "flatbuffers/util.h"

#include <fstream>

namespace rlogic
{
    static const std::string valid_empty_script = R"(
        function interface()
        end
        function run()
        end
    )";

    static const std::string invalid_empty_script = R"(
        )";

    class ALogicEngine : public ::testing::Test
    {
    public:
        void TearDown() override{
            std::remove("valid.lua");
            std::remove("empty.lua");
            std::remove("NoScriptsNoBindings.bin");
            std::remove("NoBindings.bin");
            std::remove("LogicEngine.bin");
        }
    };

    TEST_F(ALogicEngine, ProducesErrorsWhenCreatingEmptyScript)
    {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromSource("");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, FailsToCreateScriptFromFile_WhenFileDoesNotExist)
    {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromFile("somefile.txt");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, FailsToLoadsScriptFromEmptyFile)
    {
        std::ofstream ofs;
        ofs.open("empty.lua", std::ofstream::out);
        ofs.close();

        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromFile("empty.lua");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());

    }

    TEST_F(ALogicEngine, LoadsScriptFromValidLuaFileWithoutErrors)
    {
        std::ofstream ofs;
        ofs.open("valid.lua", std::ofstream::out);
        ofs << valid_empty_script;
        ofs.close();

        LogicEngine logicEngine;
        auto                        script = logicEngine.createLuaScriptFromFile("valid.lua");
        ASSERT_TRUE(nullptr != script);
        EXPECT_TRUE(logicEngine.getErrors().empty());
        // TODO fix this test!
        //EXPECT_EQ("what name do we want here?", script->getName());
        EXPECT_EQ("valid.lua", script->getFilename());
    }

    TEST_F(ALogicEngine, DestroysScriptWithoutErrors)
    {
        LogicEngine logicEngine;
        auto                        script = logicEngine.createLuaScriptFromSource(valid_empty_script.c_str());
        ASSERT_TRUE(script);
        ASSERT_TRUE(logicEngine.destroy(*script));
    }

    TEST_F(ALogicEngine, ProducesErrorsWhenDestroyingScriptFromAnotherEngineInstance)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;
        auto                        script = otherLogicEngine.createLuaScriptFromSource(valid_empty_script.c_str());
        ASSERT_TRUE(script);
        ASSERT_FALSE(logicEngine.destroy(*script));
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(logicEngine.getErrors()[0], "Can't find script in logic engine!");
    }

    TEST_F(ALogicEngine, ClearsErrorsOnCreateNewLuaScript)
    {
        LogicEngine logicEngine;
        auto                        script = logicEngine.createLuaScriptFromFile("somefile.txt");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());

        script = logicEngine.createLuaScriptFromSource(valid_empty_script.c_str());
        ASSERT_NE(nullptr, script);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, ReturnsOnFirstError) {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromSource(invalid_empty_script.c_str());
        ASSERT_EQ(nullptr, script);
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
    }

    TEST_F(ALogicEngine, ClearsErrorsOnUpdate) {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromSource(invalid_empty_script.c_str());
        ASSERT_EQ(nullptr, script);
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);

        EXPECT_TRUE(logicEngine.update());
        EXPECT_EQ(logicEngine.getErrors().size(), 0u);
    }

    TEST_F(ALogicEngine, CreatesRamsesNodeBindingWithoutErrors)
    {
        LogicEngine logicEngine;
        auto                        ramsesNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, DestroysRamsesNodeBindingWithoutErrors)
    {
        LogicEngine logicEngine;
        auto        ramsesNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        logicEngine.destroy(*ramsesNodeBinding);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherEngineInstance)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;

        auto ramsesNodeBinding = otherLogicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        ASSERT_FALSE(logicEngine.destroy(*ramsesNodeBinding));
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(logicEngine.getErrors()[0], "Can't find RamsesNodeBinding in logic engine!");
    }

    TEST_F(ALogicEngine, ClearsErrorsOnCreateNewRamsesNodeBinding)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;
        auto        ramsesNodeBinding = otherLogicEngine.createRamsesNodeBinding("NodeBinding");

        ASSERT_FALSE(logicEngine.destroy(*ramsesNodeBinding));

        EXPECT_FALSE(logicEngine.getErrors().empty());
        auto anotherNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        EXPECT_NE(nullptr, anotherNodeBinding);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine, UpdatesRamsesNodeBindingValuesOnUpdate)
    {
        LogicEngine logicEngine;
        auto        ramsesNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(logicEngine.update());

        // TODO Violin/Sven add expectations here once fully implemented
    }

    TEST_F(ALogicEngine, ProducesErrorIfDeserilizedFromInvalidFile)
    {
        LogicEngine logicEngine;
        EXPECT_FALSE(logicEngine.loadFromFile("invalid"));
        const auto& errors = logicEngine.getErrors();
        ASSERT_EQ(1u, errors.size());
        EXPECT_EQ("Error reading file: invalid", errors[0]);
    }

    TEST_F(ALogicEngine, CanFindAScriptByName)
    {
        LogicEngine logicEngine;
        logicEngine.createLuaScriptFromSource(R"(
            function interface()
            end
            function run()
            end
        )", "MyScript");


        auto script = logicEngine.m_impl->findLuaScriptByName("MyScript");
        ASSERT_NE(nullptr, script);
        EXPECT_EQ("MyScript", script->getName());
    }

    TEST_F(ALogicEngine, CanFindARamsesNodeBindingByName)
    {
        LogicEngine logicEngine;
        logicEngine.createRamsesNodeBinding("NodeBinding");

        auto binding2 = logicEngine.m_impl->findRamsesNodeBindingByName("NodeBinding");
        ASSERT_NE(nullptr, binding2);
        EXPECT_EQ("NodeBinding", binding2->getName());
    }

    TEST_F(ALogicEngine, ProducesNoErrorIfDeserializedWithNoScriptsAndNoNodeBindings)
    {
        {
            LogicEngine logicEngine;
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            LogicEngine logicEngine;
            EXPECT_TRUE(logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(logicEngine.getErrors().empty());
        }
    }

    TEST_F(ALogicEngine, ProducesNoErrorIfDeserializedWithNoScripts)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createRamsesNodeBinding("binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            LogicEngine logicEngine;
            EXPECT_TRUE(logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(logicEngine.getErrors().empty());

            {
                auto rNodeBinding = logicEngine.m_impl->findRamsesNodeBindingByName("binding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
            }
        }
    }

    TEST_F(ALogicEngine, ProducesNoErrorIfDeserilizedWithoutNodeBindings)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            LogicEngine logicEngine;
            EXPECT_TRUE(logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(logicEngine.getErrors().empty());

            {
                auto script = logicEngine.m_impl->findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());
            }
        }
    }

    TEST_F(ALogicEngine, ProducesNoErrorIfDeserilizedSuccessfully)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )", "luascript");

            logicEngine.createRamsesNodeBinding("binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            LogicEngine logicEngine;
            EXPECT_TRUE(logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(logicEngine.getErrors().empty());

            {
                auto script = logicEngine.m_impl->findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                const auto inputs = script->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(1u, inputs->getChildCount());

            }
            {
                auto rNodeBinding = logicEngine.m_impl->findRamsesNodeBindingByName("binding");
                ASSERT_NE(nullptr, rNodeBinding);
                const auto inputs = rNodeBinding->getInputs();
                ASSERT_NE(nullptr, inputs);
                EXPECT_EQ(4u, inputs->getChildCount());
            }
        }
    }

    TEST_F(ALogicEngine, ReplacesCurrentStateWithStateFromFile)
    {
        {
            LogicEngine logicEngine;
            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param = INT
                end
                function run()
                end
            )","luascript");

            logicEngine.createRamsesNodeBinding("binding");
            logicEngine.saveToFile("LogicEngine.bin");
        }
        {
            LogicEngine logicEngine;

            logicEngine.createLuaScriptFromSource(R"(
                function interface()
                    IN.param2 = FLOAT
                end
                function run()
                end
            )", "luascript2");

            logicEngine.createRamsesNodeBinding("binding2");
            EXPECT_TRUE(logicEngine.loadFromFile("LogicEngine.bin"));
            EXPECT_TRUE(logicEngine.getErrors().empty());

            {
                ASSERT_EQ(nullptr, logicEngine.m_impl->findLuaScriptByName("luascript2"));
                ASSERT_EQ(nullptr, logicEngine.m_impl->findRamsesNodeBindingByName("binding2"));

                auto script = logicEngine.m_impl->findLuaScriptByName("luascript");
                ASSERT_NE(nullptr, script);
                auto rNodeBinding = logicEngine.m_impl->findRamsesNodeBindingByName("binding");
                ASSERT_NE(nullptr, rNodeBinding);
            }
        }
    }

}

