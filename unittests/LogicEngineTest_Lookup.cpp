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

    TEST_F(ALogicEngine_Lookup, ProvidesEmptyCollections_WhenNothingWasCreated)
    {
        Collection<LuaScript> scripts = m_logicEngine.scripts();
        EXPECT_EQ(scripts.begin(), scripts.end());
        EXPECT_EQ(scripts.cbegin(), scripts.cend());

        Collection<RamsesNodeBinding> nodes = m_logicEngine.ramsesNodeBindings();
        EXPECT_EQ(nodes.begin(), nodes.end());
        EXPECT_EQ(nodes.cbegin(), nodes.cend());

        Collection<RamsesAppearanceBinding> appearances = m_logicEngine.ramsesAppearanceBindings();
        EXPECT_EQ(appearances.begin(), appearances.end());
        EXPECT_EQ(appearances.cbegin(), appearances.cend());
    }

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyScriptCollection_WhenScriptsWereCreated)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        Collection<LuaScript> scripts = m_logicEngine.scripts();

        EXPECT_EQ(*scripts.begin(), script);
        EXPECT_EQ(*scripts.cbegin(), script);

        EXPECT_NE(scripts.begin(), scripts.end());
        EXPECT_NE(scripts.cbegin(), scripts.cend());
    }

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyNodeBindingsCollection_WhenNodeBindingsWereCreated)
    {
        RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding("nodebinding");
        Collection<RamsesNodeBinding> nodes = m_logicEngine.ramsesNodeBindings();

        EXPECT_EQ(*nodes.begin(), binding);
        EXPECT_EQ(*nodes.cbegin(), binding);

        EXPECT_NE(nodes.begin(), nodes.end());
        EXPECT_NE(nodes.cbegin(), nodes.cend());
    }

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyAppearanceBindingsCollection_WhenAppearanceBindingsWereCreated)
    {
        RamsesAppearanceBinding* binding = m_logicEngine.createRamsesAppearanceBinding("appbinding");
        Collection<RamsesAppearanceBinding> appearances = m_logicEngine.ramsesAppearanceBindings();

        EXPECT_EQ(*appearances.begin(), binding);
        EXPECT_EQ(*appearances.cbegin(), binding);

        EXPECT_NE(appearances.begin(), appearances.end());
        EXPECT_NE(appearances.cbegin(), appearances.cend());
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding("nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding("appbinding");

        EXPECT_EQ(script, m_logicEngine.findScript("script"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findNodeBinding("nodebinding"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("appbinding"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsByTheirName_CutsNameAtNullTermination)
    {
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding("appbinding");
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("appbinding\0withsurprise"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectsAfterRenaming_ByNewNameOnly)
    {
        LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        RamsesNodeBinding* nodeBinding = m_logicEngine.createRamsesNodeBinding("nodebinding");
        RamsesAppearanceBinding* appearanceBinding = m_logicEngine.createRamsesAppearanceBinding("appbinding");

        // Rename
        script->setName("S");
        nodeBinding->setName("NB");
        appearanceBinding->setName("AB");

        // Can't find by old name
        EXPECT_EQ(nullptr, m_logicEngine.findScript("script"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findAppearanceBinding("appbinding"));

        // Found by new name
        EXPECT_EQ(script, m_logicEngine.findScript("S"));
        EXPECT_EQ(nodeBinding, m_logicEngine.findNodeBinding("NB"));
        EXPECT_EQ(appearanceBinding, m_logicEngine.findAppearanceBinding("AB"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyIfTypeMatches)
    {
        m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, "script");
        m_logicEngine.createRamsesNodeBinding("nodebinding");
        m_logicEngine.createRamsesAppearanceBinding("appbinding");

        EXPECT_EQ(nullptr, m_logicEngine.findScript("nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("appbinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findAppearanceBinding("script"));
    }

    TEST_F(ALogicEngine_Lookup, FindsObjectByNameOnlyStringMatchesExactly)
    {
        m_logicEngine.createRamsesNodeBinding("nodebinding");

        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("Nodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("node"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("binding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("Xnodebinding"));
        EXPECT_EQ(nullptr, m_logicEngine.findNodeBinding("nodebindinY"));
    }
}

