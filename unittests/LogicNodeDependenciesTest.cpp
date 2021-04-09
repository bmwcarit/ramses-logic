//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "internals/LogicNodeDependencies.h"
#include "internals/ErrorReporting.h"
#include "LogicNodeDummy.h"

namespace rlogic::internal
{
    class ALogicNodeDependencies : public ::testing::Test
    {
    protected:
        void expectSortedNodeOrder(std::initializer_list<LogicNodeImpl*> nodes)
        {
            EXPECT_THAT(*m_dependencies.getTopologicallySortedNodes(), ::testing::ElementsAreArray(nodes));
        }

        void expectUnsortedNodeOrder(std::initializer_list<LogicNodeImpl*> nodes)
        {
            EXPECT_THAT(*m_dependencies.getTopologicallySortedNodes(), ::testing::UnorderedElementsAreArray(nodes));
        }

        LogicNodeDummyImpl m_nodeA{ "A", false };
        LogicNodeDummyImpl m_nodeB{ "B", false };

        LogicNodeDependencies m_dependencies;
        ErrorReporting m_errorReporting;
    };

    TEST_F(ALogicNodeDependencies, IsEmptyAfterConstruction)
    {
        EXPECT_TRUE((*m_dependencies.getTopologicallySortedNodes()).empty());
        EXPECT_TRUE(m_dependencies.getLinks().empty());
    }

    TEST_F(ALogicNodeDependencies, RemovingNode_RemovesItFromAllLists)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.removeNode(m_nodeA);

        expectSortedNodeOrder({});
        EXPECT_TRUE(m_dependencies.getLinks().empty());
    }

    TEST_F(ALogicNodeDependencies, HasNoLinksAndSingleNode_Given_SingleDisconnectedNode)
    {
        m_dependencies.addNode(m_nodeA);

        expectSortedNodeOrder({&m_nodeA});
        EXPECT_TRUE(m_dependencies.getLinks().empty());
    }

    TEST_F(ALogicNodeDependencies, ConnectingTwoNodes_CreatesALink)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(m_nodeB);

        PropertyImpl& output = *m_nodeA.getOutputs()->getChild("output1")->m_impl;
        PropertyImpl& input = *m_nodeB.getInputs()->getChild("input1")->m_impl;

        EXPECT_TRUE(m_dependencies.link(output, input, m_errorReporting));

        // Sorted topologically
        expectSortedNodeOrder({ &m_nodeA, &m_nodeB });

        // Has exactly one link
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(1u, links.size());
        EXPECT_EQ(&output, links.at(&input));
        EXPECT_EQ(&output, m_dependencies.getLinkedOutput(input));
    }

    TEST_F(ALogicNodeDependencies, DisconnectingTwoNodes_RemovesLinks)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(m_nodeB);

        PropertyImpl& output = *m_nodeA.getOutputs()->getChild("output1")->m_impl;
        PropertyImpl& input = *m_nodeB.getInputs()->getChild("input1")->m_impl;

        EXPECT_TRUE(m_dependencies.link(output, input, m_errorReporting));
        EXPECT_TRUE(m_dependencies.unlink(output, input, m_errorReporting));

        // both nodes still there, but no ordering guarantees without the link
        expectUnsortedNodeOrder({ &m_nodeA, &m_nodeB });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(input));
    }

    TEST_F(ALogicNodeDependencies, RemovingSourceNode_RemovesLinks)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(m_nodeB);

        PropertyImpl& output = *m_nodeA.getOutputs()->getChild("output1")->m_impl;
        PropertyImpl& input = *m_nodeB.getInputs()->getChild("input1")->m_impl;
        EXPECT_TRUE(m_dependencies.link(output, input, m_errorReporting));

        m_dependencies.removeNode(m_nodeA);

        // only target node left
        expectSortedNodeOrder({ &m_nodeB });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(input));
    }

    TEST_F(ALogicNodeDependencies, RemovingTargetNode_RemovesLinks)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(m_nodeB);

        PropertyImpl& output = *m_nodeA.getOutputs()->getChild("output1")->m_impl;
        PropertyImpl& input = *m_nodeB.getInputs()->getChild("input1")->m_impl;
        EXPECT_TRUE(m_dependencies.link(output, input, m_errorReporting));

        m_dependencies.removeNode(m_nodeB);

        // only source node left
        expectSortedNodeOrder({ &m_nodeA });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(input));
    }

    TEST_F(ALogicNodeDependencies, RemovingMiddleNode_DoesNotAffectRelativeOrderOfOtherNodes)
    {
        LogicNodeDummyImpl nodeM{ "M", false };

        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(nodeM);
        m_dependencies.addNode(m_nodeB);

        // A   ->    M    ->   B
        //   \               /
        //      ---->-------
        EXPECT_TRUE(m_dependencies.link(*m_nodeA.getOutputs()->getChild("output1")->m_impl, *nodeM.getInputs()->getChild("input1")->m_impl, m_errorReporting));
        EXPECT_TRUE(m_dependencies.link(*nodeM.getOutputs()->getChild("output1")->m_impl, *m_nodeB.getInputs()->getChild("input1")->m_impl, m_errorReporting));
        EXPECT_TRUE(m_dependencies.link(*m_nodeA.getOutputs()->getChild("output2")->m_impl, *m_nodeB.getInputs()->getChild("input2")->m_impl, m_errorReporting));

        expectSortedNodeOrder({ &m_nodeA, &nodeM, &m_nodeB });

        m_dependencies.removeNode(nodeM);

        // only other two nodes left (A and B). Their relative order is not changed
        expectSortedNodeOrder({ &m_nodeA, &m_nodeB });

        // Only link A->B remains
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(1u, links.size());
    }

    TEST_F(ALogicNodeDependencies, ReversingDependencyOfTwoNodes_InvertsTopologicalOrder)
    {
        m_dependencies.addNode(m_nodeA);
        m_dependencies.addNode(m_nodeB);

        // Node A -> Node B  (output of node A linked to input of node B)
        PropertyImpl& inputB = *m_nodeB.getInputs()->getChild("input1")->m_impl;
        PropertyImpl& outputA = *m_nodeA.getOutputs()->getChild("output1")->m_impl;

        EXPECT_TRUE(m_dependencies.link(outputA, inputB, m_errorReporting));
        expectSortedNodeOrder({ &m_nodeA, &m_nodeB });

        // Reverse dependency
        // Node B -> Node A  (output of node B linked to input of node A)
        EXPECT_TRUE(m_dependencies.unlink(outputA, inputB, m_errorReporting));
        PropertyImpl& inputA = *m_nodeA.getInputs()->getChild("input1")->m_impl;
        PropertyImpl& outputB = *m_nodeB.getOutputs()->getChild("output1")->m_impl;

        EXPECT_TRUE(m_dependencies.link(outputB, inputA, m_errorReporting));

        // Still no disconnected nodes, but now topological order is B -> A
        expectSortedNodeOrder({ &m_nodeB, &m_nodeA });

        // Has exactly one link
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(1u, links.size());
        EXPECT_EQ(&outputB, links.at(&inputA));
        EXPECT_EQ(&outputB, m_dependencies.getLinkedOutput(inputA));
    }

    class ALogicNodeDependencies_NestedLinks : public ALogicNodeDependencies
    {
    protected:

        ALogicNodeDependencies_NestedLinks()
        {
            m_dependencies.addNode(m_nodeANested);
            m_dependencies.addNode(m_nodeBNested);

            m_nestedOutputA = m_nodeANested.getOutputs()->getChild("outputStruct")->getChild("nested")->m_impl.get();
            m_nestedInputB = m_nodeBNested.getInputs()->getChild("inputStruct")->getChild("nested")->m_impl.get();
            m_arrayOutputA = m_nodeANested.getOutputs()->getChild("outputArray")->getChild(0)->m_impl.get();
            m_arrayInputB = m_nodeBNested.getInputs()->getChild("inputArray")->getChild(0)->m_impl.get();
        }

        LogicNodeDummyImpl m_nodeANested{ "A", true };
        LogicNodeDummyImpl m_nodeBNested{ "B", true };

        PropertyImpl* m_nestedOutputA;
        PropertyImpl* m_nestedInputB;
        PropertyImpl* m_arrayOutputA;
        PropertyImpl* m_arrayInputB;
    };

    TEST_F(ALogicNodeDependencies_NestedLinks, ReportsErrorWhenUnlinkingStructInputs_BasedOnTheirType)
    {
        PropertyImpl* structProperty = m_nodeBNested.getInputs()->getChild("inputStruct")->m_impl.get();
        EXPECT_FALSE(m_dependencies.unlink(*m_nestedOutputA, *structProperty, m_errorReporting));
        EXPECT_EQ("Can't unlink properties of complex types directly!", m_errorReporting.getErrors()[0].message);
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, ReportsErrorWhenUnlinkingArrayInputs_BasedOnTheirType)
    {
        PropertyImpl* arrayProperty = m_nodeBNested.getInputs()->getChild("inputArray")->m_impl.get();
        EXPECT_FALSE(m_dependencies.unlink(*m_nestedOutputA, *arrayProperty, m_errorReporting));
        EXPECT_EQ("Can't unlink properties of complex types directly!", m_errorReporting.getErrors()[0].message);
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, ReportsErrorWhenUnlinkingStructs_WithLinkedChildren)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());

        // Still can't unlink complex type
        PropertyImpl* outputParentStruct = m_nodeANested.getOutputs()->getChild("outputStruct")->m_impl.get();
        PropertyImpl* inputParentStruct = m_nodeBNested.getInputs()->getChild("inputStruct")->m_impl.get();
        EXPECT_FALSE(m_dependencies.unlink(*outputParentStruct, *inputParentStruct, m_errorReporting));
        EXPECT_EQ("Can't unlink properties of complex types directly!", m_errorReporting.getErrors()[0].message);
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, ConnectingTwoNodes_CreatesALink)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));

        // Sorted topologically
        expectSortedNodeOrder({&m_nodeANested, &m_nodeBNested});

        // Has exactly one link
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(1u, links.size());
        EXPECT_EQ(m_nestedOutputA, links.at(m_nestedInputB));
        EXPECT_EQ(m_nestedOutputA, m_dependencies.getLinkedOutput(*m_nestedInputB));
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, DisconnectingTwoNodes_RemovesLinks)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));
        EXPECT_TRUE(m_dependencies.unlink(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));

        // both nodes still there, but no ordering guarantees without the link
        expectUnsortedNodeOrder({ &m_nodeANested, &m_nodeBNested });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(*m_nestedInputB));
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, RemovingSourceNode_RemovesLinks)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));

        m_dependencies.removeNode(m_nodeANested);

        // only target node left
        expectSortedNodeOrder({ &m_nodeBNested });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(*m_nestedInputB));
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, RemovingTargetNode_RemovesLinks)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));

        m_dependencies.removeNode(m_nodeBNested);

        // only source node left
        expectSortedNodeOrder({ &m_nodeANested });

        // No links
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(0u, links.size());
        EXPECT_EQ(nullptr, m_dependencies.getLinkedOutput(*m_nestedInputB));
    }

    TEST_F(ALogicNodeDependencies_NestedLinks, ReversingDependencyOfTwoNodes_InvertsTopologicalOrder)
    {
        EXPECT_TRUE(m_dependencies.link(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));
        expectSortedNodeOrder({ &m_nodeANested, &m_nodeBNested });

        // Reverse dependency
        // Node B -> Node A  (output of node B linked to input of node A)
        EXPECT_TRUE(m_dependencies.unlink(*m_nestedOutputA, *m_nestedInputB, m_errorReporting));
        PropertyImpl& nestedInputA = *m_nodeANested.getInputs()->getChild("inputStruct")->getChild("nested")->m_impl;
        PropertyImpl& nestedOutputB = *m_nodeBNested.getOutputs()->getChild("outputStruct")->getChild("nested")->m_impl;

        EXPECT_TRUE(m_dependencies.link(nestedOutputB, nestedInputA, m_errorReporting));

        // Still no disconnected nodes, but now topological order is B -> A
        expectSortedNodeOrder({ &m_nodeBNested, &m_nodeANested });

        // Has exactly one link
        const LinksMap& links = m_dependencies.getLinks();
        EXPECT_EQ(1u, links.size());
        EXPECT_EQ(&nestedOutputB, links.at(&nestedInputA));
        EXPECT_EQ(&nestedOutputB, m_dependencies.getLinkedOutput(nestedInputA));
    }
}
