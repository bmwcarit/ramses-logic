//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "generated/LogicEngineGen.h"

#include "internals/ApiObjects.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"

#include "impl/LogicEngineImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "ramses-client-api/Appearance.h"
#include "RamsesTestUtils.h"
#include "LogTestUtils.h"
#include "SerializationTestUtils.h"
#include "RamsesObjectResolverMock.h"

namespace rlogic::internal
{
    class AnApiObjects : public ::testing::Test
    {
    protected:
        ErrorReporting  m_errorReporting;
        ApiObjects      m_apiObjects;
        flatbuffers::FlatBufferBuilder m_flatBufferBuilder;
        SerializationTestUtils m_testUtils{ m_flatBufferBuilder };
        ::testing::StrictMock<RamsesObjectResolverMock> m_resolverMock;

        RamsesTestSetup m_ramses;
        ramses::Scene* m_scene = { m_ramses.createScene() };
        ramses::Node* m_node = { m_scene->createNode() };
        ramses::PerspectiveCamera* m_camera = { m_scene->createPerspectiveCamera() };
        ramses::Appearance* m_appearance = { &RamsesTestSetup::CreateTrivialTestAppearance(*m_scene) };

        const std::string_view m_moduleSrc = R"(
            local mymath = {}
            return mymath
        )";

        const std::string_view m_valid_empty_script = R"(
            function interface()
            end
            function run()
            end
        )";

        LuaScript* createScript()
        {
            return createScript(m_apiObjects, m_valid_empty_script);
        }

        LuaScript* createScript(ApiObjects& apiObjects, std::string_view source)
        {
            auto script = apiObjects.createLuaScript(source, {}, "script", m_errorReporting);
            EXPECT_NE(nullptr, script);
            return script;
        }

        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
    };

    TEST_F(AnApiObjects, CreatesScriptFromValidLuaWithoutErrors)
    {
        const LuaScript* script = createScript();
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(script, m_apiObjects.getApiObject(script->m_impl));
        EXPECT_EQ(script, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(script, m_apiObjects.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, DestroysScriptWithoutErrors)
    {
        LuaScript* script = createScript();
        ASSERT_TRUE(m_apiObjects.destroy(*script, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.getOwnedObjects().empty());
        EXPECT_TRUE(m_apiObjects.getLogicObjects().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingScriptFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        LuaScript* script = createScript(otherInstance, m_valid_empty_script);
        EXPECT_EQ(script, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(script, otherInstance.getLogicObjects().back());
        ASSERT_FALSE(m_apiObjects.destroy(*script, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find script in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, script);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(script, otherInstance.getApiObject(script->m_impl));
        EXPECT_EQ(script, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(script, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, CreatesLuaModule)
    {
        auto module = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        EXPECT_NE(nullptr, module);

        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        ASSERT_EQ(1u, m_apiObjects.getLuaModules().size());
        EXPECT_EQ(1u, m_apiObjects.getLogicObjects().size());
        EXPECT_EQ(1u, m_apiObjects.getOwnedObjects().size());
        EXPECT_EQ(module, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(module, m_apiObjects.getLogicObjects().back());
        EXPECT_EQ(module, m_apiObjects.getLuaModules().front());
    }

    TEST_F(AnApiObjects, CreatesRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getApiObject(ramsesNodeBinding->m_impl));
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getOwnedObjects().empty());
        EXPECT_TRUE(m_apiObjects.getLogicObjects().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesNodeBinding* ramsesNodeBinding = otherInstance.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getLogicObjects().back());
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesNodeBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, ramsesNodeBinding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObject(ramsesNodeBinding->m_impl));
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, CreatesRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "CameraBinding");
        EXPECT_NE(nullptr, ramsesCameraBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getApiObject(ramsesCameraBinding->m_impl));
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_NE(nullptr, ramsesCameraBinding);
        m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getOwnedObjects().empty());
        EXPECT_TRUE(m_apiObjects.getLogicObjects().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesCameraBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesCameraBinding* ramsesCameraBinding = otherInstance.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_TRUE(ramsesCameraBinding);
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getLogicObjects().back());
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesCameraBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, ramsesCameraBinding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObject(ramsesCameraBinding->m_impl));
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, CreatesRamsesAppearanceBindingWithoutErrors)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        EXPECT_NE(nullptr, binding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(binding, m_apiObjects.getApiObject(binding->m_impl));
        EXPECT_EQ(binding, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(binding, m_apiObjects.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesAppearanceBindingWithoutErrors)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(m_apiObjects.destroy(*binding, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.getOwnedObjects().empty());
        EXPECT_TRUE(m_apiObjects.getLogicObjects().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesAppearanceBinding* binding = otherInstance.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        ASSERT_TRUE(binding);
        EXPECT_EQ(binding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(binding, otherInstance.getLogicObjects().back());
        ASSERT_FALSE(m_apiObjects.destroy(*binding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesAppearanceBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, binding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(binding, otherInstance.getApiObject(binding->m_impl));
        EXPECT_EQ(binding, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(binding, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, CreatesDataArray)
    {
        const std::vector<float> data{ 1.f, 2.f, 3.f };
        auto dataArray = m_apiObjects.createDataArray(data, "data");
        EXPECT_NE(nullptr, dataArray);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        ASSERT_EQ(1u, m_apiObjects.getDataArrays().size());
        EXPECT_EQ(dataArray, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(dataArray, m_apiObjects.getLogicObjects().back());
        EXPECT_EQ(EPropertyType::Float, m_apiObjects.getDataArrays().front()->getDataType());
        ASSERT_NE(nullptr, m_apiObjects.getDataArrays().front()->getData<float>());
        EXPECT_EQ(data, *m_apiObjects.getDataArrays().front()->getData<float>());
    }

    TEST_F(AnApiObjects, DestroysDataArray)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getDataArrays().empty());
        EXPECT_TRUE(m_apiObjects.getOwnedObjects().empty());
        EXPECT_TRUE(m_apiObjects.getLogicObjects().empty());
    }

    TEST_F(AnApiObjects, FailsToDestroysDataArrayIfUsedInAnimationNode)
    {
        auto dataArray1 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data1");
        auto dataArray2 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data2");
        auto dataArray3 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data3");
        auto dataArray4 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data4");

        auto animNode = m_apiObjects.createAnimationNode({
            AnimationChannel{ "channel1", dataArray1, dataArray2 },
            AnimationChannel{ "channel2", dataArray1, dataArray2, EInterpolationType::Cubic, dataArray3, dataArray4 } }, "animNode");

        EXPECT_FALSE(m_apiObjects.destroy(*dataArray1, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Failed to destroy data array 'data1', it is used in animation node 'animNode' channel 'channel1'");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray1);
        m_errorReporting.clear();

        EXPECT_FALSE(m_apiObjects.destroy(*dataArray2, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Failed to destroy data array 'data2', it is used in animation node 'animNode' channel 'channel1'");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray2);
        m_errorReporting.clear();

        EXPECT_FALSE(m_apiObjects.destroy(*dataArray3, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Failed to destroy data array 'data3', it is used in animation node 'animNode' channel 'channel2'");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray3);
        m_errorReporting.clear();

        EXPECT_FALSE(m_apiObjects.destroy(*dataArray4,  m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Failed to destroy data array 'data4', it is used in animation node 'animNode' channel 'channel2'");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray4);
        m_errorReporting.clear();

        // succeeds after destroying animation node
        EXPECT_TRUE(m_apiObjects.destroy(*animNode, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray1, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray2, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray3, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray4, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
    }

    TEST_F(AnApiObjects, FailsToDestroyDataArrayFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        auto dataArray = otherInstance.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        EXPECT_EQ(dataArray, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(dataArray, otherInstance.getLogicObjects().back());
        EXPECT_FALSE(m_apiObjects.destroy(*dataArray, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find data array in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray);

        // Did not affect existence in otherInstance!
        EXPECT_TRUE(m_apiObjects.getDataArrays().empty());
        EXPECT_EQ(dataArray, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(dataArray, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, CreatesAnimationNode)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        auto animNode = m_apiObjects.createAnimationNode({ AnimationChannel{ "channel", dataArray, dataArray, EInterpolationType::Linear } }, "animNode");
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(animNode, m_apiObjects.getOwnedObjects().back().get());
        EXPECT_EQ(animNode, m_apiObjects.getLogicObjects().back());
        EXPECT_EQ(2u, m_apiObjects.getLogicObjects().size());
        EXPECT_EQ(2u, m_apiObjects.getOwnedObjects().size());
        ASSERT_EQ(1u, m_apiObjects.getAnimationNodes().size());
        EXPECT_EQ(animNode, m_apiObjects.getAnimationNodes().front());
    }

    TEST_F(AnApiObjects, DestroysAnimationNode)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        auto animNode = m_apiObjects.createAnimationNode({ AnimationChannel{ "channel", dataArray, dataArray, EInterpolationType::Linear } }, "animNode");
        EXPECT_TRUE(m_apiObjects.destroy(*animNode, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getAnimationNodes().empty());
        // did not affect data array
        EXPECT_TRUE(!m_apiObjects.getDataArrays().empty());
        EXPECT_EQ(1u, m_apiObjects.getOwnedObjects().size());
        EXPECT_EQ(1u, m_apiObjects.getLogicObjects().size());
    }

    TEST_F(AnApiObjects, FailsToDestroyAnimationNodeFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        auto dataArray = otherInstance.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        auto animNode = otherInstance.createAnimationNode({ AnimationChannel{ "channel", dataArray, dataArray, EInterpolationType::Linear } }, "animNode");
        EXPECT_EQ(animNode, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(animNode, otherInstance.getLogicObjects().back());
        EXPECT_FALSE(m_apiObjects.destroy(*animNode, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find AnimationNode in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, animNode);

        // Did not affect existence in otherInstance!
        EXPECT_TRUE(m_apiObjects.getAnimationNodes().empty());
        EXPECT_EQ(animNode, otherInstance.getOwnedObjects().back().get());
        EXPECT_EQ(animNode, otherInstance.getLogicObjects().back());
    }

    TEST_F(AnApiObjects, ProvidesEmptyCollections_WhenNothingWasCreated)
    {
        ScriptsContainer&             scripts      = m_apiObjects.getScripts();
        NodeBindingsContainer&        nodes        = m_apiObjects.getNodeBindings();
        AppearanceBindingsContainer&  appearances  = m_apiObjects.getAppearanceBindings();
        CameraBindingsContainer&      cameras      = m_apiObjects.getCameraBindings();
        LogicObjectContainer&         logicObjects = m_apiObjects.getLogicObjects();
        const ObjectsOwningContainer& ownedObjects = m_apiObjects.getOwnedObjects();

        const ApiObjects& apiObjectsConst = m_apiObjects;
        const ScriptsContainer&             const_scripts         = apiObjectsConst.getScripts();
        const NodeBindingsContainer&        const_nodes           = apiObjectsConst.getNodeBindings();
        const AppearanceBindingsContainer&  const_appearances     = apiObjectsConst.getAppearanceBindings();
        const CameraBindingsContainer&      const_cameras         = apiObjectsConst.getCameraBindings();
        const LogicObjectContainer&         const_logicObjects    = apiObjectsConst.getLogicObjects();
        const ObjectsOwningContainer&       const_ownedObjects    = apiObjectsConst.getOwnedObjects();

        EXPECT_EQ(scripts.begin(), scripts.end());
        EXPECT_EQ(const_scripts.begin(), const_scripts.end());

        EXPECT_EQ(nodes.begin(), nodes.end());
        EXPECT_EQ(const_nodes.begin(), const_nodes.end());

        EXPECT_EQ(appearances.begin(), appearances.end());
        EXPECT_EQ(const_appearances.begin(), const_appearances.end());

        EXPECT_EQ(cameras.begin(), cameras.end());
        EXPECT_EQ(const_cameras.begin(), const_cameras.end());

        EXPECT_EQ(logicObjects.begin(), logicObjects.end());
        EXPECT_EQ(const_logicObjects.begin(), const_logicObjects.end());

        EXPECT_EQ(ownedObjects.begin(), ownedObjects.end());
        EXPECT_EQ(const_ownedObjects.begin(), const_ownedObjects.end());

        EXPECT_TRUE(m_apiObjects.getReverseImplMapping().empty());
        EXPECT_TRUE(apiObjectsConst.getReverseImplMapping().empty());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyScriptCollection_WhenScriptsWereCreated)
    {
        const LuaScript* script = createScript();
        ScriptsContainer& scripts = m_apiObjects.getScripts();

        EXPECT_EQ(*scripts.begin(), script);
        EXPECT_EQ(*scripts.cbegin(), script);

        EXPECT_NE(scripts.begin(), scripts.end());
        EXPECT_NE(scripts.cbegin(), scripts.cend());

        EXPECT_EQ(script, *scripts.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyNodeBindingsCollection_WhenNodeBindingsWereCreated)
    {
        RamsesNodeBinding* binding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        NodeBindingsContainer& nodes = m_apiObjects.getNodeBindings();

        EXPECT_EQ(*nodes.begin(), binding);
        EXPECT_EQ(*nodes.cbegin(), binding);

        EXPECT_NE(nodes.begin(), nodes.end());
        EXPECT_NE(nodes.cbegin(), nodes.cend());

        EXPECT_EQ(binding, *nodes.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyAppearanceBindingsCollection_WhenAppearanceBindingsWereCreated)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        AppearanceBindingsContainer& appearances = m_apiObjects.getAppearanceBindings();

        EXPECT_EQ(*appearances.begin(), binding);
        EXPECT_EQ(*appearances.cbegin(), binding);

        EXPECT_NE(appearances.begin(), appearances.end());
        EXPECT_NE(appearances.cbegin(), appearances.cend());

        EXPECT_EQ(binding, *appearances.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyCameraBindingsCollection_WhenCameraBindingsWereCreated)
    {
        RamsesCameraBinding* binding = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        CameraBindingsContainer& cameras = m_apiObjects.getCameraBindings();

        EXPECT_EQ(*cameras.begin(), binding);
        EXPECT_EQ(*cameras.cbegin(), binding);

        EXPECT_NE(cameras.begin(), cameras.end());
        EXPECT_NE(cameras.cbegin(), cameras.cend());

        EXPECT_EQ(binding, *cameras.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyOwningAndLogicObjectsCollection_WhenLogicObjectsWereCreated)
    {
        const LogicObjectContainer&   logicObjects = m_apiObjects.getLogicObjects();
        const ObjectsOwningContainer& ownedObjects = m_apiObjects.getOwnedObjects();

        const auto* luaModule         = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        const auto* luaScript         = createScript(m_apiObjects, m_valid_empty_script);
        const auto* nodeBinding       = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        const auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        const auto* cameraBinding     = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        const auto* dataArray         = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
        const auto* animationNode     = m_apiObjects.createAnimationNode({AnimationChannel{"channel", dataArray, dataArray, EInterpolationType::Linear}}, "animNode");

        std::vector<LogicObject*> ownedLogicObjectsRawPointers;
        std::transform(ownedObjects.cbegin(), ownedObjects.cend(), std::back_inserter(ownedLogicObjectsRawPointers), [&](std::unique_ptr<LogicObject>const& obj) { return obj.get(); });

        EXPECT_THAT(logicObjects, ::testing::ElementsAre(luaModule, luaScript, nodeBinding, appearanceBinding, cameraBinding, dataArray, animationNode));
        EXPECT_THAT(ownedLogicObjectsRawPointers, ::testing::ElementsAre(luaModule, luaScript, nodeBinding, appearanceBinding, cameraBinding, dataArray, animationNode));

        const ApiObjects& apiObjectsConst = m_apiObjects;
        EXPECT_EQ(logicObjects, apiObjectsConst.getLogicObjects());
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
        m_apiObjects.createRamsesNodeBinding(*scene1->createNode("node1"), ERotationType::Euler_XYZ, "binding1");
        RamsesNodeBinding* binding2 = m_apiObjects.createRamsesNodeBinding(*scene2->createNode("node2"), ERotationType::Euler_XYZ, "binding2");

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses node 'node2' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(binding2, m_errorReporting.getErrors()[0].object);
    }

    TEST_F(AnApiObjects_SceneMismatch, recognizesNodeBindingAndAppearanceBindingAreFromDifferentScenes)
    {
        m_apiObjects.createRamsesNodeBinding(*scene1->createNode("node"), ERotationType::Euler_XYZ, "node binding");
        RamsesAppearanceBinding* appBinding = m_apiObjects.createRamsesAppearanceBinding(RamsesTestSetup::CreateTrivialTestAppearance(*scene2), "app binding");

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses appearance 'test appearance' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(appBinding, m_errorReporting.getErrors()[0].object);
    }

    TEST_F(AnApiObjects_SceneMismatch, recognizesNodeBindingAndCameraBindingAreFromDifferentScenes)
    {
        m_apiObjects.createRamsesNodeBinding(*scene1->createNode("node"), ERotationType::Euler_XYZ, "node binding");
        RamsesCameraBinding* camBinding = m_apiObjects.createRamsesCameraBinding(*scene2->createPerspectiveCamera("test camera"), "cam binding");

        EXPECT_FALSE(m_apiObjects.checkBindingsReferToSameRamsesScene(m_errorReporting));
        EXPECT_EQ(1u, m_errorReporting.getErrors().size());
        EXPECT_EQ("Ramses camera 'test camera' is from scene with id:2 but other objects are from scene with id:1!", m_errorReporting.getErrors()[0].message);
        EXPECT_EQ(camBinding, m_errorReporting.getErrors()[0].object);
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
        auto script1 = createScript();
        auto script2 = createScript();

        ASSERT_TRUE(m_apiObjects.destroy(*script1, m_errorReporting));

        ASSERT_EQ(1u, m_apiObjects.getReverseImplMapping().size());
        EXPECT_EQ(script2, m_apiObjects.getApiObject(script2->m_impl));
    }

    TEST_F(AnApiObjects_ImplMapping, DestroyingBindingDoesNotAffectScript)
    {
        const LuaScript* script = createScript();
        RamsesNodeBinding* binding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");

        ASSERT_TRUE(m_apiObjects.destroy(*binding, m_errorReporting));

        ASSERT_EQ(1u, m_apiObjects.getReverseImplMapping().size());
        EXPECT_EQ(script, m_apiObjects.getApiObject(script->m_impl));
    }

    class AnApiObjects_Serialization : public AnApiObjects
    {
    };

    TEST_F(AnApiObjects_Serialization, AlwaysCreatesEmptyFlatbuffersContainers_WhenNoObjectsPresent)
    {
        // Create without API objects -> serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            ApiObjects::Serialize(toSerialize, builder);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        // Has all containers, size = 0 because no content
        ASSERT_NE(nullptr, serialized.luaScripts());
        ASSERT_EQ(0u, serialized.luaScripts()->size());

        ASSERT_NE(nullptr, serialized.nodeBindings());
        ASSERT_EQ(0u, serialized.nodeBindings()->size());

        ASSERT_NE(nullptr, serialized.appearanceBindings());
        ASSERT_EQ(0u, serialized.appearanceBindings()->size());

        ASSERT_NE(nullptr, serialized.cameraBindings());
        ASSERT_EQ(0u, serialized.cameraBindings()->size());

        ASSERT_NE(nullptr, serialized.links());
        ASSERT_EQ(0u, serialized.links()->size());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainer_ForScripts)
    {
        // Create test flatbuffer with only a script
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            createScript(toSerialize, m_valid_empty_script);
            ApiObjects::Serialize(toSerialize, builder);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        ASSERT_NE(nullptr, serialized.luaScripts());
        ASSERT_EQ(1u, serialized.luaScripts()->size());
        const rlogic_serialization::LuaScript& serializedScript = *serialized.luaScripts()->Get(0);
        EXPECT_EQ(std::string(m_valid_empty_script), serializedScript.luaSourceCode()->str());
        EXPECT_EQ("script", serializedScript.name()->str());

        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "test", m_errorReporting);
        EXPECT_TRUE(deserialized);
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainers_ForBindings)
    {
        // Create test flatbuffer with only a node binding
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "node");
            toSerialize.createRamsesAppearanceBinding(*m_appearance, "appearance");
            toSerialize.createRamsesCameraBinding(*m_camera, "camera");
            ApiObjects::Serialize(toSerialize, builder);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        ASSERT_NE(nullptr, serialized.nodeBindings());
        ASSERT_EQ(1u, serialized.nodeBindings()->size());
        const rlogic_serialization::RamsesNodeBinding& serializedNodeBinding = *serialized.nodeBindings()->Get(0);
        EXPECT_EQ("node", serializedNodeBinding.base()->name()->str());

        ASSERT_NE(nullptr, serialized.appearanceBindings());
        ASSERT_EQ(1u, serialized.appearanceBindings()->size());
        const rlogic_serialization::RamsesAppearanceBinding& serializedAppBinding = *serialized.appearanceBindings()->Get(0);
        EXPECT_EQ("appearance", serializedAppBinding.base()->name()->str());

        ASSERT_NE(nullptr, serialized.cameraBindings());
        ASSERT_EQ(1u, serialized.cameraBindings()->size());
        const rlogic_serialization::RamsesCameraBinding& serializedCameraBinding = *serialized.cameraBindings()->Get(0);
        EXPECT_EQ("camera", serializedCameraBinding.base()->name()->str());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainers_ForLinks)
    {
        // Create test flatbuffer with a link between script and binding
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;

            const std::string_view scriptWithOutput = R"(
                function interface()
                    OUT.nested = {
                        anUnusedValue = FLOAT,
                        rotation = VEC3F
                    }
                end
                function run()
                end
            )";

            const LuaScript* script = createScript(toSerialize, scriptWithOutput);
            RamsesNodeBinding* nodeBinding = toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script->getOutputs()->getChild("nested")->getChild("rotation")->m_impl,
                *nodeBinding->getInputs()->getChild("rotation")->m_impl,
                m_errorReporting));
            ApiObjects::Serialize(toSerialize, builder);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        // Asserts both script and binding objects existence
        ASSERT_EQ(1u, serialized.luaScripts()->size());
        ASSERT_EQ(1u, serialized.nodeBindings()->size());
        const rlogic_serialization::LuaScript& script = *serialized.luaScripts()->Get(0);
        const rlogic_serialization::RamsesNodeBinding& binding = *serialized.nodeBindings()->Get(0);

        ASSERT_NE(nullptr, serialized.links());
        ASSERT_EQ(1u, serialized.links()->size());
        const rlogic_serialization::Link& link = *serialized.links()->Get(0);

        EXPECT_EQ(script.rootOutput()->children()->Get(0)->children()->Get(1), link.sourceProperty());
        EXPECT_EQ(binding.base()->rootInput()->children()->Get(size_t(ENodePropertyStaticIndex::Rotation)), link.targetProperty());
    }

    TEST_F(AnApiObjects_Serialization, ReConstructsImplMappingsWhenCreatedFromDeserializedData)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            createScript(toSerialize, m_valid_empty_script);
            toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "node");
            toSerialize.createRamsesAppearanceBinding(*m_appearance, "appearance");
            toSerialize.createRamsesCameraBinding(*m_camera, "camera");

            ApiObjects::Serialize(toSerialize, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        EXPECT_CALL(m_resolverMock, findRamsesNodeInScene(::testing::Eq("node"), m_node->getSceneObjectId())).WillOnce(::testing::Return(m_node));
        EXPECT_CALL(m_resolverMock, findRamsesAppearanceInScene(::testing::Eq("appearance"), m_appearance->getSceneObjectId())).WillOnce(::testing::Return(m_appearance));
        EXPECT_CALL(m_resolverMock, findRamsesCameraInScene(::testing::Eq("camera"), m_camera->getSceneObjectId())).WillOnce(::testing::Return(m_camera));
        std::unique_ptr<ApiObjects> apiObjectsOptional = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        ASSERT_TRUE(apiObjectsOptional);

        ApiObjects& apiObjects = *apiObjectsOptional;

        ASSERT_EQ(4u, apiObjects.getReverseImplMapping().size());

        LuaScript* script = apiObjects.getScripts()[0];
        EXPECT_EQ(script, apiObjects.getApiObject(script->m_impl));
        EXPECT_EQ(script->getName(), "script");

        RamsesNodeBinding* nodeBinding = apiObjects.getNodeBindings()[0];
        EXPECT_EQ(nodeBinding, apiObjects.getApiObject(nodeBinding->m_impl));
        EXPECT_EQ(nodeBinding->getName(), "node");

        RamsesAppearanceBinding* appBinding = apiObjects.getAppearanceBindings()[0];
        EXPECT_EQ(appBinding, apiObjects.getApiObject(appBinding->m_impl));
        EXPECT_EQ(appBinding->getName(), "appearance");

        RamsesCameraBinding* camBinding = apiObjects.getCameraBindings()[0];
        EXPECT_EQ(camBinding, apiObjects.getApiObject(camBinding->m_impl));
        EXPECT_EQ(camBinding->getName(), "camera");
    }

    TEST_F(AnApiObjects_Serialization, ReConstructsLinksWhenCreatedFromDeserializedData)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;

            const std::string_view scriptForLinks = R"(
                function interface()
                    IN.integer = INT
                    OUT.nested = {
                        unused = FLOAT,
                        integer = INT
                    }
                end
                function run()
                end
            )";

            auto script1 = createScript(toSerialize, scriptForLinks);
            auto script2 = createScript(toSerialize, scriptForLinks);
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl,
                *script2->getInputs()->getChild("integer")->m_impl,
                m_errorReporting));

            ApiObjects::Serialize(toSerialize, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        std::unique_ptr<ApiObjects> apiObjectsOptional = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        ASSERT_TRUE(apiObjectsOptional);

        ApiObjects& apiObjects = *apiObjectsOptional;

        LuaScript* script1 = apiObjects.getScripts()[0];
        ASSERT_TRUE(script1);

        LuaScript* script2 = apiObjects.getScripts()[1];
        ASSERT_TRUE(script2);

        const PropertyImpl* linkedOutput = apiObjects.getLogicNodeDependencies().getLinkedOutput(*script2->getInputs()->getChild("integer")->m_impl);
        EXPECT_EQ(script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl.get(), linkedOutput);

        // Test some more internal data (because of the fragile state of link's deserialization). Consider removing if we refactor the code
        const LinksMap& linkMap = apiObjects.getLogicNodeDependencies().getLinks();
        ASSERT_EQ(1u, linkMap.size());
        EXPECT_EQ(linkMap.begin()->second, script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl.get());
        EXPECT_EQ(linkMap.begin()->first, script2->getInputs()->getChild("integer")->m_impl.get());
    }

    // TODO VersionBreak enable test with next breaking change
    //TEST_F(AnApiObjects_Serialization, ErrorWhenLuaModulesContainerMissing)
    //{
    //    {
    //        auto apiObjects = rlogic_serialization::CreateApiObjects(
    //            m_flatBufferBuilder,
    //            0, // no modules container
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
    //            m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
    //        );
    //        m_flatBufferBuilder.Finish(apiObjects);
    //    }
    //
    //    const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
    //    std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);
    //
    //    EXPECT_FALSE(deserialized);
    //    ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
    //    EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing Lua scripts and/or modules container!");
    //}

    TEST_F(AnApiObjects_Serialization, ErrorWhenScriptsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                0, // no scripts container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing Lua scripts container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenNodeBindingsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                0, // no node bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing node bindings container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenAppearanceBindingsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                0, // no appearance bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing appearance bindings container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenCameraBindingsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                0, // no camera bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing camera bindings container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenLinksContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                0 // no links container
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing links container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenDataArrayContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                0, // no data array container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing data arrays container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenAnimationNodeContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                0, // no animation nodes container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing animation nodes container!");
    }

    TEST_F(AnApiObjects_Serialization, ReportsErrorWhenScriptCouldNotBeDeserialized)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{m_testUtils.serializeTestScript(true)}), // script has errors
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaScript from serialized data: missing name!");
    }

    TEST_F(AnApiObjects_Serialization, FillsLogicObjectAndOwnedContainerOnDeserialization)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            toSerialize.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
            createScript(toSerialize, m_valid_empty_script);
            toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "node");
            toSerialize.createRamsesAppearanceBinding(*m_appearance, "appearance");
            toSerialize.createRamsesCameraBinding(*m_camera, "camera");
            auto dataArray = toSerialize.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
            toSerialize.createAnimationNode({AnimationChannel{"channel", dataArray, dataArray, EInterpolationType::Linear}}, "animNode");

            ApiObjects::Serialize(toSerialize, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        EXPECT_CALL(m_resolverMock, findRamsesNodeInScene(::testing::Eq("node"), m_node->getSceneObjectId())).WillOnce(::testing::Return(m_node));
        EXPECT_CALL(m_resolverMock, findRamsesAppearanceInScene(::testing::Eq("appearance"), m_appearance->getSceneObjectId())).WillOnce(::testing::Return(m_appearance));
        EXPECT_CALL(m_resolverMock, findRamsesCameraInScene(::testing::Eq("camera"), m_camera->getSceneObjectId())).WillOnce(::testing::Return(m_camera));
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        ASSERT_TRUE(deserialized);

        ApiObjects& apiObjects = *deserialized;

        const LogicObjectContainer& logicObjects = apiObjects.getLogicObjects();
        const ObjectsOwningContainer& ownedObjects = apiObjects.getOwnedObjects();
        ASSERT_EQ(7u, logicObjects.size());
        ASSERT_EQ(7u, ownedObjects.size());

        const std::vector<LogicObject*> expected{
            apiObjects.getLuaModules()[0],
            apiObjects.getScripts()[0],
            apiObjects.getNodeBindings()[0],
            apiObjects.getAppearanceBindings()[0],
            apiObjects.getCameraBindings()[0],
            apiObjects.getDataArrays()[0],
            apiObjects.getAnimationNodes()[0]
        };

        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(logicObjects[i], expected[i]);
            EXPECT_EQ(ownedObjects[i].get(), expected[i]);
        }
    }
}
