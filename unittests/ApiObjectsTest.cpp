//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "generated/logicengine_gen.h"

#include "internals/ApiObjects.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"

#include "impl/PropertyImpl.h"
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
#include "LogTestUtils.h"
#include "RamsesObjectResolverMock.h"

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

        // Silence logs, unless explicitly enabled, to reduce spam and speed up tests
        ScopedLogContextLevel m_silenceLogs{ ELogMessageType::Off };
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

    class AnApiObjects_Serialization : public AnApiObjects
    {
    };

    TEST_F(AnApiObjects_Serialization, AlwaysCreatesEmptyFlatbuffersContainers_WhenNoObjectsPresent)
    {
        // Create without API objects -> serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
            ApiObjects toSerialize;
            ApiObjects::Serialize(toSerialize, builder);
        }

        const rlogic_serialization::LogicEngine& serializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        // Has all containers, size = 0 because no content
        ASSERT_NE(nullptr, serializedLogicEngine.luascripts());
        ASSERT_EQ(0u, serializedLogicEngine.luascripts()->size());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsesnodebindings());
        ASSERT_EQ(0u, serializedLogicEngine.ramsesnodebindings()->size());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsesappearancebindings());
        ASSERT_EQ(0u, serializedLogicEngine.ramsesappearancebindings()->size());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsescamerabindings());
        ASSERT_EQ(0u, serializedLogicEngine.ramsescamerabindings()->size());

        ASSERT_NE(nullptr, serializedLogicEngine.links());
        ASSERT_EQ(0u, serializedLogicEngine.links()->size());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainer_ForScripts)
    {
        // Create test flatbuffer with only a script
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
            ApiObjects toSerialize;
            toSerialize.createLuaScript(tempState, m_valid_empty_script, "", "script", m_errorReporting);
            ApiObjects::Serialize(toSerialize, builder);
        }

        const rlogic_serialization::LogicEngine& serializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        ASSERT_NE(nullptr, serializedLogicEngine.luascripts());
        ASSERT_EQ(1u, serializedLogicEngine.luascripts()->size());
        const rlogic_serialization::LuaScript& serializedScript = *serializedLogicEngine.luascripts()->Get(0);
        EXPECT_EQ(std::string(m_valid_empty_script), serializedScript.source()->str());
        EXPECT_EQ("script", serializedScript.logicnode()->name()->str());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainers_ForBindings)
    {
        // Create test flatbuffer with only a node binding
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
            ApiObjects toSerialize;
            toSerialize.createRamsesNodeBinding("node");
            toSerialize.createRamsesAppearanceBinding("appearance");
            toSerialize.createRamsesCameraBinding("camera");
            ApiObjects::Serialize(toSerialize, builder);
        }

        const rlogic_serialization::LogicEngine& serializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsesnodebindings());
        ASSERT_EQ(1u, serializedLogicEngine.ramsesnodebindings()->size());
        const rlogic_serialization::RamsesNodeBinding& serializedNodeBinding = *serializedLogicEngine.ramsesnodebindings()->Get(0);
        EXPECT_EQ("node", serializedNodeBinding.logicnode()->name()->str());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsesappearancebindings());
        ASSERT_EQ(1u, serializedLogicEngine.ramsesappearancebindings()->size());
        const rlogic_serialization::RamsesAppearanceBinding& serializedAppBinding = *serializedLogicEngine.ramsesappearancebindings()->Get(0);
        EXPECT_EQ("appearance", serializedAppBinding.logicnode()->name()->str());

        ASSERT_NE(nullptr, serializedLogicEngine.ramsescamerabindings());
        ASSERT_EQ(1u, serializedLogicEngine.ramsescamerabindings()->size());
        const rlogic_serialization::RamsesCameraBinding& serializedCameraBinding = *serializedLogicEngine.ramsescamerabindings()->Get(0);
        EXPECT_EQ("camera", serializedCameraBinding.logicnode()->name()->str());
    }

    TEST_F(AnApiObjects_Serialization, CreatesFlatbufferContainers_ForLinks)
    {
        // Create test flatbuffer with a link between script and binding
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
            ApiObjects toSerialize;

            // Create unused script and node binding to enforce a non-zero index for the linked objects
            toSerialize.createLuaScript(tempState, m_valid_empty_script, "", "script", m_errorReporting);
            toSerialize.createRamsesNodeBinding("");

            const std::string_view scriptWithOutput = R"(
                function interface()
                    OUT.nested = {
                        unused = FLOAT,
                        rotation = VEC3F
                    }
                end
                function run()
                end
            )";

            LuaScript* script = toSerialize.createLuaScript(tempState, scriptWithOutput, "", "", m_errorReporting);
            RamsesNodeBinding* nodeBinding = toSerialize.createRamsesNodeBinding("");
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script->getOutputs()->getChild("nested")->getChild("rotation")->m_impl,
                *nodeBinding->getInputs()->getChild("rotation")->m_impl,
                m_errorReporting));
            ApiObjects::Serialize(toSerialize, builder);
        }

        const rlogic_serialization::LogicEngine& serializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        // Asserts both script and binding objects existence
        ASSERT_EQ(2u, serializedLogicEngine.luascripts()->size());
        ASSERT_EQ(2u, serializedLogicEngine.ramsesnodebindings()->size());

        ASSERT_NE(nullptr, serializedLogicEngine.links());
        ASSERT_EQ(1u, serializedLogicEngine.links()->size());
        const rlogic_serialization::Link& link = *serializedLogicEngine.links()->Get(0);

        EXPECT_EQ(rlogic_serialization::ELogicNodeType::Script, link.sourceLogicNodeType());
        EXPECT_EQ(rlogic_serialization::ELogicNodeType::RamsesNodeBinding, link.targetLogicNodeType());
        // "source" is a nested link - parent index is 1 (the second script)
        EXPECT_EQ(1u, link.sourceLogicNodeId());
        ASSERT_EQ(2u, link.sourcePropertyNestedIndex()->size());
        ASSERT_EQ(0u, link.sourcePropertyNestedIndex()->Get(0)); // Index of "nested" wrt. parent "OUT" struct
        ASSERT_EQ(1u, link.sourcePropertyNestedIndex()->Get(1)); // Index of "rotation" wrt. parent "nested" struct

        EXPECT_EQ(1u, link.targetLogicNodeId());
        ASSERT_EQ(1u, link.targetPropertyNestedIndex()->size());
        ASSERT_EQ(static_cast<uint32_t>(ENodePropertyStaticIndex::Rotation), link.targetPropertyNestedIndex()->Get(0)); // Index of "rotation" wrt. parent "IN" struct
    }

    TEST_F(AnApiObjects_Serialization, ReConstructsImplMappingsWhenCreatedFromDeserializedData)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
            ApiObjects toSerialize;
            toSerialize.createLuaScript(tempState, m_valid_empty_script, "", "script", m_errorReporting);
            toSerialize.createRamsesNodeBinding("node");
            toSerialize.createRamsesAppearanceBinding("appearance");
            toSerialize.createRamsesCameraBinding("camera");

            ApiObjects::Serialize(toSerialize, builder);
        }

        RamsesObjectResolverMock resolverMock;

        auto& deSerializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        std::optional<ApiObjects> apiObjectsOptional = ApiObjects::Deserialize(m_state, deSerializedLogicEngine, resolverMock, "", m_errorReporting);

        ASSERT_TRUE(apiObjectsOptional);

        ApiObjects& apiObjects = *apiObjectsOptional;

        ASSERT_EQ(4u, apiObjects.getReverseImplMapping().size());

        LuaScript* script = apiObjects.getScripts()[0].get();
        EXPECT_EQ(script, apiObjects.getApiObject(script->m_impl));
        EXPECT_EQ(script->getName(), "script");

        RamsesNodeBinding* nodeBinding = apiObjects.getNodeBindings()[0].get();
        EXPECT_EQ(nodeBinding, apiObjects.getApiObject(nodeBinding->m_impl));
        EXPECT_EQ(nodeBinding->getName(), "node");

        RamsesAppearanceBinding* appBinding = apiObjects.getAppearanceBindings()[0].get();
        EXPECT_EQ(appBinding, apiObjects.getApiObject(appBinding->m_impl));
        EXPECT_EQ(appBinding->getName(), "appearance");

        RamsesCameraBinding* camBinding = apiObjects.getCameraBindings()[0].get();
        EXPECT_EQ(camBinding, apiObjects.getApiObject(camBinding->m_impl));
        EXPECT_EQ(camBinding->getName(), "camera");
    }

    TEST_F(AnApiObjects_Serialization, ReConstructsLinksWhenCreatedFromDeserializedData)
    {
        // Create dummy data and serialize
        flatbuffers::FlatBufferBuilder builder;
        {
            SolState tempState;
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

            LuaScript* script1 = toSerialize.createLuaScript(tempState, scriptForLinks, "", "", m_errorReporting);
            LuaScript* script2 = toSerialize.createLuaScript(tempState, scriptForLinks, "", "", m_errorReporting);
            ASSERT_TRUE(toSerialize.getLogicNodeDependencies().link(
                *script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl,
                *script2->getInputs()->getChild("integer")->m_impl,
                m_errorReporting));

            ApiObjects::Serialize(toSerialize, builder);
        }

        RamsesObjectResolverMock resolverMock;

        auto& serializedLogicEngine = *rlogic_serialization::GetLogicEngine(builder.GetBufferPointer());

        std::optional<ApiObjects> apiObjectsOptional = ApiObjects::Deserialize(m_state, serializedLogicEngine, resolverMock, "", m_errorReporting);

        ASSERT_TRUE(apiObjectsOptional);

        ApiObjects& apiObjects = *apiObjectsOptional;

        LuaScript* script1 = apiObjects.getScripts()[0].get();
        ASSERT_TRUE(script1);

        LuaScript* script2 = apiObjects.getScripts()[1].get();
        ASSERT_TRUE(script2);

        const PropertyImpl* linkedOutput = apiObjects.getLogicNodeDependencies().getLinkedOutput(*script2->getInputs()->getChild("integer")->m_impl);
        EXPECT_EQ(script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl.get(), linkedOutput);

        // Test some more internal data (because of the fragile state of link's deserialization). Consider removing if we refactor the code
        const LinksMap& linkMap = apiObjects.getLogicNodeDependencies().getLinks();
        ASSERT_EQ(1u, linkMap.size());
        EXPECT_EQ(linkMap.begin()->second, script1->getOutputs()->getChild("nested")->getChild("integer")->m_impl.get());
        EXPECT_EQ(linkMap.begin()->first, script2->getInputs()->getChild("integer")->m_impl.get());
    }
}
