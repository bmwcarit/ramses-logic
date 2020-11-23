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
}

