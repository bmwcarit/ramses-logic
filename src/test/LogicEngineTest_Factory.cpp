//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"

#include "internals/impl/LogicNodeImpl.h"

#include <fstream>

namespace rlogic
{
    class ALogicEngine_Factory : public ALogicEngine
    {
    protected:
        void TearDown() override {
            std::remove("empty.lua");
            std::remove("valid.lua");
        }
    };

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenCreatingEmptyScript)
    {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromSource("");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, FailsToCreateScriptFromFile_WhenFileDoesNotExist)
    {
        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromFile("somefile.txt");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, FailsToLoadsScriptFromEmptyFile)
    {
        std::ofstream ofs;
        ofs.open("empty.lua", std::ofstream::out);
        ofs.close();

        LogicEngine logicEngine;
        auto script = logicEngine.createLuaScriptFromFile("empty.lua");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(logicEngine.getErrors().empty());

        std::remove("empty.lua");
    }

    TEST_F(ALogicEngine_Factory, LoadsScriptFromValidLuaFileWithoutErrors)
    {
        std::ofstream ofs;
        ofs.open("valid.lua", std::ofstream::out);
        ofs << m_valid_empty_script;
        ofs.close();

        LogicEngine logicEngine;
        auto                        script = logicEngine.createLuaScriptFromFile("valid.lua");
        ASSERT_TRUE(nullptr != script);
        EXPECT_TRUE(logicEngine.getErrors().empty());
        // TODO fix this test!
        //EXPECT_EQ("what name do we want here?", script->getName());
        EXPECT_EQ("valid.lua", script->getFilename());

        std::remove("valid.lua");
    }

    TEST_F(ALogicEngine_Factory, DestroysScriptWithoutErrors)
    {
        LogicEngine logicEngine;
        auto                        script = logicEngine.createLuaScriptFromSource(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_TRUE(logicEngine.destroy(*script));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingScriptFromAnotherEngineInstance)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;
        auto                        script = otherLogicEngine.createLuaScriptFromSource(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_FALSE(logicEngine.destroy(*script));
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(logicEngine.getErrors()[0], "Can't find script in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, CreatesRamsesNodeBindingWithoutErrors)
    {
        LogicEngine logicEngine;
        auto                        ramsesNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, DestroysRamsesNodeBindingWithoutErrors)
    {
        LogicEngine logicEngine;
        auto        ramsesNodeBinding = logicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        logicEngine.destroy(*ramsesNodeBinding);
        EXPECT_TRUE(logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherEngineInstance)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;

        auto ramsesNodeBinding = otherLogicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        ASSERT_FALSE(logicEngine.destroy(*ramsesNodeBinding));
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(logicEngine.getErrors()[0], "Can't find RamsesNodeBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, DestroysRamsesAppearanceBindingWithoutErrors)
    {
        LogicEngine logicEngine;
        auto        binding = logicEngine.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(logicEngine.destroy(*binding));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherEngineInstance)
    {
        LogicEngine logicEngine;
        LogicEngine otherLogicEngine;
        auto        binding = otherLogicEngine.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_FALSE(logicEngine.destroy(*binding));
        EXPECT_EQ(logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(logicEngine.getErrors()[0], "Can't find RamsesAppearanceBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorIfWrongObjectTypeIsDestroyed)
    {
        struct UnknownObjectImpl: internal::LogicNodeImpl
        {
            UnknownObjectImpl()
                : LogicNodeImpl("name")
            {
            }

            bool update() override { return true; }
        };

        struct UnknownObject : LogicNode
        {
            explicit UnknownObject(UnknownObjectImpl& impl)
                : LogicNode(impl)
            {
            }
        };

        LogicEngine logicEngine;
        UnknownObjectImpl unknownObjectImpl;
        UnknownObject     unknownObject(unknownObjectImpl);
        EXPECT_FALSE(logicEngine.destroy(unknownObject));
        const auto& errors = logicEngine.getErrors();
        EXPECT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0], "Tried to destroy object 'name' with unknown type");
    }
}
