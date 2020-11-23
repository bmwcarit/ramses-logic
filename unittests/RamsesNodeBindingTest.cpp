//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock-matchers.h"

#include "RamsesTestUtils.h"

#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"

#include "impl/RamsesNodeBindingImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/LogicEngineImpl.h"

#include "ramses-client-api/Node.h"

namespace rlogic
{
    using rlogic::internal::ENodePropertyStaticIndex;

    class ARamsesNodeBinding : public ::testing::Test
    {
    protected:
        ARamsesNodeBinding()
            : m_testScene(*m_ramsesTestSetup.createScene())
        {
        }

        void TearDown() override {
            std::remove("OneBinding.bin");
            std::remove("WithRamsesNode.bin");
            std::remove("RamsesNodeDeleted.bin");
            std::remove("NoValuesSet.bin");
            std::remove("SomeValuesSet.bin");
        }

        RamsesTestSetup m_ramsesTestSetup;
        ramses::Scene& m_testScene;
        LogicEngine m_logicEngine;

        ramses::Node* createTestRamsesNode()
        {
            return m_testScene.createNode();
        }

        RamsesNodeBinding& getBindingByName(std::string_view name)
        {
            return **find_if(
                m_logicEngine.ramsesNodeBindings().begin(),
                m_logicEngine.ramsesNodeBindings().end(),
                [name](const RamsesNodeBinding* binding) { return binding->getName() == name; });
        }

        static void ExpectDefaultValues(ramses::Node& node, ENodePropertyStaticIndex prop)
        {
            switch (prop)
            {
            case ENodePropertyStaticIndex::Rotation:
                ExpectValues(node, ENodePropertyStaticIndex::Rotation, vec3f{ 0.0f, 0.0f, 0.0f });
                break;
            case ENodePropertyStaticIndex::Translation:
                ExpectValues(node, ENodePropertyStaticIndex::Translation, vec3f{ 0.0f, 0.0f, 0.0f });
                break;
            case ENodePropertyStaticIndex::Scaling:
                ExpectValues(node, ENodePropertyStaticIndex::Scaling, vec3f{ 1.0f, 1.0f, 1.0f });
                break;
            case ENodePropertyStaticIndex::Visibility:
                EXPECT_EQ(node.getVisibility(), ramses::EVisibilityMode::Visible);
                break;
            }
        }

        static void ExpectDefaultValues(ramses::Node& node)
        {
            ExpectDefaultValues(node, ENodePropertyStaticIndex::Translation);
            ExpectDefaultValues(node, ENodePropertyStaticIndex::Rotation);
            ExpectDefaultValues(node, ENodePropertyStaticIndex::Scaling);
            ExpectDefaultValues(node, ENodePropertyStaticIndex::Visibility);
        }

        static void ExpectValues(ramses::Node& node, ENodePropertyStaticIndex prop, vec3f expectedValues)
        {
            std::array<float, 3> values = { 0.0f, 0.0f, 0.0f };
            if (prop == ENodePropertyStaticIndex::Rotation)
            {
                ramses::ERotationConvention unused;
                node.getRotation(values[0], values[1], values[2], unused);
            }
            if (prop == ENodePropertyStaticIndex::Translation)
            {
                node.getTranslation(values[0], values[1], values[2]);
            }
            if (prop == ENodePropertyStaticIndex::Scaling)
            {
                node.getScaling(values[0], values[1], values[2]);
            }
            EXPECT_THAT(values, ::testing::ElementsAre(expectedValues[0], expectedValues[1], expectedValues[2]));
        }
    };

    TEST_F(ARamsesNodeBinding, KeepsNameProvidedDuringConstruction)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
        EXPECT_EQ("NodeBinding", nodeBinding.getName());
    }

    TEST_F(ARamsesNodeBinding, ReturnsNullptrForOutputs)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");
        EXPECT_EQ(nullptr, nodeBinding.getOutputs());
    }

    TEST_F(ARamsesNodeBinding, ProvidesAccessToAllNodePropertiesInItsInputs)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");

        auto inputs = nodeBinding.getInputs();
        ASSERT_NE(nullptr, inputs);
        EXPECT_EQ(4u, inputs->getChildCount());

        auto rotation = inputs->getChild("rotation");
        auto scaling = inputs->getChild("scaling");
        auto translation = inputs->getChild("translation");
        auto visibility = inputs->getChild("visibility");

        // Test that internal indices match properties resolved by name
        EXPECT_EQ(rotation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation)));
        EXPECT_EQ(scaling, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling)));
        EXPECT_EQ(translation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation)));
        EXPECT_EQ(visibility, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility)));

        ASSERT_NE(nullptr, rotation);
        EXPECT_EQ(EPropertyType::Vec3f, rotation->getType());
        EXPECT_EQ(0u, rotation->getChildCount());

        ASSERT_NE(nullptr, scaling);
        EXPECT_EQ(EPropertyType::Vec3f, scaling->getType());
        EXPECT_EQ(0u, scaling->getChildCount());

        ASSERT_NE(nullptr, translation);
        EXPECT_EQ(EPropertyType::Vec3f, translation->getType());
        EXPECT_EQ(0u, translation->getChildCount());

        ASSERT_NE(nullptr, visibility);
        EXPECT_EQ(EPropertyType::Bool, visibility->getType());
        EXPECT_EQ(0u, visibility->getChildCount());
    }

    TEST_F(ARamsesNodeBinding, MarksInputsAsInput)
    {
        auto*      nodeBinding     = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        auto       inputs     = nodeBinding->getInputs();
        const auto inputCount = inputs->getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            EXPECT_EQ(internal::EInputOutputProperty::Input, inputs->getChild(i)->m_impl->getInputOutputProperty());
        }
    }

    TEST_F(ARamsesNodeBinding, ReturnsNodePropertiesForInputsConst)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");
        const auto inputs = nodeBinding.getInputs();
        ASSERT_NE(nullptr, inputs);
        EXPECT_EQ(4u, inputs->getChildCount());

        auto rotation = inputs->getChild("rotation");
        auto scaling = inputs->getChild("scaling");
        auto translation = inputs->getChild("translation");
        auto visibility = inputs->getChild("visibility");

        ASSERT_NE(nullptr, rotation);
        EXPECT_EQ(EPropertyType::Vec3f, rotation->getType());
        EXPECT_EQ(0u, rotation->getChildCount());

        ASSERT_NE(nullptr, scaling);
        EXPECT_EQ(EPropertyType::Vec3f, scaling->getType());
        EXPECT_EQ(0u, scaling->getChildCount());

        ASSERT_NE(nullptr, translation);
        EXPECT_EQ(EPropertyType::Vec3f, translation->getType());
        EXPECT_EQ(0u, translation->getChildCount());

        ASSERT_NE(nullptr, visibility);
        EXPECT_EQ(EPropertyType::Bool, visibility->getType());
        EXPECT_EQ(0u, visibility->getChildCount());
    }

    TEST_F(ARamsesNodeBinding, ReturnsBoundRamsesNode)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        EXPECT_EQ(ramsesNode, nodeBinding.getRamsesNode());

        nodeBinding.setRamsesNode(nullptr);
        EXPECT_EQ(nullptr, nodeBinding.getRamsesNode());
    }

    TEST_F(ARamsesNodeBinding, DoesNotModifyRamsesWithoutUpdateBeingCalled)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        auto inputs = nodeBinding.getInputs();
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 0.1f, 0.2f, 0.3f });
        inputs->getChild("scaling")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
        inputs->getChild("translation")->set<vec3f>(vec3f{ 2.1f, 2.2f, 2.3f });
        inputs->getChild("visibility")->set<bool>(true);

        ExpectDefaultValues(*ramsesNode);
    }

    // This test is a bit too big, but splitting it creates a lot of test code duplication... Better keep it like this, it documents behavior quite well
    TEST_F(ARamsesNodeBinding, ModifiesRamsesOnUpdate_OnlyAfterExplicitlyAssignedToInputs)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        nodeBinding.m_nodeBinding->update();

        ExpectDefaultValues(*ramsesNode);

        auto inputs = nodeBinding.getInputs();
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 0.1f, 0.2f, 0.3f });

        // Updte not called yet -> still default values
        ExpectDefaultValues(*ramsesNode);

        nodeBinding.m_nodeBinding->update();
        // Only propagated rotation, the others still have default values
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 0.1f, 0.2f, 0.3f });
        ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Translation);
        ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Scaling);
        ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Visibility);

        // Set and test all properties
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 42.1f, 42.2f, 42.3f });
        inputs->getChild("scaling")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
        inputs->getChild("translation")->set<vec3f>(vec3f{ 2.1f, 2.2f, 2.3f });
        inputs->getChild("visibility")->set<bool>(true);
        nodeBinding.m_nodeBinding->update();

        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 42.1f, 42.2f, 42.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 1.1f, 1.2f, 1.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 2.1f, 2.2f, 2.3f });
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Visible);

        // Set visibility again, because it only has 2 states
        // need to change state again because default ramses state is  already 'visible'
        inputs->getChild("visibility")->set<bool>(false);
        nodeBinding.m_nodeBinding->update();
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);
    }

    TEST_F(ARamsesNodeBinding, PropagatesItsInputsToRamsesNodeOnUpdate)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        auto inputs = nodeBinding.getInputs();
        inputs->getChild("rotation")->set<vec3f>(vec3f{0.1f, 0.2f, 0.3f});
        inputs->getChild("scaling")->set<vec3f>(vec3f{1.1f, 1.2f, 1.3f});
        inputs->getChild("translation")->set<vec3f>(vec3f{2.1f, 2.2f, 2.3f});
        inputs->getChild("visibility")->set<bool>(true);

        nodeBinding.m_nodeBinding->update();

        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 0.1f, 0.2f, 0.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 1.1f, 1.2f, 1.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 2.1f, 2.2f, 2.3f });
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Visible);
    }

    TEST_F(ARamsesNodeBinding, DoesNotOverrideExistingValuesAfterNodeIsBound)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");

        ramses::Node* ramsesNode = createTestRamsesNode();
        ramsesNode->setVisibility(ramses::EVisibilityMode::Off);
        ramsesNode->setRotation(0.1f, 0.2f, 0.3f, ramses::ERotationConvention::XYZ);
        ramsesNode->setScaling(1.1f, 1.2f, 1.3f);
        ramsesNode->setTranslation(2.1f, 2.2f, 2.3f);

        EXPECT_TRUE(nodeBinding.setRamsesNode(ramsesNode));

        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 0.1f, 0.2f, 0.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 1.1f, 1.2f, 1.3f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 2.1f, 2.2f, 2.3f });
    }

    TEST_F(ARamsesNodeBinding, StopsPropagatingValuesAfterTargetNodeSetToNull)
    {
        LogicEngine engine;
        RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");

        ramses::Node* ramsesNode = createTestRamsesNode();
        ramsesNode->setVisibility(ramses::EVisibilityMode::Off);
        EXPECT_TRUE(nodeBinding.setRamsesNode(ramsesNode));

        auto inputs = nodeBinding.getInputs();
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 0.1f, 0.2f, 0.3f });

        nodeBinding.m_nodeBinding->update();

        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 0.1f, 0.2f, 0.3f });

        inputs->getChild("rotation")->set<vec3f>(vec3f{ 5.1f, 5.2f, 5.3f });
        EXPECT_TRUE(nodeBinding.setRamsesNode(nullptr));
        EXPECT_TRUE(nodeBinding.m_nodeBinding->update());
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 0.1f, 0.2f, 0.3f });
    }

    TEST_F(ARamsesNodeBinding, ContainsItsInputsAfterDeserialization)
    {
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.getInputs()->getChild("rotation")->set<vec3f>(vec3f{ 0.1f, 0.2f, 0.3f });
            nodeBinding.getInputs()->getChild("translation")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
            nodeBinding.getInputs()->getChild("scaling")->set<vec3f>(vec3f{ 2.1f, 2.2f, 2.3f });
            nodeBinding.getInputs()->getChild("visibility")->set(true);
            EXPECT_TRUE(engine.saveToFile("OneBinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("OneBinding.bin"));
            const auto& nodeBinding = getBindingByName("NodeBinding");
            EXPECT_EQ("NodeBinding", nodeBinding.getName());

            const auto& inputs = nodeBinding.getInputs();
            ASSERT_EQ(inputs->getChildCount(), 4u);

            auto rotation       = inputs->getChild("rotation");
            auto translation    = inputs->getChild("translation");
            auto scaling        = inputs->getChild("scaling");
            auto visibility     = inputs->getChild("visibility");
            ASSERT_NE(nullptr, rotation);
            EXPECT_EQ("rotation", rotation->getName());
            EXPECT_EQ(EPropertyType::Vec3f, rotation->getType());
            EXPECT_EQ(internal::EInputOutputProperty::Input, rotation->m_impl->getInputOutputProperty());
            EXPECT_THAT(*rotation->get<vec3f>(), ::testing::ElementsAre(0.1f, 0.2f, 0.3f));
            ASSERT_NE(nullptr, translation);
            EXPECT_EQ("translation", translation->getName());
            EXPECT_EQ(EPropertyType::Vec3f, translation->getType());
            EXPECT_EQ(internal::EInputOutputProperty::Input, translation->m_impl->getInputOutputProperty());
            EXPECT_THAT(*translation->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
            ASSERT_NE(nullptr, scaling);
            EXPECT_EQ("scaling", scaling->getName());
            EXPECT_EQ(EPropertyType::Vec3f, scaling->getType());
            EXPECT_EQ(internal::EInputOutputProperty::Input, scaling->m_impl->getInputOutputProperty());
            EXPECT_THAT(*scaling->get<vec3f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f));
            ASSERT_NE(nullptr, visibility);
            EXPECT_EQ("visibility", visibility->getName());
            EXPECT_EQ(EPropertyType::Bool, visibility->getType());
            EXPECT_EQ(internal::EInputOutputProperty::Input, visibility->m_impl->getInputOutputProperty());
            EXPECT_TRUE(*visibility->get<bool>());

            // Test that internal indices match properties resolved by name
            EXPECT_EQ(rotation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation)));
            EXPECT_EQ(scaling, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling)));
            EXPECT_EQ(translation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation)));
            EXPECT_EQ(visibility, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility)));
        }
    }


    TEST_F(ARamsesNodeBinding, RestoresLinkToRamsesNodeAfterLoadingFromFile)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(engine.saveToFile("OneBinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("OneBinding.bin", &m_testScene));
            const auto& nodeBinding = getBindingByName("NodeBinding");
            EXPECT_EQ(nodeBinding.getRamsesNode(), ramsesNode);
        }
    }

    TEST_F(ARamsesNodeBinding, ProducesErrorWhenDeserializingFromFile_WhenHavingLinkToRamsesNode_ButNoSceneWasProvided)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(engine.saveToFile("WithRamsesNode.bin"));
        }
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("WithRamsesNode.bin"));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0], "Fatal error during loading from file! Serialized ramses logic object 'NodeBinding' points to a Ramses object (id: 1) , but no Ramses scene was provided to resolve the Ramses object!");
        }
    }

    TEST_F(ARamsesNodeBinding, ProducesErrorWhenDeserializingFromFile_WhenHavingLinkToRamsesNode_WhichWasDeleted)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(engine.saveToFile("RamsesNodeDeleted.bin"));
        }

        m_testScene.destroy(*ramsesNode);

        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("RamsesNodeDeleted.bin", &m_testScene));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0], "Fatal error during loading from file! Serialized ramses logic object NodeBinding points to a Ramses object (id: 1) which couldn't be found in the provided scene!");
        }
    }

    TEST_F(ARamsesNodeBinding, DoesNotModifyRamsesNodePropertiesAfterLoadingFromFile_WhenNoValuesWereExplicitlySetBeforeSaving)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(engine.saveToFile("NoValuesSet.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("NoValuesSet.bin", &m_testScene));
            EXPECT_TRUE(m_logicEngine.update());

            ExpectDefaultValues(*ramsesNode);
        }
    }

    TEST_F(ARamsesNodeBinding, SetsOnlyRamsesNodePropertiesWhichWereSetExplicitly_AfterLoadingFromFile_AndCallingUpdate)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine engine;
            RamsesNodeBinding& nodeBinding = *engine.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            nodeBinding.getInputs()->getChild("translation")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
            nodeBinding.getInputs()->getChild("visibility")->set(false);
            EXPECT_TRUE(engine.saveToFile("SomeValuesSet.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("SomeValuesSet.bin", &m_testScene));

            // No change before update is called
            ExpectDefaultValues(*ramsesNode);

            EXPECT_TRUE(m_logicEngine.update());

            // Rotation and Scaling were not set -> they have default values
            ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Rotation);
            ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Scaling);
            // Translation and visibility were set -> values were updated
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 1.1f, 1.2f, 1.3f });
            EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);
        }
    }
}
