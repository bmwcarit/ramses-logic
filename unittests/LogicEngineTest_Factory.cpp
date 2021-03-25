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

#include "impl/LogicNodeImpl.h"

#include "WithTempDirectory.h"
#include <fstream>

namespace rlogic
{
    class ALogicEngine_Factory : public ALogicEngine
    {
    protected:
        WithTempDirectory tempFolder;
    };

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenCreatingEmptyScript)
    {
        auto script = m_logicEngine.createLuaScriptFromSource("");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, FailsToCreateScriptFromFile_WhenFileDoesNotExist)
    {
        auto script = m_logicEngine.createLuaScriptFromFile("somefile.txt");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, FailsToLoadsScriptFromEmptyFile)
    {
        std::ofstream ofs;
        ofs.open("empty.lua", std::ofstream::out);
        ofs.close();

        auto script = m_logicEngine.createLuaScriptFromFile("empty.lua");
        ASSERT_EQ(nullptr, script);
        EXPECT_FALSE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, LoadsScriptFromValidLuaFileWithoutErrors)
    {
        std::ofstream ofs;
        ofs.open("valid.lua", std::ofstream::out);
        ofs << m_valid_empty_script;
        ofs.close();

        auto script = m_logicEngine.createLuaScriptFromFile("valid.lua");
        ASSERT_TRUE(nullptr != script);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
        // TODO Violin fix this test!
        //EXPECT_EQ("what name do we want here?", script->getName());
        EXPECT_EQ("valid.lua", script->getFilename());
    }

    TEST_F(ALogicEngine_Factory, DestroysScriptWithoutErrors)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_TRUE(m_logicEngine.destroy(*script));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingScriptFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;
        auto script = otherLogicEngine.createLuaScriptFromSource(m_valid_empty_script);
        ASSERT_TRUE(script);
        ASSERT_FALSE(m_logicEngine.destroy(*script));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Can't find script in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, CreatesRamsesNodeBindingWithoutErrors)
    {
        auto ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, DestroysRamsesNodeBindingWithoutErrors)
    {
        auto ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        m_logicEngine.destroy(*ramsesNodeBinding);
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;

        auto ramsesNodeBinding = otherLogicEngine.createRamsesNodeBinding("NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        ASSERT_FALSE(m_logicEngine.destroy(*ramsesNodeBinding));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Can't find RamsesNodeBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, DestroysRamsesAppearanceBindingWithoutErrors)
    {
        auto binding = m_logicEngine.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(m_logicEngine.destroy(*binding));
    }

    TEST_F(ALogicEngine_Factory, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherEngineInstance)
    {
        LogicEngine otherLogicEngine;
        auto binding = otherLogicEngine.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_FALSE(m_logicEngine.destroy(*binding));
        EXPECT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0], "Can't find RamsesAppearanceBinding in logic engine!");
    }

    TEST_F(ALogicEngine_Factory, RenamesObjectsAfterCreation)
    {
        auto script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "");
        auto ramsesNodeBinding = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        auto ramsesAppearanceBinding = m_logicEngine.createRamsesAppearanceBinding("AppearanceBinding");

        script->setName("same name twice");
        ramsesNodeBinding->setName("same name twice");
        ramsesAppearanceBinding->setName("");

        EXPECT_EQ("same name twice", script->getName());
        EXPECT_EQ("same name twice", ramsesNodeBinding->getName());
        EXPECT_EQ("", ramsesAppearanceBinding->getName());
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

        UnknownObjectImpl unknownObjectImpl;
        UnknownObject     unknownObject(unknownObjectImpl);
        EXPECT_FALSE(m_logicEngine.destroy(unknownObject));
        const auto& errors = m_logicEngine.getErrors();
        EXPECT_EQ(1u, errors.size());
        EXPECT_EQ(errors[0], "Tried to destroy object 'name' with unknown type");
    }
}
