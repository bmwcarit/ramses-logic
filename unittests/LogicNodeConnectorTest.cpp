//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "internals/LogicNodeConnector.h"
#include "impl/PropertyImpl.h"
#include "impl/LogicNodeImpl.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"

#include "LogicNodeDummy.h"



namespace rlogic::internal
{
    class ALogicNodeConnector : public ::testing::Test
    {
    protected:
        LogicNodeConnector connector;

        LogicNodeDummyImpl m_nodeA{ "A", false };
        LogicNodeDummyImpl m_nodeB{ "B", false };

        PropertyImpl& m_outputA_1{ *m_nodeA.getOutputs()->getChild("output1")->m_impl };
        PropertyImpl& m_outputA_2{ *m_nodeA.getOutputs()->getChild("output2")->m_impl };
        PropertyImpl& m_inputB_1{ *m_nodeB.getInputs()->getChild("input1")->m_impl };
        PropertyImpl& m_inputB_2{ *m_nodeB.getInputs()->getChild("input2")->m_impl };
    };

    TEST_F(ALogicNodeConnector, HasNoLinksInDefaultState)
    {
        EXPECT_EQ(0u, connector.getLinks().size());

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));

        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_inputB_1));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_inputB_2));
        // TODO Violin this should trigger an assert! Make sure we check this method is called with inputs only, no outputs, and remove these two lines
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_outputA_1));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_outputA_2));
    }

    TEST_F(ALogicNodeConnector, LinksNodesSuccessfully_RemembersLinkData)
    {
        EXPECT_TRUE(connector.link(m_outputA_1, m_inputB_1));
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        EXPECT_EQ(&m_outputA_1, connector.getLinkedOutput(m_inputB_1));
        // Other input still not linked
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_inputB_2));
    }

    TEST_F(ALogicNodeConnector, RefusesToLinkTheSameOutputInputPairTwice)
    {
        EXPECT_TRUE(connector.link(m_outputA_1, m_inputB_1));
        EXPECT_FALSE(connector.link(m_outputA_1, m_inputB_1));

        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        EXPECT_EQ(&m_outputA_1, connector.getLinkedOutput(m_inputB_1));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_inputB_2));
    }

    TEST_F(ALogicNodeConnector, UnlinksInput_AfterLinkedSuccessfully)
    {
        EXPECT_TRUE(connector.link(m_outputA_1, m_inputB_1));

        connector.unlinkPrimitiveInput(m_inputB_1);
        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
    }

    TEST_F(ALogicNodeConnector, ConsidersNodeUnlinked_OnlyIfAllLinksDestroyed)
    {
        // Add a middle node, and link like this:
        // A -> M -> B
        LogicNodeDummyImpl nodeMiddle{ "M", false };
        const PropertyImpl& inputM_1{ *nodeMiddle.getInputs()->getChild("input1")->m_impl };
        const PropertyImpl& outputM_1{ *nodeMiddle.getOutputs()->getChild("output1")->m_impl };
        EXPECT_TRUE(connector.link(m_outputA_1, inputM_1));
        EXPECT_TRUE(connector.link(outputM_1, m_inputB_1));

        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(nodeMiddle));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        // A    M -> B
        connector.unlinkPrimitiveInput(inputM_1);

        // Check input has no assigned output for source now
        EXPECT_EQ(nullptr, connector.getLinkedOutput(inputM_1));

        // 'source' is the only node with no links remaining
        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(nodeMiddle));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        // A    M    B
        connector.unlinkPrimitiveInput(m_inputB_1);

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(nodeMiddle));
        EXPECT_FALSE(connector.isLinked(m_nodeB));

        // Check inputs have no assigned outputs for source now
        EXPECT_EQ(nullptr, connector.getLinkedOutput(inputM_1));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_inputB_1));
        // No links left
        EXPECT_EQ(0u, connector.getLinks().size());
    }

    TEST_F(ALogicNodeConnector, UnlinkAll_DoesNotAffectLinksOfOtherNodes)
    {
        LogicNodeDummyImpl nodeM("middle", false);
        LogicNodeDummyImpl nodeC("target2", false);

        const PropertyImpl& outputA = *m_nodeA.getOutputs()->getChild("output1")->m_impl;
        const PropertyImpl& inputM  = *nodeM.getInputs()->getChild("input1")->m_impl;
        const PropertyImpl& outputM = *nodeM.getOutputs()->getChild("output1")->m_impl;
        const PropertyImpl& input1B  = *m_nodeB.getInputs()->getChild("input1")->m_impl;
        const PropertyImpl& input2B  = *m_nodeB.getInputs()->getChild("input2")->m_impl;
        const PropertyImpl& inputC  = *nodeC.getInputs()->getChild("input1")->m_impl;

        /*
            A    ->    M   --x2->  B
             \
               ------------>  C
        */
        EXPECT_TRUE(connector.link(outputA, inputM));
        EXPECT_TRUE(connector.link(outputM, input1B));
        EXPECT_TRUE(connector.link(outputM, input2B));
        EXPECT_TRUE(connector.link(outputA, inputC));

        // All nodes linked status
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(nodeM));
        EXPECT_TRUE(connector.isLinked(m_nodeB));
        EXPECT_TRUE(connector.isLinked(nodeC));

        // Check output -> input relations
        EXPECT_EQ(&outputA, connector.getLinkedOutput(inputM));
        EXPECT_EQ(&outputM, connector.getLinkedOutput(input1B));
        EXPECT_EQ(&outputM, connector.getLinkedOutput(input2B));
        EXPECT_EQ(&outputA, connector.getLinkedOutput(inputC));

        /*
            A          M            B
             \
               ------------>  C
        */
        connector.unlinkAll(nodeM);

        // M and B not connected any more
        EXPECT_FALSE(connector.isLinked(nodeM));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
        // Link from A to C still intact
        EXPECT_TRUE(connector.isLinked(nodeC));
        EXPECT_TRUE(connector.isLinked(m_nodeA));

        // Three links deleted, one remains
        EXPECT_EQ(nullptr, connector.getLinkedOutput(inputM));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(input1B));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(input2B));
        EXPECT_EQ(&outputA, connector.getLinkedOutput(inputC)); // This one is not affected!

        // One link left
        EXPECT_EQ(1u, connector.getLinks().size());
    }

    class ALogicNodeConnector_NestedLinks : public ::testing::Test
    {
    protected:
        LogicNodeConnector connector;

        LogicNodeDummyImpl m_nodeA{ "A", true };
        LogicNodeDummyImpl m_nodeB{ "B", true };

        PropertyImpl& m_nestedStructOutputA {*m_nodeA.getOutputs()->getChild("outputStruct")->getChild("nested")->m_impl};
        PropertyImpl& m_nestedStructInputB {*m_nodeB.getInputs()->getChild("inputStruct")->getChild("nested")->m_impl};

        PropertyImpl& m_nestedArrayOutputA{ *m_nodeA.getOutputs()->getChild("outputArray")->getChild(0)->m_impl };
        PropertyImpl& m_nestedArrayInputB{ *m_nodeB.getInputs()->getChild("inputArray")->getChild(0)->m_impl };
    };

    TEST_F(ALogicNodeConnector_NestedLinks, LinksAndUnlinksStructChildPropertiesSuccessfully)
    {
        EXPECT_TRUE(connector.link(m_nestedStructOutputA, m_nestedStructInputB));
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        EXPECT_EQ(&m_nestedStructOutputA, connector.getLinkedOutput(m_nestedStructInputB));
        // Exactly one link
        EXPECT_EQ(1u, connector.getLinks().size());

        EXPECT_TRUE(connector.unlinkPrimitiveInput(m_nestedStructInputB));

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
        EXPECT_TRUE(connector.getLinks().empty());
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedStructInputB));
    }

    TEST_F(ALogicNodeConnector_NestedLinks, LinksAndUnlinksArrayChildPropertiesSuccessfully)
    {
        EXPECT_TRUE(connector.link(m_nestedArrayOutputA, m_nestedArrayInputB));
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        EXPECT_EQ(&m_nestedArrayOutputA, connector.getLinkedOutput(m_nestedArrayInputB));
        // Exactly one link
        EXPECT_EQ(1u, connector.getLinks().size());

        EXPECT_TRUE(connector.unlinkPrimitiveInput(m_nestedArrayInputB));

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
        EXPECT_TRUE(connector.getLinks().empty());
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedArrayInputB));
    }

    // Hybrid == struct to array and vice-versa
    TEST_F(ALogicNodeConnector_NestedLinks, LinksAndUnlinksNestedProperties_HybridLinks)
    {
        EXPECT_TRUE(connector.link(m_nestedArrayOutputA, m_nestedStructInputB));
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        EXPECT_TRUE(connector.link(m_nestedStructOutputA, m_nestedArrayInputB));

        EXPECT_EQ(&m_nestedArrayOutputA, connector.getLinkedOutput(m_nestedStructInputB));
        EXPECT_EQ(&m_nestedStructOutputA, connector.getLinkedOutput(m_nestedArrayInputB));
        // Exactly two links
        EXPECT_EQ(2u, connector.getLinks().size());

        // Clean up links
        EXPECT_TRUE(connector.unlinkPrimitiveInput(m_nestedStructInputB));
        EXPECT_TRUE(connector.unlinkPrimitiveInput(m_nestedArrayInputB));

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
        EXPECT_TRUE(connector.getLinks().empty());
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedStructInputB));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedArrayInputB));
    }

    TEST_F(ALogicNodeConnector_NestedLinks, ConfidenceTest_ConsidersNodeUnlinked_IFF_AllNestedLinksDestroyed)
    {
        // Add a middle node, and link like this:
        // A -> M -> B
        LogicNodeDummyImpl nodeMiddle{ "M", true };
        const PropertyImpl& nestedInputM{ *nodeMiddle.getInputs()->getChild("inputArray")->getChild(0)->m_impl };
        const PropertyImpl& nestedOutputM{ *nodeMiddle.getOutputs()->getChild("outputStruct")->getChild("nested")->m_impl };

        EXPECT_TRUE(connector.link(m_nestedStructOutputA, nestedInputM));
        EXPECT_TRUE(connector.link(nestedOutputM, m_nestedArrayInputB));
        // To be sure, test that link pointers are valid
        EXPECT_EQ(&m_nestedStructOutputA, connector.getLinkedOutput(nestedInputM));
        EXPECT_EQ(&nestedOutputM, connector.getLinkedOutput(m_nestedArrayInputB));

        // All nodes are linked
        EXPECT_TRUE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(nodeMiddle));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        // A    M -> B
        EXPECT_TRUE(connector.unlinkPrimitiveInput(nestedInputM));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(nestedInputM));

        // 'source' is the only node with no links remaining
        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_TRUE(connector.isLinked(nodeMiddle));
        EXPECT_TRUE(connector.isLinked(m_nodeB));

        // A    M    B
        connector.unlinkPrimitiveInput(m_nestedArrayInputB);

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(nodeMiddle));
        EXPECT_FALSE(connector.isLinked(m_nodeB));

        // Check inputs have no assigned outputs for source now
        EXPECT_EQ(nullptr, connector.getLinkedOutput(nestedInputM));
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedArrayInputB));
        // No links left
        EXPECT_EQ(0u, connector.getLinks().size());
    }

    TEST_F(ALogicNodeConnector_NestedLinks, UnlinkAll_AlsoRemovesNestedOutgoingLinks)
    {
        EXPECT_TRUE(connector.link(m_nestedArrayOutputA, m_nestedStructInputB));
        EXPECT_TRUE(connector.link(m_nestedStructOutputA, m_nestedArrayInputB));
        // Exactly two links
        EXPECT_EQ(2u, connector.getLinks().size());

        // Clean up links
        connector.unlinkAll(m_nodeB);

        EXPECT_FALSE(connector.isLinked(m_nodeA));
        EXPECT_FALSE(connector.isLinked(m_nodeB));
        EXPECT_TRUE(connector.getLinks().empty());
        EXPECT_EQ(nullptr, connector.getLinkedOutput(m_nestedArrayInputB));
    }

}
