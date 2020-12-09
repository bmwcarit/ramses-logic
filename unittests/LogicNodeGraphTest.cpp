//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "internals/LogicNodeGraph.h"

#include "ramses-logic/LogicNode.h"
#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "LogicNodeDummy.h"

#include <memory>
#include "gmock/gmock-matchers.h"

namespace rlogic::internal
{

    class AlogicNodeGraph : public ::testing::Test
    {
    };

    TEST_F(AlogicNodeGraph, ReturnsTwoNodesInRightOrder)
    {
        internal::LogicNodeGraph graph;

        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        graph.addLink(source->m_impl.get(), target->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        EXPECT_THAT(sortedNodes, ::testing::ElementsAre(&source->m_impl.get(), &target->m_impl.get()));
    }

    TEST_F(AlogicNodeGraph, DoesNotReturnNodesIfLinkIsRemoved)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        graph.addLink(source->m_impl.get(), target->m_impl.get());
        graph.removeLink(source->m_impl.get(), target->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        EXPECT_TRUE(sortedNodes.empty());
    }

    TEST_F(AlogicNodeGraph, ComputesRightOrderForComplexGraph)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");
        auto n5 = LogicNodeDummy::Create("N5");
        auto n6 = LogicNodeDummy::Create("N6");

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());
        graph.addLink(n2->m_impl.get(), n6->m_impl.get());
        graph.addLink(n3->m_impl.get(), n5->m_impl.get());
        graph.addLink(n3->m_impl.get(), n6->m_impl.get());
        graph.addLink(n4->m_impl.get(), n5->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        for (auto node : sortedNodes)
        {
            node->update();
        }

        EXPECT_LT(n1->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n1->m_node->updateId, n4->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n5->m_node->updateId);
        EXPECT_LT(n4->m_node->updateId, n5->m_node->updateId);
    }

    TEST_F(AlogicNodeGraph, ComputesRightOrderForComplexGraphAfterLinksAreChanged)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");
        auto n5 = LogicNodeDummy::Create("N5");
        auto n6 = LogicNodeDummy::Create("N6");

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());
        graph.addLink(n2->m_impl.get(), n6->m_impl.get());
        graph.addLink(n3->m_impl.get(), n5->m_impl.get());
        graph.addLink(n3->m_impl.get(), n6->m_impl.get());
        graph.addLink(n4->m_impl.get(), n5->m_impl.get());

        /*     -----
         *   /        \
         * N2 -- N3 -- N6 -- N1 -- N4 -- N5
         *          \                   /
         *           \ --------------- /
         *
         */

        graph.removeLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n6->m_impl.get(), n1->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        for (auto node : sortedNodes)
        {
            node->update();
        }

        EXPECT_LT(n2->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n6->m_node->updateId, n1->m_node->updateId);
        EXPECT_LT(n1->m_node->updateId, n4->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n4->m_node->updateId, n5->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n5->m_node->updateId);
    }

    TEST_F(AlogicNodeGraph, ComputesRightOrderIfANodeIsRemovedFromBeginning)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");
        auto n5 = LogicNodeDummy::Create("N5");
        auto n6 = LogicNodeDummy::Create("N6");

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());
        graph.addLink(n2->m_impl.get(), n6->m_impl.get());
        graph.addLink(n3->m_impl.get(), n5->m_impl.get());
        graph.addLink(n3->m_impl.get(), n6->m_impl.get());
        graph.addLink(n4->m_impl.get(), n5->m_impl.get());

        /*
         *       N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.removeLinksForNode(n2->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        for (auto node : sortedNodes)
        {
            node->update();
        }

        EXPECT_EQ(n2->m_node->updateId, 0u);

        EXPECT_LT(n1->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n1->m_node->updateId, n4->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n5->m_node->updateId);
        EXPECT_LT(n4->m_node->updateId, n5->m_node->updateId);
    }

    TEST_F(AlogicNodeGraph, ComputesRightOrderIfANodeIsRemovedFromEnd)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");
        auto n5 = LogicNodeDummy::Create("N5");
        auto n6 = LogicNodeDummy::Create("N6");

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());
        graph.addLink(n2->m_impl.get(), n6->m_impl.get());
        graph.addLink(n3->m_impl.get(), n5->m_impl.get());
        graph.addLink(n3->m_impl.get(), n6->m_impl.get());
        graph.addLink(n4->m_impl.get(), n5->m_impl.get());

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /
         *    /
         * N1 -- N4
         */

        graph.removeLinksForNode(n5->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        for (auto node : sortedNodes)
        {
            node->update();
        }

        EXPECT_EQ(n5->m_node->updateId, 0u);

        EXPECT_LT(n1->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n1->m_node->updateId, n4->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n3->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n3->m_node->updateId, n6->m_node->updateId);
    }

    TEST_F(AlogicNodeGraph, ComputesRightOrderIfANodeIsRemovedFromTheMiddle)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");
        auto n5 = LogicNodeDummy::Create("N5");
        auto n6 = LogicNodeDummy::Create("N6");

        /*     -----
         *   /        \
         * N2 -- N3 -- N6
         *     /    \
         *    /      \
         * N1 -- N4 -- N5
         */

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());
        graph.addLink(n2->m_impl.get(), n6->m_impl.get());
        graph.addLink(n3->m_impl.get(), n5->m_impl.get());
        graph.addLink(n3->m_impl.get(), n6->m_impl.get());
        graph.addLink(n4->m_impl.get(), n5->m_impl.get());


        /*     -----
         *   /        \
         * N2          N6
         * N1 -- N4 -- N5
         */

        graph.removeLinksForNode(n3->m_impl.get());

        graph.updateOrder();
        const LogicNodeVector& sortedNodes = graph.getOrderedNodesCache();
        for (auto node : sortedNodes)
        {
            node->update();
        }

        EXPECT_LT(n1->m_node->updateId, n4->m_node->updateId);
        EXPECT_LT(n2->m_node->updateId, n6->m_node->updateId);
        EXPECT_LT(n4->m_node->updateId, n5->m_node->updateId);
    }

    TEST_F(AlogicNodeGraph, ReturnsTrueForIsLinkedIfNodeIsLinked)
    {
        internal::LogicNodeGraph graph;
        LogicNodeDummyImpl::UpdateCounter = 0;

        auto n1 = LogicNodeDummy::Create("N1");
        auto n2 = LogicNodeDummy::Create("N2");
        auto n3 = LogicNodeDummy::Create("N3");
        auto n4 = LogicNodeDummy::Create("N4");

        graph.addLink(n1->m_impl.get(), n3->m_impl.get());
        graph.addLink(n1->m_impl.get(), n4->m_impl.get());
        graph.addLink(n2->m_impl.get(), n3->m_impl.get());

        EXPECT_TRUE(graph.isLinked(n1->m_impl.get()));
        EXPECT_TRUE(graph.isLinked(n2->m_impl.get()));
        EXPECT_TRUE(graph.isLinked(n3->m_impl.get()));
        EXPECT_TRUE(graph.isLinked(n4->m_impl.get()));

        graph.removeLink(n1->m_impl.get(), n3->m_impl.get());
        graph.removeLink(n1->m_impl.get(), n4->m_impl.get());

        EXPECT_FALSE(graph.isLinked(n1->m_impl.get()));
        EXPECT_TRUE(graph.isLinked(n2->m_impl.get()));
        EXPECT_TRUE(graph.isLinked(n3->m_impl.get()));
        EXPECT_FALSE(graph.isLinked(n4->m_impl.get()));
    }
}
