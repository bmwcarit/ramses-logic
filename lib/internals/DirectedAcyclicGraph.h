//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <vector>
#include <optional>
#include <unordered_map>
#include <cstdint>

namespace rlogic::internal
{
    // We don't depend on any functionality of LogicNodeImpl here, just using the
    // pointer as a unique identifier of the node
    class LogicNodeImpl;

    // TODO narrow down the scope of this typedef
    using NodeVector = std::vector<LogicNodeImpl*>;

    // For short: DAG
    // This DAG is used to represent the "property links" in LogicNode's, but abstracts the individual links and only
    // counts the number of links between two nodes, not the actual properties which are linked (this info is stored in PropertyImpl).
    // Edge direction is equivalent to direction of data flow
    // inside the logic engine (outputs -> inputs). Node outgoing degree represent the
    // number of total links of node properties to other nodes' properties, i.e. if two nodes A and B have three connected
    // properties, and node A and C have two connected properties, then addEdge(A, B) will have been called 3 times,
    // addEdge(A, C) two times, and A will have outDegree=5.
    // Topological sort result is cached because it's sensitive for performance (and is only
    // executed once before update()
    class DirectedAcyclicGraph
    {
    private:
        // Mask the LogicNodeImpl type as Node for easier readability inside the class
        using Node = LogicNodeImpl;
    public:
        void addNode(Node& node);
        void removeNode(Node& node);
        [[nodiscard]] bool containsNode(Node& node) const;

        bool addEdge(Node& source, Node& target);
        void removeEdge(Node& source, const Node& target);

        [[nodiscard]] std::optional<NodeVector> getTopologicallySortedNodes() const;

        // For testing only
        size_t getInDegree(Node& node) const;
        size_t getOutDegree(Node& node) const;

    private:
        struct Edge
        {
            Node* target;
            // A "ref count" which remembers how many times addEdge() was called on a pair of nodes
            size_t multiplicity;
        };

        using EdgeList = std::vector<Edge>;

        std::vector<Node*> collectRootNodes() const;

        // Stores both nodes and their edges in one hashmap
        // If a node has no outgoing links, the 'EdgeList' is empty
        // Each entry in 'EdgeList' represents an edge to another node
        std::unordered_map<Node*, EdgeList> m_nodeOutgoingEdges;
    };
}
