//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

namespace rlogic
{
    class ALogicEngine_Lookup : public ALogicEngine
    {
    };

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName)
    {
        LuaModule* luaModule = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding* cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "dataarray");
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", dataArray, dataArray } }, "animNode");
        const auto timerNode = m_logicEngine.createTimerNode("timerNode");

        EXPECT_EQ(luaModule, m_logicEngine.findByName<LuaModule>("luaModule"));
        EXPECT_EQ(script, m_logicEngine.findByName<LuaScript>("script"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findByName<RamsesNodeBinding>("nodebinding"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findByName<RamsesAppearanceBinding>("appbinding"));
        EXPECT_EQ(cameraBinding, m_logicEngine.findByName<RamsesCameraBinding>("camerabinding"));
        EXPECT_EQ(dataArray, m_logicEngine.findByName<DataArray>("dataarray"));
        EXPECT_EQ(animNode, m_logicEngine.findByName<AnimationNode>("animNode"));
        EXPECT_EQ(timerNode, m_logicEngine.findByName<TimerNode>("timerNode"));

        EXPECT_EQ(luaModule, m_logicEngine.findByName<LogicObject>("luaModule"));
        EXPECT_EQ(script, m_logicEngine.findByName<LogicObject>("script"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findByName<LogicObject>("nodebinding"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findByName<LogicObject>("appbinding"));
        EXPECT_EQ(cameraBinding, m_logicEngine.findByName<LogicObject>("camerabinding"));
        EXPECT_EQ(dataArray, m_logicEngine.findByName<LogicObject>("dataarray"));
        EXPECT_EQ(animNode, m_logicEngine.findByName<LogicObject>("animNode"));
        EXPECT_EQ(timerNode, m_logicEngine.findByName<LogicObject>("timerNode"));

        auto it = m_logicEngine.getCollection<LogicObject>().cbegin();
        EXPECT_EQ(*it++, luaModule);
        EXPECT_EQ(*it++, script);
        EXPECT_EQ(*it++, nodeBinding);
        EXPECT_EQ(*it++, appearanceBinding);
        EXPECT_EQ(*it++, cameraBinding);
        EXPECT_EQ(*it++, dataArray);
        EXPECT_EQ(*it++, animNode);
        EXPECT_EQ(*it++, timerNode);
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_Const)
    {
        LuaModule* luaModule = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding* cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "dataarray");
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", dataArray, dataArray } }, "animNode");
        const auto timerNode = m_logicEngine.createTimerNode("timerNode");

        const LogicEngine& immutableLogicEngine = m_logicEngine;
        EXPECT_EQ(luaModule, immutableLogicEngine.findByName<LuaModule>("luaModule"));
        EXPECT_EQ(script, immutableLogicEngine.findByName<LuaScript>("script"));
        EXPECT_EQ(nodeBinding, immutableLogicEngine.findByName<RamsesNodeBinding>("nodebinding"));
        EXPECT_EQ(appearanceBinding, immutableLogicEngine.findByName<RamsesAppearanceBinding>("appbinding"));
        EXPECT_EQ(cameraBinding, immutableLogicEngine.findByName<RamsesCameraBinding>("camerabinding"));
        EXPECT_EQ(dataArray, immutableLogicEngine.findByName<DataArray>("dataarray"));
        EXPECT_EQ(animNode, immutableLogicEngine.findByName<AnimationNode>("animNode"));
        EXPECT_EQ(timerNode, immutableLogicEngine.findByName<TimerNode>("timerNode"));

        EXPECT_EQ(luaModule, immutableLogicEngine.findByName<LogicObject>("luaModule"));
        EXPECT_EQ(script, immutableLogicEngine.findByName<LogicObject>("script"));
        EXPECT_EQ(nodeBinding, immutableLogicEngine.findByName<LogicObject>("nodebinding"));
        EXPECT_EQ(appearanceBinding, immutableLogicEngine.findByName<LogicObject>("appbinding"));
        EXPECT_EQ(cameraBinding, immutableLogicEngine.findByName<LogicObject>("camerabinding"));
        EXPECT_EQ(dataArray, immutableLogicEngine.findByName<LogicObject>("dataarray"));
        EXPECT_EQ(animNode, immutableLogicEngine.findByName<LogicObject>("animNode"));
        EXPECT_EQ(timerNode, immutableLogicEngine.findByName<LogicObject>("timerNode"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_CanBeUsedWithRealType)
    {
        m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        m_logicEngine.createTimerNode("timerNode");

        const auto* luaModuleFound         = m_logicEngine.findByName<LogicObject>("luaModule")->as<LuaModule>();
        const auto* luaScriptFound         = m_logicEngine.findByName<LogicObject>("script")->as<LuaScript>();
        const auto* nodeBindingFound       = m_logicEngine.findByName<LogicObject>("nodebinding")->as<RamsesNodeBinding>();
        const auto* appearanceBindingFound = m_logicEngine.findByName<LogicObject>("appbinding")->as<RamsesAppearanceBinding>();
        const auto* cameraBindingFound     = m_logicEngine.findByName<LogicObject>("camerabinding")->as<RamsesCameraBinding>();
        const auto* dataArrayFound         = m_logicEngine.findByName<LogicObject>("dataarray")->as<DataArray>();
        const auto* animNodeFound          = m_logicEngine.findByName<LogicObject>("animNode")->as<AnimationNode>();
        const auto* timerNodeFound         = m_logicEngine.findByName<LogicObject>("timerNode")->as<TimerNode>();

        ASSERT_NE(nullptr, luaModuleFound);
        ASSERT_NE(nullptr, luaScriptFound);
        ASSERT_NE(nullptr, nodeBindingFound);
        ASSERT_NE(nullptr, appearanceBindingFound);
        ASSERT_NE(nullptr, cameraBindingFound);
        ASSERT_NE(nullptr, dataArrayFound);
        ASSERT_NE(nullptr, animNodeFound);
        ASSERT_NE(nullptr, timerNodeFound);

        EXPECT_EQ(luaModuleFound->getName(), "luaModule");
        EXPECT_EQ(luaScriptFound->getName(), "script");
        EXPECT_EQ(nodeBindingFound->getName(), "nodebinding");
        EXPECT_EQ(appearanceBindingFound->getName(), "appbinding");
        EXPECT_EQ(cameraBindingFound->getName(), "camerabinding");
        EXPECT_EQ(dataArrayFound->getName(), "dataarray");
        EXPECT_EQ(animNodeFound->getName(), "animNode");
        EXPECT_EQ(timerNodeFound->getName(), "timerNode");
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_CanBeUsedAsRealType_Const)
    {
        m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        m_logicEngine.createTimerNode("timerNode");

        const LogicEngine& immutableLogicEngine   = m_logicEngine;
        const auto*        luaModuleFound         = immutableLogicEngine.findByName<LogicObject>("luaModule")->as<LuaModule>();
        const auto*        luaScriptFound         = immutableLogicEngine.findByName<LogicObject>("script")->as<LuaScript>();
        const auto*        nodeBindingFound       = immutableLogicEngine.findByName<LogicObject>("nodebinding")->as<RamsesNodeBinding>();
        const auto*        appearanceBindingFound = immutableLogicEngine.findByName<LogicObject>("appbinding")->as<RamsesAppearanceBinding>();
        const auto*        cameraBindingFound     = immutableLogicEngine.findByName<LogicObject>("camerabinding")->as<RamsesCameraBinding>();
        const auto*        dataArrayFound         = immutableLogicEngine.findByName<LogicObject>("dataarray")->as<DataArray>();
        const auto*        animNodeFound          = immutableLogicEngine.findByName<LogicObject>("animNode")->as<AnimationNode>();
        const auto*        timerNodeFound         = immutableLogicEngine.findByName<LogicObject>("timerNode")->as<TimerNode>();

        ASSERT_NE(nullptr, luaModuleFound);
        ASSERT_NE(nullptr, luaScriptFound);
        ASSERT_NE(nullptr, nodeBindingFound);
        ASSERT_NE(nullptr, appearanceBindingFound);
        ASSERT_NE(nullptr, cameraBindingFound);
        ASSERT_NE(nullptr, dataArrayFound);
        ASSERT_NE(nullptr, animNodeFound);
        ASSERT_NE(nullptr, timerNodeFound);

        EXPECT_EQ(luaModuleFound->getName(), "luaModule");
        EXPECT_EQ(luaScriptFound->getName(), "script");
        EXPECT_EQ(nodeBindingFound->getName(), "nodebinding");
        EXPECT_EQ(appearanceBindingFound->getName(), "appbinding");
        EXPECT_EQ(cameraBindingFound->getName(), "camerabinding");
        EXPECT_EQ(dataArrayFound->getName(), "dataarray");
        EXPECT_EQ(animNodeFound->getName(), "animNode");
        EXPECT_EQ(timerNodeFound->getName(), "timerNode");
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirId)
    {
        LuaModule*               luaModule         = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        LuaScript*               script            = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        RamsesNodeBinding*       nodeBinding       = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding*     cameraBinding     = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto               dataArray         = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        const auto               animNode          = m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        const auto               timerNode         = m_logicEngine.createTimerNode("timerNode");

        EXPECT_EQ(luaModule, m_logicEngine.findLogicObjectById(1u));
        EXPECT_EQ(script, m_logicEngine.findLogicObjectById(2u));
        EXPECT_EQ(nodeBinding, m_logicEngine.findLogicObjectById(3u));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findLogicObjectById(4u));
        EXPECT_EQ(cameraBinding, m_logicEngine.findLogicObjectById(5u));
        EXPECT_EQ(dataArray, m_logicEngine.findLogicObjectById(6u));
        EXPECT_EQ(animNode, m_logicEngine.findLogicObjectById(7u));
        EXPECT_EQ(timerNode, m_logicEngine.findLogicObjectById(8u));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirId_Const)
    {
        LuaModule*               luaModule         = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        LuaScript*               script            = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        RamsesNodeBinding*       nodeBinding       = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding*     cameraBinding     = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto               dataArray         = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        const auto               animNode          = m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        const auto               timerNode         = m_logicEngine.createTimerNode("timerNode");

        const LogicEngine& immutableLogicEngine = m_logicEngine;
        EXPECT_EQ(luaModule, immutableLogicEngine.findLogicObjectById(1u));
        EXPECT_EQ(script, immutableLogicEngine.findLogicObjectById(2u));
        EXPECT_EQ(nodeBinding, immutableLogicEngine.findLogicObjectById(3u));
        EXPECT_EQ(appearanceBinding, immutableLogicEngine.findLogicObjectById(4u));
        EXPECT_EQ(cameraBinding, immutableLogicEngine.findLogicObjectById(5u));
        EXPECT_EQ(dataArray, immutableLogicEngine.findLogicObjectById(6u));
        EXPECT_EQ(animNode, immutableLogicEngine.findLogicObjectById(7u));
        EXPECT_EQ(timerNode, immutableLogicEngine.findLogicObjectById(8u));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirId_CanBeUsedWithRealType)
    {
        m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        m_logicEngine.createTimerNode("timerNode");

        const auto* luaModuleFound         = m_logicEngine.findLogicObjectById(1u)->as<LuaModule>();
        const auto* luaScriptFound         = m_logicEngine.findLogicObjectById(2u)->as<LuaScript>();
        const auto* nodeBindingFound       = m_logicEngine.findLogicObjectById(3u)->as<RamsesNodeBinding>();
        const auto* appearanceBindingFound = m_logicEngine.findLogicObjectById(4u)->as<RamsesAppearanceBinding>();
        const auto* cameraBindingFound     = m_logicEngine.findLogicObjectById(5u)->as<RamsesCameraBinding>();
        const auto* dataArrayFound         = m_logicEngine.findLogicObjectById(6u)->as<DataArray>();
        const auto* animNodeFound          = m_logicEngine.findLogicObjectById(7u)->as<AnimationNode>();
        const auto* timerNodeFound         = m_logicEngine.findLogicObjectById(8u)->as<TimerNode>();

        ASSERT_NE(nullptr, luaModuleFound);
        ASSERT_NE(nullptr, luaScriptFound);
        ASSERT_NE(nullptr, nodeBindingFound);
        ASSERT_NE(nullptr, appearanceBindingFound);
        ASSERT_NE(nullptr, cameraBindingFound);
        ASSERT_NE(nullptr, dataArrayFound);
        ASSERT_NE(nullptr, animNodeFound);
        ASSERT_NE(nullptr, timerNodeFound);

        EXPECT_EQ(luaModuleFound->getName(), "luaModule");
        EXPECT_EQ(luaScriptFound->getName(), "script");
        EXPECT_EQ(nodeBindingFound->getName(), "nodebinding");
        EXPECT_EQ(appearanceBindingFound->getName(), "appbinding");
        EXPECT_EQ(cameraBindingFound->getName(), "camerabinding");
        EXPECT_EQ(dataArrayFound->getName(), "dataarray");
        EXPECT_EQ(animNodeFound->getName(), "animNode");
        EXPECT_EQ(timerNodeFound->getName(), "timerNode");
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirId_CanBeUsedAsRealType_Const)
    {
        m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataarray");
        m_logicEngine.createAnimationNode({{"channel", dataArray, dataArray}}, "animNode");
        m_logicEngine.createTimerNode("timerNode");

        const LogicEngine& immutableLogicEngine   = m_logicEngine;
        const auto*        luaModuleFound         = immutableLogicEngine.findLogicObjectById(1u)->as<LuaModule>();
        const auto*        luaScriptFound         = immutableLogicEngine.findLogicObjectById(2u)->as<LuaScript>();
        const auto*        nodeBindingFound       = immutableLogicEngine.findLogicObjectById(3u)->as<RamsesNodeBinding>();
        const auto*        appearanceBindingFound = immutableLogicEngine.findLogicObjectById(4u)->as<RamsesAppearanceBinding>();
        const auto*        cameraBindingFound     = immutableLogicEngine.findLogicObjectById(5u)->as<RamsesCameraBinding>();
        const auto*        dataArrayFound         = immutableLogicEngine.findLogicObjectById(6u)->as<DataArray>();
        const auto*        animNodeFound          = immutableLogicEngine.findLogicObjectById(7u)->as<AnimationNode>();
        const auto*        timerNodeFound         = immutableLogicEngine.findLogicObjectById(8u)->as<TimerNode>();

        ASSERT_NE(nullptr, luaModuleFound);
        ASSERT_NE(nullptr, luaScriptFound);
        ASSERT_NE(nullptr, nodeBindingFound);
        ASSERT_NE(nullptr, appearanceBindingFound);
        ASSERT_NE(nullptr, cameraBindingFound);
        ASSERT_NE(nullptr, dataArrayFound);
        ASSERT_NE(nullptr, animNodeFound);
        ASSERT_NE(nullptr, timerNodeFound);

        EXPECT_EQ(luaModuleFound->getName(), "luaModule");
        EXPECT_EQ(luaScriptFound->getName(), "script");
        EXPECT_EQ(nodeBindingFound->getName(), "nodebinding");
        EXPECT_EQ(appearanceBindingFound->getName(), "appbinding");
        EXPECT_EQ(cameraBindingFound->getName(), "camerabinding");
        EXPECT_EQ(dataArrayFound->getName(), "dataarray");
        EXPECT_EQ(animNodeFound->getName(), "animNode");
        EXPECT_EQ(timerNodeFound->getName(), "timerNode");
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_CutsNameAtNullTermination)
    {
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        EXPECT_EQ(appearanceBinding, m_logicEngine.findByName<RamsesAppearanceBinding>("appbinding\0withsurprise"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsAfterRenaming_ByNewNameOnly)
    {
        LuaModule* luaModule = m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        LuaScript* script = m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding* cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        auto dataArray = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "dataarray");
        auto animNode = m_logicEngine.createAnimationNode({ { "channel", dataArray, dataArray } }, "animNode");
        auto timerNode = m_logicEngine.createTimerNode("timerNode");

        // Rename
        luaModule->setName("L");
        script->setName("S");
        nodeBinding->setName("NB");
        appearanceBinding->setName("AB");
        cameraBinding->setName("CB");
        dataArray->setName("DA");
        animNode->setName("AN");
        timerNode->setName("TN");

        // Can't find by old name
        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaModule>("luaModule"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaScript>("script"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesAppearanceBinding>("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesCameraBinding>("camerabinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<DataArray>("dataarray"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<AnimationNode>("animNode"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<TimerNode>("timerNode"));

        // Found by new name
        EXPECT_EQ(luaModule, m_logicEngine.findByName<LuaModule>("L"));
        EXPECT_EQ(script, m_logicEngine.findByName<LuaScript>("S"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findByName<RamsesNodeBinding>("NB"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findByName<RamsesAppearanceBinding>("AB"));
        EXPECT_EQ(cameraBinding, m_logicEngine.findByName<RamsesCameraBinding>("CB"));
        EXPECT_EQ(dataArray, m_logicEngine.findByName<DataArray>("DA"));
        EXPECT_EQ(animNode, m_logicEngine.findByName<AnimationNode>("AN"));
        EXPECT_EQ(timerNode, m_logicEngine.findByName<TimerNode>("TN"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyIfTypeMatches)
    {
        m_logicEngine.createLuaModule(m_moduleSourceCode, {}, "luaModule");
        m_logicEngine.createLuaScript(m_valid_empty_script, {}, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");
        const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "dataarray");
        m_logicEngine.createAnimationNode({ { "channel", dataArray, dataArray } }, "animNode");
        m_logicEngine.createTimerNode("timerNode");

        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaModule>("dataarray"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaScript>("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaScript>("camerabinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesAppearanceBinding>("animNode"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesCameraBinding>("script"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<DataArray>("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<AnimationNode>("dataarray"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<LuaModule>("timerNode"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyStringMatchesExactly)
    {
        m_logicEngine.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodebinding");

        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("Nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("node"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("binding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("Xnodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findByName<RamsesNodeBinding>("nodebindinY"));
    }
}

