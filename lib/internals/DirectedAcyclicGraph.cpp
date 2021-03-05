//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/DirectedAcyclicGraph.h"

#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <numeric>
#include <iterator>

namespace rlogic::internal
{
    void DirectedAcyclicGraph::addNode(Node& node)
    {
        assert(!containsNode(node));
        m_nodeOutgoingEdges.insert({ &node, std::vector<Edge>() });
    }

    void DirectedAcyclicGraph::removeNode(Node& nodeToRemove)
    {
        // First, remove all 'incoming edges' of the node by going through all outgoing edges
        // and filtering those out which have targetNode == nodeToRemove
        for (auto& nodeEntry : m_nodeOutgoingEdges)
        {
            // skip nodeToRemove, it can not have edges to itself
            if (nodeEntry.first != &nodeToRemove)
            {
                EdgeList& otherNodeOutgoingEdges = nodeEntry.second;

                otherNodeOutgoingEdges.erase(std::remove_if(otherNodeOutgoingEdges.begin(), otherNodeOutgoingEdges.end(), [&nodeToRemove](const Edge& edge)
                    {
                        return (edge.target == &nodeToRemove);
                    }), otherNodeOutgoingEdges.end());
            }
        }

        // Remove outgoing edges by simply removing the node from m_nodeOutgoingEdges
        assert(m_nodeOutgoingEdges.find(&nodeToRemove) != m_nodeOutgoingEdges.end());
        m_nodeOutgoingEdges.erase(&nodeToRemove);
    }

    // This is a slightly exotic sorting algorithm for DAGs
    // It works based on these general principles:
    // - Traverse the DAG starting from the root nodes
    // - Keep the nodes in a sparsely sorted queue (with more slots than actual nodes, and some empty slots)
    // - Any time a new 'edge' is traversed, moves the 'target' node of the edge to the last position of the queue
    // - If number of iterations exceeds N^2, there was a loop in the graph -> abort
    // This is supposed to work fast, because the queue is never re-allocated or re-sorted, only grows incrementally, and
    // we only need to run a second time and remove the 'empty slots' to get the final order.
    std::optional<NodeVector> DirectedAcyclicGraph::getTopologicallySortedNodes() const
    {
        const size_t totalNodeCount = m_nodeOutgoingEdges.size();

        // This remembers temporarily the position of node N in 'nodeQueue' (see below)
        // This index can change in different loops of the code below
        std::unordered_map<const Node*, size_t> nodeIndexIntoQueue;
        nodeIndexIntoQueue.reserve(totalNodeCount);

        // This is a queue of nodes which is:
        // - partially sorted (at any given time during the loops, the first X entries are sorted, while the rest is not sorted yet)
        // - sparse (some entries can be nullptr) - of nodes which were moved during the algorithm, see below
        // - sorted at the end of the loop (by their topological rank)
        // - starts with the root nodes (they are always at the beginning)
        NodeVector sparseNodeQueue = collectRootNodes();

        // Cycle condition - can't find root nodes among a non-empty set of nodes
        if (sparseNodeQueue.empty() && !m_nodeOutgoingEdges.empty())
        {
            return std::nullopt;
        }

        // sparseNodeQueue grows here all the time
        // TODO Violin rework algorithm to not grow and loop over the same container
        for (size_t i = 0; i < sparseNodeQueue.size(); ++i)
        {
            if (i > totalNodeCount * totalNodeCount)
            {
                // TODO Violin this is a primitive loop detection. Replace with a proper graph-based solution!
                // The inner and the outer loop are bound by N(number of nodes), so exceeding that is a
                // sufficient condition that there was a loop
                return std::nullopt;
            }

            // Get the next node in the queue and process based on its outgoing edges
            Node* nextNode = sparseNodeQueue[i];
            // sparseNodeQueue has nullptr holes - skip those
            if (nextNode != nullptr)
            {
                const EdgeList& nextNodeEdges = m_nodeOutgoingEdges.find(nextNode)->second;

                // For each edge, put the 'target' node to the end of the queue (this order may be temporarily wrong,
                // because we don't know if those nodes have also edges between them which would affect this order)
                // What happens if it's wrong? see the if() inside the loop
                for (const auto& outgoingEdge : nextNodeEdges)
                {
                    // Put the node at the end of the 'sparseNodeQueue' and remember the index
                    Node* outgoingEdgeTarget = outgoingEdge.target;
                    sparseNodeQueue.emplace_back(outgoingEdgeTarget);
                    const size_t targetNodeIndex = sparseNodeQueue.size() - 1;

                    const auto potentiallyAlreadyProcessedNode = nodeIndexIntoQueue.find(outgoingEdgeTarget);
                    // target node not processed yet?
                    if (potentiallyAlreadyProcessedNode == nodeIndexIntoQueue.end())
                    {
                        // => insert to processed queue, with current index from 'queue'
                        nodeIndexIntoQueue.insert({ outgoingEdgeTarget, targetNodeIndex });
                    }
                    // target node processed already?
                    else
                    {
                        // => move the node from its last computed index to the current one
                        // (and set to nullptr on its last position so that it does not occur twice in the queue)
                        // Why do we do this? Because it makes sure that any time there is a 'new edge' to a node,
                        // it is moved to the last position in the queue, unless it has no 'incoming edges' (root node)
                        // or it has exactly one incoming edge (and never needs to be re-sorted)
                        sparseNodeQueue[potentiallyAlreadyProcessedNode->second] = nullptr;
                        potentiallyAlreadyProcessedNode->second = targetNodeIndex;
                    }
                }
            }
        }

        NodeVector topologicallySortedNodes;
        topologicallySortedNodes.reserve(totalNodeCount);
        for (auto sortedNode : sparseNodeQueue)
        {
            // Some nodes are nullptr because of the special 'bubble sort' sorting method
            if (sortedNode != nullptr)
            {
                topologicallySortedNodes.emplace_back(sortedNode);
            }
        }

        return topologicallySortedNodes;
    }

    bool DirectedAcyclicGraph::addEdge(Node& source, Node& target)
    {
        auto nodeEdges = m_nodeOutgoingEdges.find(&source);
        assert (nodeEdges != m_nodeOutgoingEdges.end());

        auto outgoingEdgeIter = std::find_if(nodeEdges->second.begin(), nodeEdges->second.end(), [&target](const Edge& edge) {
            return &target == edge.target;
        });

        // Did not find outgoing edge to target node? Create one, with weight 0 (will be increased to one in next step)
        bool isNewEdge = false;
        if (outgoingEdgeIter == nodeEdges->second.end())
        {
            nodeEdges->second.push_back({ &target, 0 });
            outgoingEdgeIter = std::prev(nodeEdges->second.end());
            isNewEdge = true;
        }

        // Increase weight (we have one more link between these two nodes)
        outgoingEdgeIter->multiplicity++;
        return isNewEdge;
    }

    void DirectedAcyclicGraph::removeEdge(Node& source, const Node& target)
    {
        const auto srcNodeEdges = m_nodeOutgoingEdges.find(&source);
        assert (srcNodeEdges != m_nodeOutgoingEdges.end());
        const auto outgoingEdge = std::find_if(srcNodeEdges->second.begin(), srcNodeEdges->second.end(), [&target](const Edge& edge) {
            return &target == edge.target;
        });

        assert (outgoingEdge != srcNodeEdges->second.end());
        assert(outgoingEdge->multiplicity > 0);
        outgoingEdge->multiplicity--;
        if (outgoingEdge->multiplicity == 0)
        {
            srcNodeEdges->second.erase(outgoingEdge);
        }
    }

    size_t DirectedAcyclicGraph::getInDegree(Node& node) const
    {
        // "reduces" (sums up in arbitrary order) outgoing edges of node
        // TODO Violin This can be done much simpler with a standard adjacency matrix, if we redesign the class a bit more
        return std::accumulate(m_nodeOutgoingEdges.begin(), m_nodeOutgoingEdges.end(), size_t(0u),
            [&node](size_t sum, const std::pair<Node*, EdgeList>& edgeList)
            {
                auto potentialLinkToNode = std::find_if(edgeList.second.begin(), edgeList.second.end(), [&node](const Edge& edge){return edge.target == &node;});
                if (potentialLinkToNode != edgeList.second.end())
                {
                    sum += potentialLinkToNode->multiplicity;
                }

                return sum;
            });
    }

    size_t DirectedAcyclicGraph::getOutDegree(Node& node) const
    {
        const auto nodeOutgoingEdges = m_nodeOutgoingEdges.find(&node);
        assert(nodeOutgoingEdges != m_nodeOutgoingEdges.end());
        // sums up outgoing edge count to other nodes
        return std::accumulate(nodeOutgoingEdges->second.begin(), nodeOutgoingEdges->second.end(), size_t(0u),
            [](size_t sum, const Edge& edge)
            {
                return sum + edge.multiplicity;
            });
    }

    NodeVector DirectedAcyclicGraph::collectRootNodes() const
    {
        // Outgoing edges has size == number of nodes in the graph
        const size_t nodeCount = m_nodeOutgoingEdges.size();

        NodeVector rootNodes;
        rootNodes.reserve(nodeCount);

        std::unordered_set<Node*> nodesWithIncomingEdges;
        nodesWithIncomingEdges.reserve(nodeCount);

        for (auto& outgoingEdgesOfNode : m_nodeOutgoingEdges)
        {
            for (auto& outgoineEdge : outgoingEdgesOfNode.second)
            {
                nodesWithIncomingEdges.insert(outgoineEdge.target);
            }
        }

        for (auto& nodeEntry : m_nodeOutgoingEdges)
        {
            Node* node = nodeEntry.first;
            if (nodesWithIncomingEdges.end() == nodesWithIncomingEdges.find(node))
            {
                rootNodes.emplace_back(node);
            }
        }

        return rootNodes;
    }

    bool DirectedAcyclicGraph::containsNode(Node& node) const
    {
        return m_nodeOutgoingEdges.find(&node) != m_nodeOutgoingEdges.end();
    }

}
