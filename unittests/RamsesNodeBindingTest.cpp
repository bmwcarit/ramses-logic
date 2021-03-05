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

        RamsesTestSetup m_ramsesTestSetup;
        ramses::Scene& m_testScene;
        LogicEngine m_logicEngine;

        ramses::Node* createTestRamsesNode()
        {
            return m_testScene.createNode();
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
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");
        EXPECT_EQ("NodeBinding", nodeBinding.getName());
    }

    TEST_F(ARamsesNodeBinding, ReturnsNullptrForOutputs)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");
        EXPECT_EQ(nullptr, nodeBinding.getOutputs());
    }

    TEST_F(ARamsesNodeBinding, ProvidesAccessToAllNodePropertiesInItsInputs)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

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

    TEST_F(ARamsesNodeBinding, InitializesInputPropertiesToMatchRamsesDefaultValues)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

        auto inputs = nodeBinding.getInputs();
        ASSERT_NE(nullptr, inputs);
        EXPECT_EQ(4u, inputs->getChildCount());


        vec3f zeroes;
        vec3f ones;

        ramses::ERotationConvention rotationConvention;

        //Check that the default values we assume are indeed the ones in ramses
        ramses::Node* ramsesNode = createTestRamsesNode();
        ramsesNode->getRotation(zeroes[0], zeroes[1], zeroes[2], rotationConvention);
        EXPECT_THAT(zeroes, ::testing::ElementsAre(0.f, 0.f, 0.f));

        //TODO Violin enable this once fix is merged in R27.0.5
        //EXPECT_EQ(rotationConvention, ramses::ERotationConvention::XYZ);
        ramsesNode->getTranslation(zeroes[0], zeroes[1], zeroes[2]);
        EXPECT_THAT(zeroes, ::testing::ElementsAre(0.f, 0.f, 0.f));
        ramsesNode->getScaling(ones[0], ones[1], ones[2]);
        EXPECT_THAT(ones, ::testing::ElementsAre(1.f, 1.f, 1.f));
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Visible);

        EXPECT_EQ(zeroes, *inputs->getChild("rotation")->get<vec3f>());
        EXPECT_EQ(zeroes, *inputs->getChild("translation")->get<vec3f>());
        EXPECT_EQ(ones, *inputs->getChild("scaling")->get<vec3f>());
        EXPECT_EQ(true, *inputs->getChild("visibility")->get<bool>());
    }

    TEST_F(ARamsesNodeBinding, MarksInputsAsBindingInputs)
    {
        auto*      nodeBinding     = m_logicEngine.createRamsesNodeBinding("NodeBinding");
        auto       inputs     = nodeBinding->getInputs();
        const auto inputCount = inputs->getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, inputs->getChild(i)->m_impl->getPropertySemantics());
        }
    }

    TEST_F(ARamsesNodeBinding, ReturnsNodePropertiesForInputsConst)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");
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
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        EXPECT_EQ(ramsesNode, nodeBinding.getRamsesNode());

        nodeBinding.setRamsesNode(nullptr);
        EXPECT_EQ(nullptr, nodeBinding.getRamsesNode());
    }

    TEST_F(ARamsesNodeBinding, DoesNotModifyRamsesWithoutUpdateBeingCalled)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

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
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

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
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");

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

    TEST_F(ARamsesNodeBinding, PropagatesItsInputsToRamsesNodeOnUpdate_WithLinksInsteadOfSetCall)
    {
        const std::string_view scriptSrc = R"(
            function interface()
                OUT.rotation = VEC3F
                OUT.visibility = BOOL
            end
            function run()
                OUT.rotation = {1, 2, 3}
                OUT.visibility = false
            end
        )";

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);

        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");

        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("visibility"), *nodeBinding.getInputs()->getChild("visibility")));

        // Links have no effect before update() explicitly called
        ExpectDefaultValues(*ramsesNode);

        m_logicEngine.update();

        // Linked values got updates, not-linked values were not modified
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 2.f, 3.f });
        ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Scaling);
        ExpectDefaultValues(*ramsesNode, ENodePropertyStaticIndex::Translation);
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);
    }

    TEST_F(ARamsesNodeBinding, DoesNotOverrideExistingValuesAfterRamsesNodeIsAssignedToBinding)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");

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
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");

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

    TEST_F(ARamsesNodeBinding, HasSameDefaultRotationConventionAsRamsesNode)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");
        ramses::Node* ramsesNode = createTestRamsesNode();

        float unused = 0.f;
        ramses::ERotationConvention rotationConvention;
        ramsesNode->getRotation(unused, unused, unused, rotationConvention);

        EXPECT_EQ(rotationConvention, nodeBinding.getRotationConvention());
    }

    TEST_F(ARamsesNodeBinding, ChangesToRotationConventionArePassedToRamses)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");
        ramses::Node* ramsesNode = createTestRamsesNode();
        nodeBinding.setRamsesNode(ramsesNode);

        vec3f vec3;
        ramses::ERotationConvention rotationConvention;

        nodeBinding.getInputs()->getChild("rotation")->set<vec3f>({ 1, 2, 3 });
        EXPECT_TRUE(m_logicEngine.update());

        ramsesNode->getRotation(vec3[0], vec3[1], vec3[2], rotationConvention);
        EXPECT_THAT(vec3, ::testing::ElementsAre(1.f, 2.f, 3.f));
        EXPECT_EQ(rotationConvention, ramses::ERotationConvention::XYZ);

        nodeBinding.setRotationConvention(ramses::ERotationConvention::ZYX);
        nodeBinding.getInputs()->getChild("rotation")->set<vec3f>({ 15, 0, 5 });
        EXPECT_TRUE(m_logicEngine.update());

        ramsesNode->getRotation(vec3[0], vec3[1], vec3[2], rotationConvention);
        EXPECT_THAT(vec3, ::testing::ElementsAre(15.f, 0.f, 5.f));
        EXPECT_EQ(rotationConvention, ramses::ERotationConvention::ZYX);

    }

    class ARamsesNodeBinding_WithSerialization : public ARamsesNodeBinding
    {
    protected:
        void TearDown() override
        {
            std::remove("OneBinding.bin");
            std::remove("WithRamsesNode.bin");
            std::remove("RamsesNodeDeleted.bin");
            std::remove("NoValuesSet.bin");
            std::remove("SomeValuesSet.bin");
            std::remove("AllValuesSet.bin");
            std::remove("SomeInputsLinked.bin");
        }
    };

    TEST_F(ARamsesNodeBinding_WithSerialization, ContainsItsDataAfterDeserialization)
    {
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.getInputs()->getChild("rotation")->set<vec3f>(vec3f{ 0.1f, 0.2f, 0.3f });
            nodeBinding.getInputs()->getChild("translation")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
            nodeBinding.getInputs()->getChild("scaling")->set<vec3f>(vec3f{ 2.1f, 2.2f, 2.3f });
            nodeBinding.getInputs()->getChild("visibility")->set(true);
            nodeBinding.setRotationConvention(ramses::ERotationConvention::XZX);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("OneBinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("OneBinding.bin"));
            const auto& nodeBinding = *m_logicEngine.findNodeBinding("NodeBinding");
            EXPECT_EQ("NodeBinding", nodeBinding.getName());

            const auto& inputs = nodeBinding.getInputs();
            ASSERT_EQ(inputs->getChildCount(), 4u);

            auto rotation       = inputs->getChild("rotation");
            auto translation    = inputs->getChild("translation");
            auto scaling        = inputs->getChild("scaling");
            auto visibility     = inputs->getChild("visibility");
            EXPECT_EQ(ramses::ERotationConvention::XZX, nodeBinding.getRotationConvention());
            ASSERT_NE(nullptr, rotation);
            EXPECT_EQ("rotation", rotation->getName());
            EXPECT_EQ(EPropertyType::Vec3f, rotation->getType());
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, rotation->m_impl->getPropertySemantics());
            EXPECT_THAT(*rotation->get<vec3f>(), ::testing::ElementsAre(0.1f, 0.2f, 0.3f));
            ASSERT_NE(nullptr, translation);
            EXPECT_EQ("translation", translation->getName());
            EXPECT_EQ(EPropertyType::Vec3f, translation->getType());
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, translation->m_impl->getPropertySemantics());
            EXPECT_THAT(*translation->get<vec3f>(), ::testing::ElementsAre(1.1f, 1.2f, 1.3f));
            ASSERT_NE(nullptr, scaling);
            EXPECT_EQ("scaling", scaling->getName());
            EXPECT_EQ(EPropertyType::Vec3f, scaling->getType());
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, scaling->m_impl->getPropertySemantics());
            EXPECT_THAT(*scaling->get<vec3f>(), ::testing::ElementsAre(2.1f, 2.2f, 2.3f));
            ASSERT_NE(nullptr, visibility);
            EXPECT_EQ("visibility", visibility->getName());
            EXPECT_EQ(EPropertyType::Bool, visibility->getType());
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, visibility->m_impl->getPropertySemantics());
            EXPECT_TRUE(*visibility->get<bool>());

            // Test that internal indices match properties resolved by name
            EXPECT_EQ(rotation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation)));
            EXPECT_EQ(scaling, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling)));
            EXPECT_EQ(translation, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation)));
            EXPECT_EQ(visibility, inputs->m_impl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility)));
        }
    }


    TEST_F(ARamsesNodeBinding_WithSerialization, RestoresLinkToRamsesNodeAfterLoadingFromFile)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("OneBinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("OneBinding.bin", &m_testScene));
            const auto& nodeBinding = *m_logicEngine.findNodeBinding("NodeBinding");
            EXPECT_EQ(nodeBinding.getRamsesNode(), ramsesNode);
        }
    }

    TEST_F(ARamsesNodeBinding_WithSerialization, ProducesErrorWhenDeserializingFromFile_WhenHavingLinkToRamsesNode_ButNoSceneWasProvided)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("WithRamsesNode.bin"));
        }
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("WithRamsesNode.bin"));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0], "Fatal error during loading from file! Serialized ramses logic object 'NodeBinding' points to a Ramses object (id: 1) , but no Ramses scene was provided to resolve the Ramses object!");
        }
    }

    TEST_F(ARamsesNodeBinding_WithSerialization, ProducesErrorWhenDeserializingFromFile_WhenHavingLinkToRamsesNode_WhichWasDeleted)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("RamsesNodeDeleted.bin"));
        }

        m_testScene.destroy(*ramsesNode);

        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("RamsesNodeDeleted.bin", &m_testScene));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0], "Fatal error during loading from file! Serialized ramses logic object NodeBinding points to a Ramses object (id: 1) which couldn't be found in the provided scene!");
        }
    }

    TEST_F(ARamsesNodeBinding_WithSerialization, DoesNotModifyRamsesNodePropertiesAfterLoadingFromFile_WhenNoValuesWereExplicitlySetBeforeSaving)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("NoValuesSet.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("NoValuesSet.bin", &m_testScene));
            EXPECT_TRUE(m_logicEngine.update());

            ExpectDefaultValues(*ramsesNode);
        }
    }

    // Tests that the node properties don't overwrite ramses' values after loading from file, until
    // set() is called again explicitly after loadFromFile()
    TEST_F(ARamsesNodeBinding_WithSerialization, DoesNotReapplyPropertiesToRamsesAfterLoading_UntilExplicitlySetAgain)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();
        {
            LogicEngine tempEngineForSaving;
            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);
            // Set some values to the binding's inputs
            nodeBinding.getInputs()->getChild("translation")->set<vec3f>(vec3f{ 1.1f, 1.2f, 1.3f });
            nodeBinding.getInputs()->getChild("rotation")->set<vec3f>(vec3f{ 2.1f, 2.2f, 2.3f });
            nodeBinding.getInputs()->getChild("scaling")->set<vec3f>(vec3f{ 3.1f, 3.2f, 3.3f });
            nodeBinding.getInputs()->getChild("visibility")->set<bool>(true);
            EXPECT_TRUE(tempEngineForSaving.saveToFile("AllValuesSet.bin"));
        }

        // Set properties to other values to check if they are overwritten after load
        ramsesNode->setTranslation(100.f, 100.f, 100.f);
        ramsesNode->setRotation(100.f, 100.f, 100.f, ramses::ERotationConvention::XYZ);
        ramsesNode->setScaling(100.f, 100.f, 100.f);
        ramsesNode->setVisibility(ramses::EVisibilityMode::Invisible);

        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("AllValuesSet.bin", &m_testScene));

            EXPECT_TRUE(m_logicEngine.update());

            // Node binding does not re-apply its values to ramses node
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 100.f, 100.f, 100.f });
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 100.f, 100.f, 100.f });
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 100.f, 100.f, 100.f });
            EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);

            // Set only scaling. Use the same value as before save on purpose! Calling set forces set on ramses
            m_logicEngine.findNodeBinding("NodeBinding")->getInputs()->getChild("scaling")->set<vec3f>(vec3f{ 3.1f, 3.2f, 3.3f });
            EXPECT_TRUE(m_logicEngine.update());

            // Only scaling changed, the rest is unchanged
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 100.f, 100.f, 100.f });
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 100.f, 100.f, 100.f });
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 3.1f, 3.2f, 3.3f });
            EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);
        }
    }

    // This is sort of a confidence test, testing a combination of:
    // - bindings only propagating their values to ramses node if the value was set by an incoming link
    // - saving and loading files
    // The general expectation is that after loading + update(), the logic scene would overwrite only ramses
    // properties wrapped by a LogicBinding which is linked to a script
    TEST_F(ARamsesNodeBinding_WithSerialization, SetsOnlyRamsesNodePropertiesForWhichTheBindingInputIsLinked_AfterLoadingFromFile_AndCallingUpdate)
    {
        ramses::Node* ramsesNode = createTestRamsesNode();

        // These values should not be overwritten by logic on update()
        ramsesNode->setScaling(22, 33, 44);
        ramsesNode->setTranslation(100, 200, 300);

        {
            LogicEngine tempEngineForSaving;

            const std::string_view scriptSrc = R"(
                function interface()
                    OUT.rotation = VEC3F
                    OUT.visibility = BOOL
                end
                function run()
                    OUT.rotation = {1, 2, 3}
                    OUT.visibility = false
                end
            )";

            LuaScript* script = tempEngineForSaving.createLuaScriptFromSource(scriptSrc);

            RamsesNodeBinding& nodeBinding = *tempEngineForSaving.createRamsesNodeBinding("NodeBinding");
            nodeBinding.setRamsesNode(ramsesNode);

            ASSERT_TRUE(tempEngineForSaving.link(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
            ASSERT_TRUE(tempEngineForSaving.link(*script->getOutputs()->getChild("visibility"), *nodeBinding.getInputs()->getChild("visibility")));

            EXPECT_TRUE(tempEngineForSaving.saveToFile("SomeInputsLinked.bin"));
        }

        // Modify 'linked' properties before loading to check if logic will overwrite them after load + update
        ramsesNode->setRotation(100, 100, 100, ramses::ERotationConvention::XYZ);
        ramsesNode->setVisibility(ramses::EVisibilityMode::Off);

        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("SomeInputsLinked.bin", &m_testScene));

            EXPECT_TRUE(m_logicEngine.update());

            // Translation and Scaling were not linked -> their values are not modified
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, { 100.f, 200.f, 300.f });
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, { 22.f, 33.f, 44.f });
            // Rotation and visibility are linked -> values were updated
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 2.f, 3.f });
            EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);

            // Manually setting values on ramses followed by a logic update has no effect
            // Logic is not "dirty" and it doesn't know it needs to update ramses
            ramsesNode->setRotation(1, 2, 3, ramses::ERotationConvention::XYZ);
            EXPECT_TRUE(m_logicEngine.update());
            ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 2.f, 3.f });
        }
    }

    // Larger confidence tests which verify and document the entire data flow cycle of bindings
    // There are smaller tests which test only properties and their data propagation rules (see property unit tests)
    // There are also "dirtiness" tests which test when a node is being re-updated (see logic engine dirtiness tests)
    // These tests test everything in combination
    class ARamsesNodeBinding_DataFlow : public ARamsesNodeBinding
    {
    };

    TEST_F(ARamsesNodeBinding_DataFlow, WithExplicitSet)
    {
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("");

        // Create node and preset values
        ramses::Node* ramsesNode = createTestRamsesNode();
        ramsesNode->setRotation(1.f, 1.f, 1.f, ramses::ERotationConvention::XYZ);
        ramsesNode->setScaling(2.f, 2.f, 2.f);
        ramsesNode->setTranslation(3.f, 3.f, 3.f);
        ramsesNode->setVisibility(ramses::EVisibilityMode::Invisible);

        nodeBinding.setRamsesNode(ramsesNode);

        m_logicEngine.update();

        // Nothing happened - binding did not overwrite preset values because no user value set()
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 1.f, 1.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 2.f, 2.f, 2.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 3.f, 3.f, 3.f });
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);

        // Set rotation only
        Property* inputs = nodeBinding.getInputs();
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 42.f, 42.f, 42.f });

        // Update not called yet -> still has preset values for rotation in ramses node
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 1.f, 1.f });

        // Update() only propagates rotation and does not touch other data
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 42.f, 42.f, 42.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 2.f, 2.f, 2.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 3.f, 3.f, 3.f });
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Invisible);

        // Calling update again does not "rewrite" the data to ramses. Check this by setting a value manually and call update() again
        ramsesNode->setRotation(1.f, 1.f, 1.f, ramses::ERotationConvention::XYZ);
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 1.f, 1.f });

        // Set all properties manually this time
        inputs->getChild("rotation")->set<vec3f>(vec3f{ 100.f, 100.f, 100.f });
        inputs->getChild("scaling")->set<vec3f>(vec3f{ 200.f, 200.f, 200.f });
        inputs->getChild("translation")->set<vec3f>(vec3f{ 300.f, 300.f, 300.f });
        inputs->getChild("visibility")->set<bool>(true);
        m_logicEngine.update();

        // All of the property values were passed to ramses
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 100.f, 100.f, 100.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Scaling, vec3f{ 200.f, 200.f, 200.f });
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Translation, vec3f{ 300.f, 300.f, 300.f });
        EXPECT_EQ(ramsesNode->getVisibility(), ramses::EVisibilityMode::Visible);
    }

    TEST_F(ARamsesNodeBinding_DataFlow, WithLinks)
    {
        const std::string_view scriptSrc = R"(
            function interface()
                OUT.rotation = VEC3F
            end
            function run()
                OUT.rotation = {1, 2, 3}
            end
        )";

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);
        RamsesNodeBinding& nodeBinding = *m_logicEngine.createRamsesNodeBinding("NodeBinding");

        // Create node and preset values
        ramses::Node* ramsesNode = createTestRamsesNode();
        ramsesNode->setRotation(1.f, 1.f, 1.f, ramses::ERotationConvention::XYZ);
        ramsesNode->setScaling(2.f, 2.f, 2.f);
        ramsesNode->setTranslation(3.f, 3.f, 3.f);
        ramsesNode->setVisibility(ramses::EVisibilityMode::Off);

        nodeBinding.setRamsesNode(ramsesNode);

        // Adding and removing link does not set anything in ramses
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
        ASSERT_TRUE(m_logicEngine.unlink(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 1.f, 1.f });

        // Create link and calling update -> sets values to ramses
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 2.f, 3.f });

        // As long as link is active, binding overwrites value which was manually set directly to the ramses node
        ramsesNode->setRotation(100.f, 100.f, 100.f, ramses::ERotationConvention::XYZ);
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 1.f, 2.f, 3.f });

        // Remove link -> value is not overwritten any more
        ASSERT_TRUE(m_logicEngine.unlink(*script->getOutputs()->getChild("rotation"), *nodeBinding.getInputs()->getChild("rotation")));
        ramsesNode->setRotation(100.f, 100.f, 100.f, ramses::ERotationConvention::XYZ);
        m_logicEngine.update();
        ExpectValues(*ramsesNode, ENodePropertyStaticIndex::Rotation, vec3f{ 100.f, 100.f, 100.f });
    }
}
