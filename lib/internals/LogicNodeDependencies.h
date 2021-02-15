//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/DirectedAcyclicGraph.h"
#include "internals/LogicNodeConnector.h"

#include <unordered_set>

namespace rlogic::internal
{
    class LogicNodeImpl;
    class ErrorReporting;

    using NodeSet = std::unordered_set<LogicNodeImpl*>;

    // Tracks the links between logic nodes and orders them based on the topological structure derived
    // from those links.
    class LogicNodeDependencies
    {
    public:
        // The primary purpose of this class
        bool updateTopologicalSorting();
        [[nodiscard]] const NodeVector& getOrderedNodesCache() const;

        // Nodes management
        void addNode(LogicNodeImpl& node);
        void removeNode(LogicNodeImpl& node);

        // Link management
        bool link(PropertyImpl& output, PropertyImpl& input, ErrorReporting& errorReporting);
        bool unlink(PropertyImpl& output, PropertyImpl& input, ErrorReporting& errorReporting);
        [[nodiscard]] bool isLinked(const LogicNodeImpl& node) const;
        [[nodiscard]] const LinksMap& getLinks() const;
        [[nodiscard]] const PropertyImpl* getLinkedOutput(PropertyImpl& inputProperty) const;

        [[nodiscard]] bool isDirty() const;

    private:
        // TODO Violin redesign these classes, they have redundant data
        DirectedAcyclicGraph                m_logicNodeDAG;
        LogicNodeConnector                  m_logicNodeConnector;
    };
}
