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
#include "impl/LuaModuleImpl.h"
#include "impl/LuaScriptImpl.h"
#include "impl/LuaInterfaceImpl.h"
#include "impl/RamsesNodeBindingImpl.h"
#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/RamsesCameraBindingImpl.h"
#include "impl/DataArrayImpl.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/AnimationNodeConfig.h"
#include "ramses-logic/TimerNode.h"
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
            function interface(IN,OUT)
            end
            function run(IN,OUT)
            end
        )";

        const std::string_view m_valid_empty_interface = R"(
            function interface(IN,OUT)
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

        LuaInterface* createInterface()
        {
            return createInterface(m_apiObjects);
        }

        LuaInterface* createInterface(ApiObjects& apiObjects)
        {
            auto intf = apiObjects.createLuaInterface(m_valid_empty_interface, "intf", m_errorReporting);
            EXPECT_NE(nullptr, intf);
            return intf;
        }

        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
    };

    TEST_F(AnApiObjects, CreatesScriptFromValidLuaWithoutErrors)
    {
        const LuaScript* script = createScript();
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(script, m_apiObjects.getApiObject(script->m_impl));
        EXPECT_EQ(script, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(script, m_apiObjects.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, DestroysScriptWithoutErrors)
    {
        LuaScript* script = createScript();
        ASSERT_TRUE(m_apiObjects.destroy(*script, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingScriptFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        LuaScript* script = createScript(otherInstance, m_valid_empty_script);
        EXPECT_EQ(script, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(script, otherInstance.getApiObjectContainer<LogicObject>().back());
        ASSERT_FALSE(m_apiObjects.destroy(*script, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find script in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, script);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(script, otherInstance.getApiObject(script->m_impl));
        EXPECT_EQ(script, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(script, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesInterfaceFromValidLuaWithoutErrors)
    {
        const LuaInterface* intf = createInterface();
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(intf, m_apiObjects.getApiObject(intf->m_impl));
        EXPECT_EQ(intf, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(intf, m_apiObjects.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, DestroysInterfaceWithoutErrors)
    {
        LuaInterface* intf = createInterface();
        ASSERT_TRUE(m_apiObjects.destroy(*intf, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingInterfaceFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        LuaInterface* intf = createInterface(otherInstance);
        EXPECT_EQ(intf, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(intf, otherInstance.getApiObjectContainer<LogicObject>().back());
        ASSERT_FALSE(m_apiObjects.destroy(*intf, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find interface in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, intf);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(intf, otherInstance.getApiObject(intf->m_impl));
        EXPECT_EQ(intf, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(intf, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesLuaModule)
    {
        auto module = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        EXPECT_NE(nullptr, module);

        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        ASSERT_EQ(1u, m_apiObjects.getApiObjectContainer<LuaModule>().size());
        EXPECT_EQ(1u, m_apiObjects.getApiObjectContainer<LogicObject>().size());
        EXPECT_EQ(1u, m_apiObjects.getApiObjectOwningContainer().size());
        EXPECT_EQ(module, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(module, m_apiObjects.getApiObjectContainer<LogicObject>().back());
        EXPECT_EQ(module, m_apiObjects.getApiObjectContainer<LuaModule>().front());
    }

    TEST_F(AnApiObjects, CreatesRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        EXPECT_NE(nullptr, ramsesNodeBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getApiObject(ramsesNodeBinding->m_impl));
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesNodeBinding, m_apiObjects.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesNodeBindingWithoutErrors)
    {
        RamsesNodeBinding* ramsesNodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        ASSERT_NE(nullptr, ramsesNodeBinding);
        m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesNodeBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesNodeBinding* ramsesNodeBinding = otherInstance.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "NodeBinding");
        ASSERT_TRUE(ramsesNodeBinding);
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObjectContainer<LogicObject>().back());
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesNodeBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesNodeBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, ramsesNodeBinding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObject(ramsesNodeBinding->m_impl));
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesNodeBinding, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "CameraBinding");
        EXPECT_NE(nullptr, ramsesCameraBinding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getApiObject(ramsesCameraBinding->m_impl));
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesCameraBinding, m_apiObjects.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesCameraBindingWithoutErrors)
    {
        RamsesCameraBinding* ramsesCameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_NE(nullptr, ramsesCameraBinding);
        m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesCameraBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesCameraBinding* ramsesCameraBinding = otherInstance.createRamsesCameraBinding(*m_camera, "CameraBinding");
        ASSERT_TRUE(ramsesCameraBinding);
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObjectContainer<LogicObject>().back());
        ASSERT_FALSE(m_apiObjects.destroy(*ramsesCameraBinding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesCameraBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, ramsesCameraBinding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObject(ramsesCameraBinding->m_impl));
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(ramsesCameraBinding, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesRamsesAppearanceBindingWithoutErrors)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        EXPECT_NE(nullptr, binding);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(binding, m_apiObjects.getApiObject(binding->m_impl));
        EXPECT_EQ(binding, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(binding, m_apiObjects.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, DestroysRamsesAppearanceBindingWithoutErrors)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        ASSERT_TRUE(binding);
        ASSERT_TRUE(m_apiObjects.destroy(*binding, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, ProducesErrorsWhenDestroyingRamsesAppearanceBindingFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        RamsesAppearanceBinding* binding = otherInstance.createRamsesAppearanceBinding(*m_appearance, "AppearanceBinding");
        ASSERT_TRUE(binding);
        EXPECT_EQ(binding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(binding, otherInstance.getApiObjectContainer<LogicObject>().back());
        ASSERT_FALSE(m_apiObjects.destroy(*binding, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find RamsesAppearanceBinding in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, binding);

        // Did not affect existence in otherInstance!
        EXPECT_EQ(binding, otherInstance.getApiObject(binding->m_impl));
        EXPECT_EQ(binding, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(binding, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesDataArray)
    {
        const std::vector<float> data{ 1.f, 2.f, 3.f };
        auto dataArray = m_apiObjects.createDataArray(data, "data");
        EXPECT_NE(nullptr, dataArray);
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        ASSERT_EQ(1u, m_apiObjects.getApiObjectContainer<DataArray>().size());
        EXPECT_EQ(dataArray, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(dataArray, m_apiObjects.getApiObjectContainer<LogicObject>().back());
        EXPECT_EQ(EPropertyType::Float, m_apiObjects.getApiObjectContainer<DataArray>().front()->getDataType());
        ASSERT_NE(nullptr, m_apiObjects.getApiObjectContainer<DataArray>().front()->getData<float>());
        EXPECT_EQ(data, *m_apiObjects.getApiObjectContainer<DataArray>().front()->getData<float>());
    }

    TEST_F(AnApiObjects, DestroysDataArray)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        EXPECT_TRUE(m_apiObjects.destroy(*dataArray, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<DataArray>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, FailsToDestroysDataArrayIfUsedInAnimationNode)
    {
        auto dataArray1 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data1");
        auto dataArray2 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data2");
        auto dataArray3 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data3");
        auto dataArray4 = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data4");

        AnimationNodeConfig config;
        EXPECT_TRUE(config.addChannel({ "channel1", dataArray1, dataArray2 }));
        EXPECT_TRUE(config.addChannel({ "channel2", dataArray1, dataArray2, EInterpolationType::Cubic, dataArray3, dataArray4 }));
        auto animNode = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");

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
        EXPECT_EQ(dataArray, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(dataArray, otherInstance.getApiObjectContainer<LogicObject>().back());
        EXPECT_FALSE(m_apiObjects.destroy(*dataArray, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find data array in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, dataArray);

        // Did not affect existence in otherInstance!
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<DataArray>().empty());
        EXPECT_EQ(dataArray, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(dataArray, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesAnimationNode)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        AnimationNodeConfig config;
        EXPECT_TRUE(config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear }));
        auto animNode = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_EQ(animNode, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(animNode, m_apiObjects.getApiObjectContainer<LogicObject>().back());
        EXPECT_EQ(2u, m_apiObjects.getApiObjectContainer<LogicObject>().size());
        EXPECT_EQ(2u, m_apiObjects.getApiObjectOwningContainer().size());
        ASSERT_EQ(1u, m_apiObjects.getApiObjectContainer<AnimationNode>().size());
        EXPECT_EQ(animNode, m_apiObjects.getApiObjectContainer<AnimationNode>().front());
    }

    TEST_F(AnApiObjects, DestroysAnimationNode)
    {
        auto dataArray = m_apiObjects.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        AnimationNodeConfig config;
        EXPECT_TRUE(config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear }));
        auto animNode = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        EXPECT_TRUE(m_apiObjects.destroy(*animNode, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<AnimationNode>().empty());
        // did not affect data array
        EXPECT_TRUE(!m_apiObjects.getApiObjectContainer<DataArray>().empty());
        EXPECT_EQ(1u, m_apiObjects.getApiObjectOwningContainer().size());
        EXPECT_EQ(1u, m_apiObjects.getApiObjectContainer<LogicObject>().size());
    }

    TEST_F(AnApiObjects, FailsToDestroyAnimationNodeFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        auto dataArray = otherInstance.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f }, "data");
        ASSERT_NE(nullptr, dataArray);
        AnimationNodeConfig config;
        EXPECT_TRUE(config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear }));
        auto animNode = otherInstance.createAnimationNode(*config.m_impl, "animNode");
        EXPECT_EQ(animNode, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(animNode, otherInstance.getApiObjectContainer<LogicObject>().back());
        EXPECT_FALSE(m_apiObjects.destroy(*animNode, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find AnimationNode in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, animNode);

        // Did not affect existence in otherInstance!
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<AnimationNode>().empty());
        EXPECT_EQ(animNode, otherInstance.getApiObjectOwningContainer().back().get());
        EXPECT_EQ(animNode, otherInstance.getApiObjectContainer<LogicObject>().back());
    }

    TEST_F(AnApiObjects, CreatesTimerNode)
    {
        auto timerNode = m_apiObjects.createTimerNode("timerNode");
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        ASSERT_EQ(1u, m_apiObjects.getApiObjectOwningContainer().size());
        EXPECT_EQ(timerNode, m_apiObjects.getApiObjectOwningContainer().back().get());
        EXPECT_THAT(m_apiObjects.getApiObjectContainer<LogicObject>(), ::testing::ElementsAre(timerNode));
        EXPECT_THAT(m_apiObjects.getApiObjectContainer<TimerNode>(), ::testing::ElementsAre(timerNode));
    }

    TEST_F(AnApiObjects, DestroysTimerNode)
    {
        auto timerNode = m_apiObjects.createTimerNode("timerNode");
        EXPECT_TRUE(m_apiObjects.destroy(*timerNode, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<TimerNode>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
    }

    TEST_F(AnApiObjects, FailsToDestroyTimerNodeFromAnotherClassInstance)
    {
        ApiObjects otherInstance;
        auto timerNode = otherInstance.createTimerNode("timerNode");
        EXPECT_FALSE(m_apiObjects.destroy(*timerNode, m_errorReporting));
        EXPECT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Can't find TimerNode in logic engine!");
        EXPECT_EQ(m_errorReporting.getErrors()[0].object, timerNode);

        // Did not affect existence in otherInstance!
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<TimerNode>().empty());
        EXPECT_EQ(timerNode, otherInstance.getApiObjectContainer<TimerNode>().front());
        EXPECT_EQ(timerNode, otherInstance.getApiObjectOwningContainer().front().get());
        EXPECT_EQ(timerNode, otherInstance.getApiObjectContainer<LogicObject>().front());
    }

    TEST_F(AnApiObjects, ProvidesEmptyCollections_WhenNothingWasCreated)
    {
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LuaScript>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<RamsesNodeBinding>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<RamsesAppearanceBinding>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<RamsesCameraBinding>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<DataArray>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<AnimationNode>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<TimerNode>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectContainer<LogicObject>().empty());
        EXPECT_TRUE(m_apiObjects.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(m_apiObjects.getReverseImplMapping().empty());

        const ApiObjects& apiObjectsConst = m_apiObjects;
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<LuaScript>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<RamsesNodeBinding>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<RamsesAppearanceBinding>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<RamsesCameraBinding>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<DataArray>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<AnimationNode>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<TimerNode>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectContainer<LogicObject>().empty());
        EXPECT_TRUE(apiObjectsConst.getApiObjectOwningContainer().empty());
        EXPECT_TRUE(apiObjectsConst.getReverseImplMapping().empty());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyScriptCollection_WhenScriptsWereCreated)
    {
        const LuaScript* script = createScript();
        ApiObjectContainer<LuaScript>& scripts = m_apiObjects.getApiObjectContainer<LuaScript>();

        EXPECT_EQ(*scripts.begin(), script);
        EXPECT_EQ(*scripts.cbegin(), script);

        EXPECT_NE(scripts.begin(), scripts.end());
        EXPECT_NE(scripts.cbegin(), scripts.cend());

        EXPECT_EQ(script, *scripts.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyInterfaceCollection_WhenInterfacesWereCreated)
    {
        const LuaInterface* intf = createInterface();
        ApiObjectContainer<LuaInterface>& interfaces = m_apiObjects.getApiObjectContainer<LuaInterface>();

        EXPECT_EQ(*interfaces.begin(), intf);
        EXPECT_EQ(*interfaces.cbegin(), intf);

        EXPECT_NE(interfaces.begin(), interfaces.end());
        EXPECT_NE(interfaces.cbegin(), interfaces.cend());

        EXPECT_EQ(intf, *interfaces.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyNodeBindingsCollection_WhenNodeBindingsWereCreated)
    {
        RamsesNodeBinding* binding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        ApiObjectContainer<RamsesNodeBinding>& nodes = m_apiObjects.getApiObjectContainer<RamsesNodeBinding>();

        EXPECT_EQ(*nodes.begin(), binding);
        EXPECT_EQ(*nodes.cbegin(), binding);

        EXPECT_NE(nodes.begin(), nodes.end());
        EXPECT_NE(nodes.cbegin(), nodes.cend());

        EXPECT_EQ(binding, *nodes.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyAppearanceBindingsCollection_WhenAppearanceBindingsWereCreated)
    {
        RamsesAppearanceBinding* binding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        ApiObjectContainer<RamsesAppearanceBinding>& appearances = m_apiObjects.getApiObjectContainer<RamsesAppearanceBinding>();

        EXPECT_EQ(*appearances.begin(), binding);
        EXPECT_EQ(*appearances.cbegin(), binding);

        EXPECT_NE(appearances.begin(), appearances.end());
        EXPECT_NE(appearances.cbegin(), appearances.cend());

        EXPECT_EQ(binding, *appearances.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyCameraBindingsCollection_WhenCameraBindingsWereCreated)
    {
        RamsesCameraBinding* binding = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        ApiObjectContainer<RamsesCameraBinding>& cameras = m_apiObjects.getApiObjectContainer<RamsesCameraBinding>();

        EXPECT_EQ(*cameras.begin(), binding);
        EXPECT_EQ(*cameras.cbegin(), binding);

        EXPECT_NE(cameras.begin(), cameras.end());
        EXPECT_NE(cameras.cbegin(), cameras.cend());

        EXPECT_EQ(binding, *cameras.begin());
    }

    TEST_F(AnApiObjects, ProvidesNonEmptyOwningAndLogicObjectsCollection_WhenLogicObjectsWereCreated)
    {
        const ApiObjectContainer<LogicObject>&   logicObjects = m_apiObjects.getApiObjectContainer<LogicObject>();
        const ApiObjectOwningContainer& ownedObjects = m_apiObjects.getApiObjectOwningContainer();

        const auto* luaModule         = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        const auto* luaScript         = createScript(m_apiObjects, m_valid_empty_script);
        const auto* luaInterface      = createInterface();
        const auto* nodeBinding       = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        const auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        const auto* cameraBinding     = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        const auto* dataArray         = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        const auto* animationNode     = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        const auto* timerNode         = m_apiObjects.createTimerNode("timerNode");

        std::vector<LogicObject*> ownedLogicObjectsRawPointers;
        std::transform(ownedObjects.cbegin(), ownedObjects.cend(), std::back_inserter(ownedLogicObjectsRawPointers), [&](std::unique_ptr<LogicObject>const& obj) { return obj.get(); });

        EXPECT_THAT(logicObjects, ::testing::ElementsAre(luaModule, luaScript, luaInterface, nodeBinding, appearanceBinding, cameraBinding, dataArray, animationNode, timerNode));
        EXPECT_THAT(ownedLogicObjectsRawPointers, ::testing::ElementsAre(luaModule, luaScript, luaInterface, nodeBinding, appearanceBinding, cameraBinding, dataArray, animationNode, timerNode));

        const ApiObjects& apiObjectsConst = m_apiObjects;
        EXPECT_EQ(logicObjects, apiObjectsConst.getApiObjectContainer<LogicObject>());
    }

    TEST_F(AnApiObjects, logicObjectsGetUniqueIds)
    {
        const auto* luaModule         = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        const auto* luaScript         = createScript(m_apiObjects, m_valid_empty_script);
        const auto* luaInterface      = createInterface();
        const auto* nodeBinding       = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        const auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        const auto* cameraBinding     = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        const auto* dataArray         = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        const auto* animationNode     = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        const auto* timerNode         = m_apiObjects.createTimerNode("timerNode");

        std::unordered_set<uint64_t> logicObjectIds =
        {
            luaModule->getId(),
            luaScript->getId(),
            luaInterface->getId(),
            nodeBinding->getId(),
            appearanceBinding->getId(),
            cameraBinding->getId(),
            dataArray->getId(),
            animationNode->getId(),
            timerNode->getId()
        };

        EXPECT_EQ(9u, logicObjectIds.size());
    }

    TEST_F(AnApiObjects, canGetLogicObjectById)
    {
        const auto* luaModule         = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        const auto* luaScript         = createScript(m_apiObjects, m_valid_empty_script);
        const auto* nodeBinding       = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        const auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        const auto* cameraBinding     = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        const auto* dataArray         = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        const auto* animationNode     = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        const auto* timerNode         = m_apiObjects.createTimerNode("timerNode");
        const auto* luaInterface      = createInterface();

        EXPECT_EQ(luaModule->getId(), 1u);
        EXPECT_EQ(luaScript->getId(), 2u);
        EXPECT_EQ(nodeBinding->getId(), 3u);
        EXPECT_EQ(appearanceBinding->getId(), 4u);
        EXPECT_EQ(cameraBinding->getId(), 5u);
        EXPECT_EQ(dataArray->getId(), 6u);
        EXPECT_EQ(animationNode->getId(), 7u);
        EXPECT_EQ(timerNode->getId(), 8u);
        EXPECT_EQ(luaInterface->getId(), 9u);

        EXPECT_EQ(m_apiObjects.getApiObjectById(1u), luaModule);
        EXPECT_EQ(m_apiObjects.getApiObjectById(2u), luaScript);
        EXPECT_EQ(m_apiObjects.getApiObjectById(3u), nodeBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(4u), appearanceBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(5u), cameraBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(6u), dataArray);
        EXPECT_EQ(m_apiObjects.getApiObjectById(7u), animationNode);
        EXPECT_EQ(m_apiObjects.getApiObjectById(8u), timerNode);
        EXPECT_EQ(m_apiObjects.getApiObjectById(9u), luaInterface);
    }

    TEST_F(AnApiObjects, logicObjectIdsAreRemovedFromIdMappingWhenObjectIsDestroyed)
    {
        const auto* luaModule         = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        auto* luaScript               = createScript(m_apiObjects, m_valid_empty_script);
        const auto* nodeBinding       = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
        auto* appearanceBinding       = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "");
        const auto* cameraBinding     = m_apiObjects.createRamsesCameraBinding(*m_camera, "");
        const auto* dataArray         = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        auto* animationNode           = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        const auto* timerNode         = m_apiObjects.createTimerNode("timerNode");
        const auto* luaInterface      = createInterface();

        EXPECT_EQ(m_apiObjects.getApiObjectById(1u), luaModule);
        EXPECT_EQ(m_apiObjects.getApiObjectById(2u), luaScript);
        EXPECT_EQ(m_apiObjects.getApiObjectById(3u), nodeBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(4u), appearanceBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(5u), cameraBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(6u), dataArray);
        EXPECT_EQ(m_apiObjects.getApiObjectById(7u), animationNode);
        EXPECT_EQ(m_apiObjects.getApiObjectById(8u), timerNode);
        EXPECT_EQ(m_apiObjects.getApiObjectById(9u), luaInterface);

        EXPECT_TRUE(m_apiObjects.destroy(*luaScript, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.destroy(*appearanceBinding, m_errorReporting));
        EXPECT_TRUE(m_apiObjects.destroy(*animationNode, m_errorReporting));

        EXPECT_EQ(m_apiObjects.getApiObjectById(1u), luaModule);
        EXPECT_EQ(m_apiObjects.getApiObjectById(2u), nullptr);
        EXPECT_EQ(m_apiObjects.getApiObjectById(3u), nodeBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(4u), nullptr);
        EXPECT_EQ(m_apiObjects.getApiObjectById(5u), cameraBinding);
        EXPECT_EQ(m_apiObjects.getApiObjectById(6u), dataArray);
        EXPECT_EQ(m_apiObjects.getApiObjectById(7u), nullptr);
        EXPECT_EQ(m_apiObjects.getApiObjectById(8u), timerNode);
        EXPECT_EQ(m_apiObjects.getApiObjectById(9u), luaInterface);
    }

    TEST_F(AnApiObjects, logicObjectsGenerateIdentificationString)
    {
        const auto* luaModule = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        auto* luaScript = createScript(m_apiObjects, m_valid_empty_script);
        const auto* nodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodeBinding");
        auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "appearanceBinding");
        const auto* cameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "cameraBinding");
        const auto* dataArray = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataArray");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        auto* animationNode = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        const auto* timerNode = m_apiObjects.createTimerNode("timerNode");
        const auto* luaInterface = createInterface();

        EXPECT_EQ(luaModule->m_impl.getIdentificationString(), "module [Id=1]");
        EXPECT_EQ(luaScript->m_impl.getIdentificationString(), "script [Id=2]");
        EXPECT_EQ(nodeBinding->m_impl.getIdentificationString(), "nodeBinding [Id=3]");
        EXPECT_EQ(appearanceBinding->m_impl.getIdentificationString(), "appearanceBinding [Id=4]");
        EXPECT_EQ(cameraBinding->m_impl.getIdentificationString(), "cameraBinding [Id=5]");
        EXPECT_EQ(dataArray->m_impl.getIdentificationString(), "dataArray [Id=6]");
        EXPECT_EQ(animationNode->m_impl.getIdentificationString(), "animNode [Id=7]");
        EXPECT_EQ(timerNode->m_impl.getIdentificationString(), "timerNode [Id=8]");
        EXPECT_EQ(luaInterface->m_impl.getIdentificationString(), "intf [Id=9]");
    }

    TEST_F(AnApiObjects, logicObjectsGenerateIdentificationStringWithUserId)
    {
        auto* luaModule = m_apiObjects.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
        auto* luaScript = createScript(m_apiObjects, m_valid_empty_script);
        auto* nodeBinding = m_apiObjects.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "nodeBinding");
        auto* appearanceBinding = m_apiObjects.createRamsesAppearanceBinding(*m_appearance, "appearanceBinding");
        auto* cameraBinding = m_apiObjects.createRamsesCameraBinding(*m_camera, "cameraBinding");
        auto* dataArray = m_apiObjects.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "dataArray");
        AnimationNodeConfig config;
        config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
        auto* animationNode = m_apiObjects.createAnimationNode(*config.m_impl, "animNode");
        auto* timerNode = m_apiObjects.createTimerNode("timerNode");
        auto* luaInterface = createInterface();

        EXPECT_TRUE(luaModule->setUserId(1u, 2u));
        EXPECT_TRUE(luaScript->setUserId(3u, 4u));
        EXPECT_TRUE(nodeBinding->setUserId(5u, 6u));
        EXPECT_TRUE(appearanceBinding->setUserId(7u, 8u));
        EXPECT_TRUE(cameraBinding->setUserId(9u, 10u));
        EXPECT_TRUE(dataArray->setUserId(11u, 12u));
        EXPECT_TRUE(animationNode->setUserId(13u, 14u));
        EXPECT_TRUE(timerNode->setUserId(15u, 16u));
        EXPECT_TRUE(luaInterface->setUserId(17u, 18u));

        EXPECT_EQ(luaModule->m_impl.getIdentificationString(), "module [Id=1 UserId=00000000000000010000000000000002]");
        EXPECT_EQ(luaScript->m_impl.getIdentificationString(), "script [Id=2 UserId=00000000000000030000000000000004]");
        EXPECT_EQ(nodeBinding->m_impl.getIdentificationString(), "nodeBinding [Id=3 UserId=00000000000000050000000000000006]");
        EXPECT_EQ(appearanceBinding->m_impl.getIdentificationString(), "appearanceBinding [Id=4 UserId=00000000000000070000000000000008]");
        EXPECT_EQ(cameraBinding->m_impl.getIdentificationString(), "cameraBinding [Id=5 UserId=0000000000000009000000000000000A]");
        EXPECT_EQ(dataArray->m_impl.getIdentificationString(), "dataArray [Id=6 UserId=000000000000000B000000000000000C]");
        EXPECT_EQ(animationNode->m_impl.getIdentificationString(), "animNode [Id=7 UserId=000000000000000D000000000000000E]");
        EXPECT_EQ(timerNode->m_impl.getIdentificationString(), "timerNode [Id=8 UserId=000000000000000F0000000000000010]");
        EXPECT_EQ(luaInterface->m_impl.getIdentificationString(), "intf [Id=9 UserId=00000000000000110000000000000012]");
    }

    TEST_F(AnApiObjects, CanCheckIfAllLuaInterfaceOutputsAreLinked_GeneratesWarningsIfOutputsNotLinked)
    {
        LuaInterface* intf = m_apiObjects.createLuaInterface(R"(
            function interface(IN,OUT)

                IN.param1 = Type:Int32()
                IN.param2 = {a=Type:Float(), b=Type:Int32()}

            end
        )", "intf name", m_errorReporting);
        ASSERT_NE(nullptr, intf);

        ValidationResults validationResults;
        m_apiObjects.checkAllInterfaceOutputsLinked(validationResults);
        EXPECT_EQ(3u, validationResults.getWarnings().size());
        EXPECT_THAT(validationResults.getWarnings(),
            ::testing::Each(::testing::Field(&WarningData::message, ::testing::HasSubstr("Interface [intf name] has unlinked output"))));
        EXPECT_THAT(validationResults.getWarnings(), ::testing::Each(::testing::Field(&WarningData::type, ::testing::Eq(EWarningType::UnusedContent))));
    }

    TEST_F(AnApiObjects, CanCheckIfAllLuaInterfaceOutputsAreLinked_DoesNotGenerateWarningsIfAllOutputsLinked)
    {
        LuaInterface* intf = m_apiObjects.createLuaInterface(R"(
            function interface(IN,OUT)

                IN.param1 = Type:Int32()
                IN.param2 = {a=Type:Float(), b=Type:Int32()}

            end
        )", "intf name", m_errorReporting);

        LuaScript* inputsScript = m_apiObjects.createLuaScript(R"LUA_SCRIPT(
        function interface(IN,OUT)

            IN.param1 = Type:Int32()
            IN.param21 = Type:Float()
            IN.param22 = Type:Int32()

        end

        function run(IN,OUT)
        end
        )LUA_SCRIPT", {}, "inputs script", m_errorReporting);

        const auto* output1 = intf->getOutputs()->getChild(0);
        const auto* output21 = intf->getOutputs()->getChild(1)->getChild(0);
        const auto* output22 = intf->getOutputs()->getChild(1)->getChild(1);

        m_apiObjects.getLogicNodeDependencies().link(*output1->m_impl, *inputsScript->getInputs()->getChild(0)->m_impl, false, m_errorReporting);
        m_apiObjects.getLogicNodeDependencies().link(*output21->m_impl, *inputsScript->getInputs()->getChild(1)->m_impl, false, m_errorReporting);
        m_apiObjects.getLogicNodeDependencies().link(*output22->m_impl, *inputsScript->getInputs()->getChild(2)->m_impl, false, m_errorReporting);

        ValidationResults validationResults;
        m_apiObjects.checkAllInterfaceOutputsLinked(validationResults);
        EXPECT_TRUE(validationResults.getWarnings().empty());
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

        ASSERT_NE(nullptr, serialized.luaInterfaces());
        ASSERT_EQ(0u, serialized.luaInterfaces()->size());

        ASSERT_NE(nullptr, serialized.nodeBindings());
        ASSERT_EQ(0u, serialized.nodeBindings()->size());

        ASSERT_NE(nullptr, serialized.appearanceBindings());
        ASSERT_EQ(0u, serialized.appearanceBindings()->size());

        ASSERT_NE(nullptr, serialized.cameraBindings());
        ASSERT_EQ(0u, serialized.cameraBindings()->size());

        ASSERT_NE(nullptr, serialized.links());
        ASSERT_EQ(0u, serialized.links()->size());

        EXPECT_EQ(0u, serialized.lastObjectId());
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
        EXPECT_EQ("script", serializedScript.base()->name()->str());
        EXPECT_EQ(1u, serializedScript.base()->id());

        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "test", m_errorReporting);
        EXPECT_TRUE(deserialized);
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainer_ForInterfaces)
    {
        // Create test flatbuffer with only a script
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            createInterface(toSerialize);
            ApiObjects::Serialize(toSerialize, builder);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        ASSERT_NE(nullptr, serialized.luaInterfaces());
        ASSERT_EQ(1u, serialized.luaInterfaces()->size());
        const rlogic_serialization::LuaInterface& serializedInterface = *serialized.luaInterfaces()->Get(0);
        EXPECT_EQ("intf", serializedInterface.base()->name()->str());
        EXPECT_EQ(1u, serializedInterface.base()->id());

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
        EXPECT_EQ("node", serializedNodeBinding.base()->base()->name()->str());
        EXPECT_EQ(1u, serializedNodeBinding.base()->base()->id());

        ASSERT_NE(nullptr, serialized.appearanceBindings());
        ASSERT_EQ(1u, serialized.appearanceBindings()->size());
        const rlogic_serialization::RamsesAppearanceBinding& serializedAppBinding = *serialized.appearanceBindings()->Get(0);
        EXPECT_EQ("appearance", serializedAppBinding.base()->base()->name()->str());
        EXPECT_EQ(2u, serializedAppBinding.base()->base()->id());

        ASSERT_NE(nullptr, serialized.cameraBindings());
        ASSERT_EQ(1u, serialized.cameraBindings()->size());
        const rlogic_serialization::RamsesCameraBinding& serializedCameraBinding = *serialized.cameraBindings()->Get(0);
        EXPECT_EQ("camera", serializedCameraBinding.base()->base()->name()->str());
        EXPECT_EQ(3u, serializedCameraBinding.base()->base()->id());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainers_ForLinks)
    {
        // Create test flatbuffer with a link between script and binding
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;

            const std::string_view scriptWithOutput = R"(
                function interface(IN,OUT)
                    OUT.nested = {
                        anUnusedValue = Type:Float(),
                        rotation = Type:Vec3f()
                    }
                end
                function run(IN,OUT)
                end
            )";

            const LuaScript* script = createScript(toSerialize, scriptWithOutput);
            RamsesNodeBinding* nodeBinding = toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "");
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script->getOutputs()->getChild("nested")->getChild("rotation")->m_impl,
                *nodeBinding->getInputs()->getChild("rotation")->m_impl,
                false,
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
            createInterface(toSerialize);
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

        ASSERT_EQ(5u, apiObjects.getReverseImplMapping().size());

        LuaScript* script = apiObjects.getApiObjectContainer<LuaScript>()[0];
        EXPECT_EQ(script, apiObjects.getApiObject(script->m_impl));
        EXPECT_EQ(script->getName(), "script");

        LuaInterface* intf = apiObjects.getApiObjectContainer<LuaInterface>()[0];
        EXPECT_EQ(intf, apiObjects.getApiObject(intf->m_impl));
        EXPECT_EQ(intf->getName(), "intf");

        RamsesNodeBinding* nodeBinding = apiObjects.getApiObjectContainer<RamsesNodeBinding>()[0];
        EXPECT_EQ(nodeBinding, apiObjects.getApiObject(nodeBinding->m_impl));
        EXPECT_EQ(nodeBinding->getName(), "node");

        RamsesAppearanceBinding* appBinding = apiObjects.getApiObjectContainer<RamsesAppearanceBinding>()[0];
        EXPECT_EQ(appBinding, apiObjects.getApiObject(appBinding->m_impl));
        EXPECT_EQ(appBinding->getName(), "appearance");

        RamsesCameraBinding* camBinding = apiObjects.getApiObjectContainer<RamsesCameraBinding>()[0];
        EXPECT_EQ(camBinding, apiObjects.getApiObject(camBinding->m_impl));
        EXPECT_EQ(camBinding->getName(), "camera");
    }

    TEST_F(AnApiObjects_Serialization, ObjectsCreatedAfterLoadingReceiveUniqueId)
    {
        ApiObjects beforeSaving;

        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            createScript(beforeSaving, m_valid_empty_script);
            createScript(beforeSaving, m_valid_empty_script);
            createScript(beforeSaving, m_valid_empty_script);

            ApiObjects::Serialize(beforeSaving, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        EXPECT_EQ(3u, serialized.lastObjectId());

        std::unique_ptr<ApiObjects> afterLoadingObjects = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        auto* newScript = createScript(*afterLoadingObjects, m_valid_empty_script);
        // new script's ID does not overlap with one of the IDs of the objects before saving
        EXPECT_EQ(nullptr, beforeSaving.getApiObjectById(newScript->getId()));
    }

    TEST_F(AnApiObjects_Serialization, ReConstructsLinksWhenCreatedFromDeserializedData)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;

            const std::string_view scriptForLinks = R"(
                function interface(IN,OUT)
                    IN.integer = Type:Int32()
                    OUT.nested = {
                        unused = Type:Float(),
                        integer = Type:Int32()
                    }
                end
                function run(IN,OUT)
                end
            )";

            auto script1 = createScript(toSerialize, scriptForLinks);
            auto script2 = createScript(toSerialize, scriptForLinks);
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl,
                *script2->getInputs()->getChild("integer")->m_impl,
                false,
                m_errorReporting));

            ApiObjects::Serialize(toSerialize, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        std::unique_ptr<ApiObjects> apiObjectsOptional = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        ASSERT_TRUE(apiObjectsOptional);

        ApiObjects& apiObjects = *apiObjectsOptional;

        LuaScript* script1 = apiObjects.getApiObjectContainer<LuaScript>()[0];
        ASSERT_TRUE(script1);

        LuaScript* script2 = apiObjects.getApiObjectContainer<LuaScript>()[1];
        ASSERT_TRUE(script2);

        EXPECT_TRUE(apiObjects.getLogicNodeDependencies().isLinked(script1->m_impl));
        EXPECT_TRUE(apiObjects.getLogicNodeDependencies().isLinked(script2->m_impl));

        const PropertyImpl* script1Output = script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl.get();
        const PropertyImpl* script2Input = script2->getInputs()->getChild("integer")->m_impl.get();
        EXPECT_EQ(script1Output, script2Input->getIncomingLink().property);
        EXPECT_FALSE(script2Input->getIncomingLink().isWeakLink);

        ASSERT_EQ(1u, script1Output->getOutgoingLinks().size());
        EXPECT_EQ(script1Output->getOutgoingLinks()[0].property, script2Input);
        EXPECT_FALSE(script1Output->getOutgoingLinks()[0].isWeakLink);
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenLuaModulesContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                0, // no modules container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing Lua modules container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenScriptsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                0, // no scripts container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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

    TEST_F(AnApiObjects_Serialization, ErrorWhenInterfacesContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                0, // no interfaces container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing Lua interfaces container!");
    }

    TEST_F(AnApiObjects_Serialization, ErrorWhenNodeBindingsContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                0, // no node bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                0, // no appearance bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                0, // no camera bindings container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                0, // no data array container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                0, // no animation nodes container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
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

    TEST_F(AnApiObjects_Serialization, ErrorWhenTimerNodeContainerMissing)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                0, // no timer nodes container
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading from serialized data: missing timer nodes container!");
    }

    TEST_F(AnApiObjects_Serialization, ReportsErrorWhenScriptCouldNotBeDeserialized)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{ m_testUtils.serializeTestScriptWithError() }),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 2u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LogicObject base from serialized data: missing base table!");
        EXPECT_EQ(m_errorReporting.getErrors()[1].message, "Fatal error during loading of LuaScript from serialized data: missing name and/or ID!");
    }

    TEST_F(AnApiObjects_Serialization, ReportsErrorWhenInterfaceCouldNotBeDeserialized)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{m_testUtils.serializeTestInterfaceWithError()}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 1u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LuaInterface from serialized data: empty name!");
    }

    TEST_F(AnApiObjects_Serialization, ReportsErrorWhenModuleCouldNotBeDeserialized)
    {
        {
            auto apiObjects = rlogic_serialization::CreateApiObjects(
                m_flatBufferBuilder,
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaModule>>{ m_testUtils.serializeTestModule(true) }), // module has errors
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaScript>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::LuaInterface>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::DataArray>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::AnimationNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::TimerNode>>{}),
                m_flatBufferBuilder.CreateVector(std::vector<flatbuffers::Offset<rlogic_serialization::Link>>{})
            );
            m_flatBufferBuilder.Finish(apiObjects);
        }

        const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(m_flatBufferBuilder.GetBufferPointer());
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "unit test", m_errorReporting);

        EXPECT_FALSE(deserialized);
        ASSERT_EQ(m_errorReporting.getErrors().size(), 2u);
        EXPECT_EQ(m_errorReporting.getErrors()[0].message, "Fatal error during loading of LogicObject base from serialized data: missing name!");
        EXPECT_EQ(m_errorReporting.getErrors()[1].message, "Fatal error during loading of LuaModule from serialized data: missing name and/or ID!");
    }

    TEST_F(AnApiObjects_Serialization, FillsLogicObjectAndOwnedContainerOnDeserialization)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            ApiObjects toSerialize;
            toSerialize.createLuaModule(m_moduleSrc, {}, "module", m_errorReporting);
            createScript(toSerialize, m_valid_empty_script);
            createInterface(toSerialize);
            toSerialize.createRamsesNodeBinding(*m_node, ERotationType::Euler_XYZ, "node");
            toSerialize.createRamsesAppearanceBinding(*m_appearance, "appearance");
            toSerialize.createRamsesCameraBinding(*m_camera, "camera");
            auto dataArray = toSerialize.createDataArray(std::vector<float>{1.f, 2.f, 3.f}, "data");
            AnimationNodeConfig config;
            config.addChannel({ "channel", dataArray, dataArray, EInterpolationType::Linear });
            toSerialize.createAnimationNode(*config.m_impl, "animNode");
            toSerialize.createTimerNode("timerNode");

            ApiObjects::Serialize(toSerialize, builder);
        }

        auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::ApiObjects>(builder.GetBufferPointer());

        EXPECT_CALL(m_resolverMock, findRamsesNodeInScene(::testing::Eq("node"), m_node->getSceneObjectId())).WillOnce(::testing::Return(m_node));
        EXPECT_CALL(m_resolverMock, findRamsesAppearanceInScene(::testing::Eq("appearance"), m_appearance->getSceneObjectId())).WillOnce(::testing::Return(m_appearance));
        EXPECT_CALL(m_resolverMock, findRamsesCameraInScene(::testing::Eq("camera"), m_camera->getSceneObjectId())).WillOnce(::testing::Return(m_camera));
        std::unique_ptr<ApiObjects> deserialized = ApiObjects::Deserialize(serialized, m_resolverMock, "", m_errorReporting);

        ASSERT_TRUE(deserialized);

        ApiObjects& apiObjects = *deserialized;

        const ApiObjectContainer<LogicObject>& logicObjects = apiObjects.getApiObjectContainer<LogicObject>();
        const ApiObjectOwningContainer& ownedObjects = apiObjects.getApiObjectOwningContainer();
        ASSERT_EQ(9u, logicObjects.size());
        ASSERT_EQ(9u, ownedObjects.size());

        const std::vector<LogicObject*> expected{
            apiObjects.getApiObjectContainer<LuaModule>()[0],
            apiObjects.getApiObjectContainer<LuaScript>()[0],
            apiObjects.getApiObjectContainer<LuaInterface>()[0],
            apiObjects.getApiObjectContainer<RamsesNodeBinding>()[0],
            apiObjects.getApiObjectContainer<RamsesAppearanceBinding>()[0],
            apiObjects.getApiObjectContainer<RamsesCameraBinding>()[0],
            apiObjects.getApiObjectContainer<DataArray>()[0],
            apiObjects.getApiObjectContainer<AnimationNode>()[0],
            apiObjects.getApiObjectContainer<TimerNode>()[0]
        };

        for (size_t i = 0; i < expected.size(); ++i)
        {
            EXPECT_EQ(logicObjects[i], expected[i]);
            EXPECT_EQ(ownedObjects[i].get(), expected[i]);
        }
    }
}
