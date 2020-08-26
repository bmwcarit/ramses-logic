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

namespace rlogic
{
    class ALuaScript_Types : public ALuaScript
    {
    };

    TEST_F(ALuaScript_Types, FailsToSetValueOfTopLevelInput_WhenTemplateDoesNotMatchDeclaredInputType)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);
        auto  inputs = script->getInputs();

        auto speedInt32  = inputs->getChild("speed");
        auto tempFloat   = inputs->getChild("temp");
        auto nameString  = inputs->getChild("name");
        auto enabledBool = inputs->getChild("enabled");
        auto vec_2f      = inputs->getChild("vec2f");
        auto vec_3f      = inputs->getChild("vec3f");
        auto vec_4f      = inputs->getChild("vec4f");
        auto vec_2i      = inputs->getChild("vec2i");
        auto vec_3i      = inputs->getChild("vec3i");
        auto vec_4i      = inputs->getChild("vec4i");

        EXPECT_FALSE(speedInt32->set<float>(4711.f));
        EXPECT_FALSE(tempFloat->set<int32_t>(4711));
        EXPECT_FALSE(nameString->set<bool>(true));
        EXPECT_FALSE(enabledBool->set<std::string>("some string"));
        EXPECT_FALSE(vec_2f->set<float>(4711.f));
        EXPECT_FALSE(vec_3f->set<float>(4711.f));
        EXPECT_FALSE(vec_4f->set<float>(4711.f));
        EXPECT_FALSE(vec_2i->set<int32_t>(4711));
        EXPECT_FALSE(vec_3i->set<int32_t>(4711));
        EXPECT_FALSE(vec_4i->set<int32_t>(4711));
    }

    TEST_F(ALuaScript_Types, ProvidesIndexBasedAndNameBasedAccessToInputProperties)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);
        ASSERT_NE(nullptr, script);
        auto speed_byIndex = script->getInputs()->getChild(0);
        EXPECT_NE(nullptr, speed_byIndex);
        EXPECT_EQ("speed", speed_byIndex->getName());
        auto speed_byName = script->getInputs()->getChild("speed");
        EXPECT_NE(nullptr, speed_byName);
        EXPECT_EQ("speed", speed_byName->getName());
    }

    TEST_F(ALuaScript_Types, ProvidesIndexBasedAndNameBasedAccessToOutputProperties)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithOutputs);
        ASSERT_NE(nullptr, script);
        auto speed_byIndex = script->getOutputs()->getChild(0);
        EXPECT_NE(nullptr, speed_byIndex);
        EXPECT_EQ("speed", speed_byIndex->getName());
        auto speed_byName = script->getOutputs()->getChild("speed");
        EXPECT_NE(nullptr, speed_byName);
        EXPECT_EQ("speed", speed_byName->getName());
    }

    TEST_F(ALuaScript_Types, AssignsNameAndTypeToGlobalInputsStruct)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScript);

        auto inputs = script->getInputs();

        ASSERT_EQ(0u, inputs->getChildCount());
        EXPECT_EQ("IN", inputs->getName());
        EXPECT_EQ(EPropertyType::Struct, inputs->getType());
    }

    TEST_F(ALuaScript_Types, AssignsNameAndTypeToGlobalOutputsStruct)
    {
        auto* script  = m_logicEngine.createLuaScriptFromSource(m_minimalScript);
        auto  outputs = script->getOutputs();

        ASSERT_EQ(0u, outputs->getChildCount());
        EXPECT_EQ("OUT", outputs->getName());
        EXPECT_EQ(EPropertyType::Struct, outputs->getType());
    }

    TEST_F(ALuaScript_Types, ReturnsItsTopLevelInputsByIndex_IndexEqualsOrderOfDeclaration)
    {
        auto* script = m_logicEngine.createLuaScriptFromSource(m_minimalScriptWithInputs);

        auto inputs = script->getInputs();

        ASSERT_EQ(10u, inputs->getChildCount());
        EXPECT_EQ("speed", inputs->getChild(0)->getName());
        EXPECT_EQ(EPropertyType::Int32, inputs->getChild(0)->getType());
        EXPECT_EQ("temp", inputs->getChild(1)->getName());
        EXPECT_EQ(EPropertyType::Float, inputs->getChild(1)->getType());
        EXPECT_EQ("name", inputs->getChild(2)->getName());
        EXPECT_EQ(EPropertyType::String, inputs->getChild(2)->getType());
        EXPECT_EQ("enabled", inputs->getChild(3)->getName());
        EXPECT_EQ(EPropertyType::Bool, inputs->getChild(3)->getType());

        // Vec2/3/4 f/i
        EXPECT_EQ("vec2f", inputs->getChild(4)->getName());
        EXPECT_EQ(EPropertyType::Vec2f, inputs->getChild(4)->getType());
        EXPECT_EQ("vec3f", inputs->getChild(5)->getName());
        EXPECT_EQ(EPropertyType::Vec3f, inputs->getChild(5)->getType());
        EXPECT_EQ("vec4f", inputs->getChild(6)->getName());
        EXPECT_EQ(EPropertyType::Vec4f, inputs->getChild(6)->getType());
        EXPECT_EQ("vec2i", inputs->getChild(7)->getName());
        EXPECT_EQ(EPropertyType::Vec2i, inputs->getChild(7)->getType());
        EXPECT_EQ("vec3i", inputs->getChild(8)->getName());
        EXPECT_EQ(EPropertyType::Vec3i, inputs->getChild(8)->getType());
        EXPECT_EQ("vec4i", inputs->getChild(9)->getName());
        EXPECT_EQ(EPropertyType::Vec4i, inputs->getChild(9)->getType());
    }

}
