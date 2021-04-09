//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "internals/ApiObjects.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"

#include "impl/LuaScriptImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "RamsesTestUtils.h"

namespace rlogic::internal
{
    class AnApiObjects : public ::testing::Test
    {
    protected:
        SolState        m_state;
        ErrorReporting  m_errorReporting;
        ApiObjects      m_apiObjects;

        const std::string_view m_valid_empty_script = R"(
            function interface()
            end
            function run()
            end
        )";

    };

    TEST_F(AnApiObjects, ProducesErrorWhenCreatingEmptyScript)
    {
        auto script = m_apiObjects.createLuaScript(m_state, "", "", "", m_errorReporting);
        ASSERT_EQ(nullptr, script);
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());

        EXPECT_TRUE(m_apiObjects.getReverseImplMapping().empty());
    }

    TEST_F(AnApiObjects, CreatesScriptFromValidLuaWithoutErrors)
    {
        LuaScript* script = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        ASSERT_TRUE(nullptr != script);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(script, m_apiObjects.getApiObject(script->m_impl));
    }

    TEST_F(AnApiObjects, DestroysScriptWithoutErrors)
    {
        LuaScript* script = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        ASSERT_TRUE(script);
        ASSERT_TRUE(m_apiObjects.destroy(*script, m_errorReporting));
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingScriptFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        LuaScript* script = otherInstance.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        ASSERT_TRUE(script);
        ASSERT_FALSE(m_apiObjects.destroy(*script, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find script in logic engine!");

        // Did not affect existence in otherInstance!
        EXPECT_EQ(script, otherInstance.getApiObject(script->m_impl));
    }

    TEST_F(AnApiObjects, CreatesRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding("NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getApiObject(ramsesNodeBinding->m_impl));
    }

    TEST_F(AnApiObjects, DestroysRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding("NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesNodeBinding* ramsesNodeBinding = otherInstance.createRamsesNodeBinding("NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesNodeBinding in logic engine!");

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObject(ramsesNodeBinding->m_impl));
    }

    TEST_F(AnApiObjects, CreatesRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding("CameraBinding");
        EXPECT_NE(nullptr, ramsesCameraBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getApiObject(ramsesCameraBinding->m_impl));
    }

    TEST_F(AnApiObjects, DestroysRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding("CameraBinding");
        ASSERT_NE(nullptr, ramsesCameraBinding);
        m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesCameraBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesCameraBinding* ramsesCameraBinding = otherInstance.createRamsesCameraBinding("CameraBinding");
        ASSERT_TRUE(ramsesCameraBinding);
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesCameraBinding in logic engine!");

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObject(ramsesCameraBinding->m_impl));
    }

    TEST_F(AnApiObjects, DestroysRamsesAppearanceBindingWithoutErrors)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(m_apiObjects.destroy(*binding, m_errorReporting));
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesAppearanceBinding* binding = otherInstance.createRamsesAppearanceBinding("AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_FALSE(m_apiObjects.destroy(*binding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesAppearanceBinding in logic engine!");

        // Did not affect existence in otherInstance!
        EXPECT_EQ(binding, otherInstance.getApiObject(binding->m_impl));
    }

    TEST_F(AnApiObjects, CanBeMovedWithoutChangingObjectAddresses)
    {
        LuaScript* script = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding("NodeBinding");
        RamsesAppearanceBinding* appBinding = m_apiObjects.createRamsesAppearanceBinding("AppearanceBinding");
        RamsesCameraBinding* camBinding = m_apiObjects.createRamsesCameraBinding("CameraBinding");

        ApiObjects movedObjects(std::move(m_apiObjects));
        EXPECT_EQ(script, movedObjects.getScripts()[0].get());
        EXPECT_EQ(ramsesNodeBinding, movedObjects.getNodeBindings()[0].get());
        EXPECT_EQ(appBinding, movedObjects.getAppearanceBindings()[0].get());
        EXPECT_EQ(camBinding, movedObjects.getCameraBindings()[0].get());

        EXPECT_EQ(script, movedObjects.getApiObject(script->m_impl));
        EXPECT_EQ(ramsesNodeBinding, movedObjects.getApiObject(ramsesNodeBinding->m_impl));
        EXPECT_EQ(appBinding, movedObjects.getApiObject(appBinding->m_impl));
        EXPECT_EQ(camBinding, movedObjects.getApiObject(camBinding->m_impl));
    }

    TEST_F(AnApiObjects, ProvidesEmptyCollections_WhenNothingWasCreated)
    {
        ScriptsContainer& scripts = m_apiObjects.getScripts();
        EXPECT_EQ(scripts.begin(), scripts.end());
        EXPECT_EQ(scripts.cbegin(), scripts.cend());

        NodeBindingsContainer& nodes = m_apiObjects.getNodeBindings();
        EXPECT_EQ(nodes.begin(), nodes.end());
        EXPECT_EQ(nodes.cbegin(), nodes.cend());

        AppearanceBindingsContainer& appearances = m_apiObjects.getAppearanceBindings();
        EXPECT_EQ(appearances.begin(), appearances.end());
        EXPECT_EQ(appearances.cbegin(), appearances.cend());

        CameraBindingsContainer& cameras = m_apiObjects.getCameraBindings();
        EXPECT_EQ(cameras.begin(), cameras.end());
        EXPECT_EQ(cameras.cbegin(), cameras.cend());

        EXPECT_TRUE(m_apiObjects.getReverseImplMapping().empty());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyScriptCollection_WhenScriptsWereCreated)
    {
        LuaScript* script = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        ScriptsContainer& scripts = m_apiObjects.getScripts();

        EXPECT_EQ(scripts.begin()->get(), script);
        EXPECT_EQ(scripts.cbegin()->get(), script);

        EXPECT_NE(scripts.begin(), scripts.end());
        EXPECT_NE(scripts.cbegin(), scripts.cend());

        EXPECT_EQ(script, scripts.begin()->get());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyNodeBindingsCollection_WhenNodeBindingsWereCreated)
    {
        RamsesNodeBinding* binding = m_apiObjects.createRamsesNodeBinding("");
        NodeBindingsContainer& nodes = m_apiObjects.getNodeBindings();

        EXPECT_EQ(nodes.begin()->get(), binding);
        EXPECT_EQ(nodes.cbegin()->get(), binding);

        EXPECT_NE(nodes.begin(), nodes.end());
        EXPECT_NE(nodes.cbegin(), nodes.cend());

        EXPECT_EQ(binding, nodes.begin()->get());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyAppearanceBindingsCollection_WhenAppearanceBindingsWereCreated)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding("");
        AppearanceBindingsContainer& appearances = m_apiObjects.getAppearanceBindings();

        EXPECT_EQ(appearances.begin()->get(), binding);
        EXPECT_EQ(appearances.cbegin()->get(), binding);

        EXPECT_NE(appearances.begin(), appearances.end());
        EXPECT_NE(appearances.cbegin(), appearances.cend());

        EXPECT_EQ(binding, appearances.begin()->get());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyCameraBindingsCollection_WhenCameraBindingsWereCreated)
    {
        RamsesCameraBinding* binding = m_apiObjects.createRamsesCameraBinding("");
        CameraBindingsContainer& cameras = m_apiObjects.getCameraBindings();

        EXPECT_EQ(cameras.begin()->get(), binding);
        EXPECT_EQ(cameras.cbegin()->get(), binding);

        EXPECT_NE(cameras.begin(), cameras.end());
        EXPECT_NE(cameras.cbegin(), cameras.cend());

        EXPECT_EQ(binding, cameras.begin()->get());
    }

    class AnApiObjects_SceneMismatch : public AnApiObjects
    {
    protected:
        RamsesTestSetup m_testSetup;
        ramses::Scene* scene1 {m_testSetup.createScene(ramses::sceneId_t(1))};
        ramses::Scene* scene2 {m_testSetup.createScene(ramses::sceneId_t(2))};
    };

    TEST_F(AnApiObjects_SceneMismatch, recognizesNodeBindingsCarryNodesFromDifferentScenes)
    {
        RamsesNodeBinding* binding1 = m_apiObjects.createRamsesNodeBinding("binding1");
        RamsesNodeBinding* binding2 = m_apiObjects.createRamsesNodeBinding("binding2");

        binding1->setRamsesNode(scene1->createNode("node1"));
        binding2->setRamsesNode(scene2->createNode("node2"));

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(binding2, m_errorReporting.getErrors()[0].node);
    }

    TEST_F(AnApiObjects_SceneMismatch, recognizesNodeBindingAndAppearanceBindingAreFromDifferentScenes)
    {
        RamsesNodeBinding* nodeBinding = m_apiObjects.createRamsesNodeBinding("node binding");
        RamsesAppearanceBinding* appBinding = m_apiObjects.createRamsesAppearanceBinding("app binding");

        nodeBinding->setRamsesNode(scene1->createNode("node"));
        appBinding->setRamsesAppearance(&RamsesTestSetup::CreateTrivialTestAppearance(*scene2));

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses appearance 'test appearance' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(appBinding, m_errorReporting.getErrors()[0].node);
    }

    TEST_F(AnApiObjects_SceneMismatch, recognizesNodeBindingAndCameraBindingAreFromDifferentScenes)
    {
        RamsesNodeBinding* nodeBinding = m_apiObjects.createRamsesNodeBinding("node binding");
        RamsesCameraBinding* camBinding = m_apiObjects.createRamsesCameraBinding("cam binding");

        nodeBinding->setRamsesNode(scene1->createNode("node"));
        camBinding->setRamsesCamera(scene2->createPerspectiveCamera("test camera"));

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses camera 'test camera' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(camBinding, m_errorReporting.getErrors()[0].node);
    }

    class AnApiObjects_ImplMapping : public AnApiObjects
    {
    };

    TEST_F(AnApiObjects_ImplMapping, EmptyWhenCreated)
    {
        EXPECT_TRUE(m_apiObjects.getReverseImplMapping().empty());
    }

    TEST_F(AnApiObjects_ImplMapping, DestroyingScriptDoesNotAffectOtherScript)
    {
        LuaScript* script1 = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        LuaScript* script2 = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);

        ASSERT_TRUE(m_apiObjects.destroy(*script1, m_errorReporting));

        ASSERT_EQ(1u, m_apiObjects.getReverseImplMapping().size());
        EXPECT_EQ(script2, m_apiObjects.getApiObject(script2->m_impl));
    }

    TEST_F(AnApiObjects_ImplMapping, DestroyingBindingDoesNotAffectScript)
    {
        LuaScript* script = m_apiObjects.createLuaScript(m_state, m_valid_empty_script, "", "", m_errorReporting);
        RamsesNodeBinding* binding = m_apiObjects.createRamsesNodeBinding("");

        ASSERT_TRUE(m_apiObjects.destroy(*binding, m_errorReporting));

        ASSERT_EQ(1u, m_apiObjects.getReverseImplMapping().size());
        EXPECT_EQ(script, m_apiObjects.getApiObject(script->m_impl));
    }

    TEST_F(AnApiObjects_ImplMapping, ConstructsMappingWhenCreatedFromDeserializedData)
    {
        SolState solState;

        ScriptsContainer scripts;
        NodeBindingsContainer ramsesNodeBindings;
        AppearanceBindingsContainer ramsesAppearanceBindings;
        CameraBindingsContainer ramsesCameraBindings;

        scripts.emplace_back(std::make_unique<LuaScript>(LuaScriptImpl::Create(solState, m_valid_empty_script, "script", "", m_errorReporting)));
        ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Create("node")));
        ramsesAppearanceBindings.emplace_back(std::make_unique<RamsesAppearanceBinding>(RamsesAppearanceBindingImpl::Create("appearance")));
        ramsesCameraBindings.emplace_back(std::make_unique<RamsesCameraBinding>(RamsesCameraBindingImpl::Create("camera")));

        LogicNodeDependencies logicNodeDependencies;
        logicNodeDependencies.addNode(scripts.back()->m_impl);
        logicNodeDependencies.addNode(ramsesNodeBindings.back()->m_impl);
        logicNodeDependencies.addNode(ramsesAppearanceBindings.back()->m_impl);
        logicNodeDependencies.addNode(ramsesCameraBindings.back()->m_impl);

        ApiObjects apiObjects(std::move(scripts), std::move(ramsesNodeBindings), std::move(ramsesAppearanceBindings), std::move(ramsesCameraBindings), std::move(logicNodeDependencies));

        ASSERT_EQ(4u, apiObjects.getReverseImplMapping().size());

        LuaScript* script = apiObjects.getScripts()[0].get();
        EXPECT_EQ(script, apiObjects.getApiObject(script->m_impl));

        RamsesNodeBinding* nodeBinding = apiObjects.getNodeBindings()[0].get();
        EXPECT_EQ(nodeBinding, apiObjects.getApiObject(nodeBinding->m_impl));

        RamsesAppearanceBinding* appBinding = apiObjects.getAppearanceBindings()[0].get();
        EXPECT_EQ(appBinding, apiObjects.getApiObject(appBinding->m_impl));

        RamsesCameraBinding* camBinding = apiObjects.getCameraBindings()[0].get();
        EXPECT_EQ(camBinding, apiObjects.getApiObject(camBinding->m_impl));
    }
}
