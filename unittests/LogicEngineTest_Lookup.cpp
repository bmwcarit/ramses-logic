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
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding* cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");

        EXPECT_EQ(script, m_logicEngine.findScript("script"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findNodeBinding("nodebinding"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("appbinding"));
        EXPECT_EQ(cameraBinding, m_logicEngine.findCameraBinding("camerabinding"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_CutsNameAtNullTermination)
    {
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("appbinding\0withsurprise"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsAfterRenaming_ByNewNameOnly)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        RamsesCameraBinding* cameraBinding = m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");

        // Rename
        script->setName("S");
        nodeBinding->setName("NB");
        appearanceBinding->setName("AB");
        cameraBinding->setName("CB");

        // Can't find by old name
        EXPECT_EQ(nullptr, m_logicEngine.findScript("script"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findAppearanceBinding("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findCameraBinding("camerabinding"));

        // Found by new name
        EXPECT_EQ(script, m_logicEngine.findScript("S"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findNodeBinding("NB"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("AB"));
        EXPECT_EQ(cameraBinding, m_logicEngine.findCameraBinding("CB"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyIfTypeMatches)
    {
        m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        m_logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");
        m_logicEngine.createRamsesAppearanceBinding(*m_appearance, "appbinding");
        m_logicEngine.createRamsesCameraBinding(*m_camera, "camerabinding");

        EXPECT_EQ(nullptr, m_logicEngine.findScript("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findScript("camerabinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findAppearanceBinding("script"));
        EXPECT_EQ(nullptr, m_logicEngine.findCameraBinding("script"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyStringMatchesExactly)
    {
        m_logicEngine.createRamsesNodeBinding(*m_node, "nodebinding");

        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("Nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("node"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("binding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("Xnodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("nodebindinY"));
    }
}

