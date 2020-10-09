//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicEngineTest_Base.h"

#include "ramses-logic/RamsesNodeBinding.h"
#include "internals/impl/LogicEngineImpl.h"

namespace rlogic
{
    class ALogicEngine_Lookup : public ALogicEngine
    {
    protected:
        LogicEngine m_logicEngine;

        std::vector<LuaScript*> createEmptyScripts(std::initializer_list<std::string> names)
        {
            std::vector<LuaScript*> scripts;
            for (const auto& name : names)
            {
                LuaScript* script = m_logicEngine.createLuaScriptFromSource(m_valid_empty_script, name);
                EXPECT_NE(nullptr, script);
                scripts.emplace_back(script);
            }
            return scripts;
        }

        std::vector<RamsesNodeBinding*> createEmptyNodeBindings(std::initializer_list<std::string> names)
        {
            std::vector<RamsesNodeBinding*> bindings;
            for (const auto& name : names)
            {
                RamsesNodeBinding* binding = m_logicEngine.createRamsesNodeBinding(name);
                EXPECT_NE(nullptr, binding);
                bindings.emplace_back(binding);
            }
            return bindings;
        }

        std::vector<RamsesAppearanceBinding*> createEmptyAppearanceBindings(std::initializer_list<std::string> names)
        {
            std::vector<RamsesAppearanceBinding*> bindings;
            for (const auto& name : names)
            {
                RamsesAppearanceBinding* binding = m_logicEngine.createRamsesAppearanceBinding(name);
                EXPECT_NE(nullptr, binding);
                bindings.emplace_back(binding);
            }
            return bindings;
        }
    };

    TEST_F(ALogicEngine_Lookup, ProvidesEmptyCollection_WhenNothingWasCreated)
    {
        Collection<LuaScript> scripts = m_logicEngine.scripts();

        // Begin == end because containers empty
        EXPECT_EQ(scripts.begin(), scripts.end());
        EXPECT_EQ(scripts.cbegin(), scripts.cend());

        Collection<RamsesNodeBinding> nodes = m_logicEngine.ramsesNodeBindings();
        EXPECT_EQ(nodes.begin(), nodes.end());
        EXPECT_EQ(nodes.cbegin(), nodes.cend());

        Collection<RamsesAppearanceBinding> appearances = m_logicEngine.ramsesAppearanceBindings();
        EXPECT_EQ(appearances.begin(), appearances.end());
        EXPECT_EQ(appearances.cbegin(), appearances.cend());
    }

    // TODO Violin see if googletest has issues with string_view in newer versions and remove std::string constructor in below tests

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyScriptCollection_WhenScriptsWereCreated)
    {
        createEmptyScripts({ "script1", "script2" });

        Collection<LuaScript> scripts = m_logicEngine.scripts();

        // Begin works (const and non-const versions)
        EXPECT_THAT(std::string(scripts.begin()->getName()), ::testing::HasSubstr("script"));
        EXPECT_THAT(std::string(scripts.cbegin()->getName()), ::testing::HasSubstr("script"));

        // Begin != end because container not empty
        EXPECT_NE(scripts.begin(), scripts.end());
        EXPECT_NE(scripts.cbegin(), scripts.cend());
    }

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyNodeBindingsCollection_WhenNodeBindingsWereCreated)
    {
        createEmptyNodeBindings({ "node1", "node2" });
        Collection<RamsesNodeBinding> nodes = m_logicEngine.ramsesNodeBindings();
        EXPECT_THAT(std::string(nodes.begin()->getName()), ::testing::HasSubstr("node"));
        EXPECT_THAT(std::string(nodes.cbegin()->getName()), ::testing::HasSubstr("node"));
        EXPECT_NE(nodes.begin(), nodes.end());
        EXPECT_NE(nodes.cbegin(), nodes.cend());
    }

    TEST_F(ALogicEngine_Lookup, ProvidesNonEmptyAppearanceBindingsCollection_WhenAppearanceBindingsWereCreated)
    {
        createEmptyAppearanceBindings({ "app1", "app2" });
        Collection<RamsesAppearanceBinding> appearances = m_logicEngine.ramsesAppearanceBindings();
        EXPECT_THAT(std::string(appearances.begin()->getName()), ::testing::HasSubstr("app"));
        EXPECT_THAT(std::string(appearances.cbegin()->getName()), ::testing::HasSubstr("app"));
        EXPECT_NE(appearances.begin(), appearances.end());
        EXPECT_NE(appearances.cbegin(), appearances.cend());
    }

    TEST_F(ALogicEngine_Lookup, CanDoRangeLoopsOnCollections)
    {
        const std::initializer_list<std::string> scriptNames =
        {
            "script0",
            "script1",
        };

        createEmptyScripts(scriptNames);

        Collection<LuaScript> scripts = m_logicEngine.scripts();
        Collection<LuaScript>& scriptsRef = scripts;
        const Collection<LuaScript>& scriptsConstRef = scriptsRef;
        Collection<LuaScript> scriptsCopy = scripts;

        auto nameIter = scriptNames.begin();
        for (auto script : scripts)
        {
            EXPECT_EQ(*nameIter, script->getName());
            nameIter++;
        }

        nameIter = scriptNames.begin();
        for (auto script : scriptsRef)
        {
            EXPECT_EQ(*nameIter, script->getName());
            nameIter++;
        }

        nameIter = scriptNames.begin();
        for (auto script : scriptsCopy)
        {
            EXPECT_EQ(*nameIter, script->getName());
            nameIter++;
        }

        nameIter = scriptNames.begin();
        auto constIterBegin = scriptsConstRef.cbegin();
        auto constIterEnd = scriptsConstRef.cend();
        for (; constIterBegin != constIterEnd; constIterBegin++)
        {
            const LuaScript* script = *constIterBegin;
            EXPECT_EQ(*nameIter, script->getName());
            nameIter++;
        }
    }

    TEST_F(ALogicEngine_Lookup, ProvidesCollectionsWhichWorkWithStdBeginAndStdEnd)
    {
        createEmptyScripts({ "script0", "script1" });

        Collection<LuaScript> scripts = m_logicEngine.scripts();

        EXPECT_EQ(std::begin(scripts), std::begin(scripts));
        EXPECT_EQ(std::end(scripts), std::end(scripts));
    }

    TEST_F(ALogicEngine_Lookup, CanExecuteStandardAlgorithmsOnCollections)
    {
        const auto allScripts = createEmptyScripts({ "script0", "script1" });

        Collection<LuaScript> scripts = m_logicEngine.scripts();
        Collection<LuaScript> scriptsCopy(scripts);
        const Collection<LuaScript>& scriptsConstRef = scripts;

        size_t i = 0;
        auto checkNames = [&i](const LuaScript* script)
        {
            ASSERT_EQ(std::string("script") + std::to_string(i++), script->getName());
        };
        std::for_each(scripts.begin(), scripts.end(), checkNames);

        i = 0;
        std::for_each(scripts.cbegin(), scripts.cend(), checkNames);

        i = 0;
        std::for_each(scriptsConstRef.cbegin(), scriptsConstRef.cend(), checkNames);

        EXPECT_EQ(allScripts[1], *std::find_if(scripts.begin(), scripts.end(), [](LuaScript* script) {return script->getName() == "script1"; }));
        EXPECT_EQ(allScripts[1], *std::find_if(scriptsCopy.begin(), scriptsCopy.end(), [](const LuaScript* script) {return script->getName() == "script1"; }));
        EXPECT_EQ(scripts.end(), std::find_if(scripts.begin(), scripts.end(), [](LuaScript* script) {return script->getName() == "script15"; }));
    }

    TEST_F(ALogicEngine_Lookup, CanIncrementIteratorsOfCollection)
    {
        const auto allScripts = createEmptyScripts({ "script0", "script1" });

        Collection<LuaScript> scripts = m_logicEngine.scripts();
        Collection<LuaScript> scriptsCopy(scripts);
        auto iter = scriptsCopy.begin();
        auto otherIter = iter++;
        EXPECT_NE(iter, otherIter);
        ++otherIter;
        EXPECT_EQ(otherIter, iter);
    }

    TEST_F(ALogicEngine_Lookup, CanCompareIteratorsOfCollection)
    {
        const auto allScripts = createEmptyScripts({"script0", "script1"});

        Collection<LuaScript> scripts = m_logicEngine.scripts();

        EXPECT_EQ(scripts.begin(), scripts.begin());
        EXPECT_EQ(scripts.end(), scripts.end());
        auto iter0_a = scripts.begin();
        auto iter0_b = scripts.begin();
        auto iter1 = ++scripts.begin();
        EXPECT_EQ(iter0_a, iter0_a);
        EXPECT_NE(iter0_a, iter1);
        EXPECT_NE(iter0_b, iter1);
        EXPECT_EQ(iter1, iter1);
    }

    TEST_F(ALogicEngine_Lookup, CanCompareDefaultInitializedIterators)
    {
        Iterator<LuaScript, std::vector<std::unique_ptr<LuaScript>>, false> defaultInitializedIterator1;
        Iterator<LuaScript, std::vector<std::unique_ptr<LuaScript>>, false> defaultInitializedIterator2;
        EXPECT_EQ(defaultInitializedIterator1, defaultInitializedIterator2);
        Iterator<LuaScript, std::vector<std::unique_ptr<LuaScript>>, true> defaultInitializedConstIterator1;
        Iterator<LuaScript, std::vector<std::unique_ptr<LuaScript>>, true> defaultInitializedConstIterator2;
        EXPECT_EQ(defaultInitializedConstIterator1, defaultInitializedConstIterator2);
    }

    TEST_F(ALogicEngine_Lookup, CollectionsReturnsSameIteratorsForConstAndExplicitConstIterators)
    {
        const auto allScripts = createEmptyScripts({"script0", "script1"});

        EXPECT_EQ(allScripts.begin(), allScripts.cbegin());
        EXPECT_EQ(allScripts.end(), allScripts.cend());
    }

    // TODO Violin/Tobias this doesn't work... See if we can find a way to implement it
    //TEST_F(ALogicEngine_Lookup, CanCompareConstAndMutableIteratorsInAnyDirection)
    //{
    //    const auto allScripts = createEmptyScripts({ "script0", "script1" });
    //    Collection<LuaScript> scripts = m_logicEngine.scripts();
    //
    //    EXPECT_EQ(scripts.cbegin(), scripts.begin());
    //    EXPECT_EQ(scripts.begin(), scripts.cbegin());
    //    EXPECT_EQ(scripts.cend(), scripts.end());
    //
    //    EXPECT_NE(scripts.cbegin(), scripts.end());
    //    EXPECT_NE(scripts.cend(), scripts.begin());
    //    EXPECT_NE(scripts.cend(), scripts.end());
    //}
}

