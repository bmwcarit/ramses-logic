//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LogicNodeDependencies.h"

#include "ramses-logic/Property.h"

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "internals/ErrorReporting.h"
#include "internals/TypeUtils.h"

#include <cassert>
#include "fmt/format.h"

namespace rlogic::internal
{

    void LogicNodeDependencies::addNode(LogicNodeImpl& node)
    {
        m_logicNodeDAG.addNode(node);
        m_nodeTopologyChanged = true;
    }

    void LogicNodeDependencies::removeNode(LogicNodeImpl& node)
    {
        m_logicNodeConnector.unlinkAll(node);
        m_logicNodeDAG.removeNode(node);

        // Remove the node from the cache without reordering the rest (unless there is no cache yet)
        // Removing nodes does not require topology update (we don't guarantee specific ordering when
        // nodes are not related, we only guarantee relative ordering when nodes are linked)
        if (m_cachedTopologicallySortedNodes)
        {
            NodeVector& cachedNodes = *m_cachedTopologicallySortedNodes;
            cachedNodes.erase(std::remove(cachedNodes.begin(), cachedNodes.end(), &node), cachedNodes.end());
        }
    }

    bool LogicNodeDependencies::isLinked(const LogicNodeImpl& node) const
    {
        return m_logicNodeConnector.isLinked(node);
    }

    std::optional<NodeVector> LogicNodeDependencies::getTopologicallySortedNodes()
    {
        if (m_nodeTopologyChanged)
        {
            m_cachedTopologicallySortedNodes = m_logicNodeDAG.getTopologicallySortedNodes();
            m_nodeTopologyChanged = false;
        }

        return m_cachedTopologicallySortedNodes;
    }

    const PropertyImpl* LogicNodeDependencies::getLinkedOutput(PropertyImpl& inputProperty) const
    {
        return m_logicNodeConnector.getLinkedOutput(inputProperty);
    }

    const LinksMap& LogicNodeDependencies::getLinks() const
    {
        return m_logicNodeConnector.getLinks();
    }

    bool LogicNodeDependencies::link(PropertyImpl& output, PropertyImpl& input, ErrorReporting& errorReporting)
    {
        if (!m_logicNodeDAG.containsNode(output.getLogicNode()))
        {
            errorReporting.add(fmt::format("LogicNode '{}' is not an instance of this LogicEngine", output.getLogicNode().getName()), nullptr);
            return false;
        }

        if (!m_logicNodeDAG.containsNode(input.getLogicNode()))
        {
            errorReporting.add(fmt::format("LogicNode '{}' is not an instance of this LogicEngine", input.getLogicNode().getName()), nullptr);
            return false;
        }

        if (&output.getLogicNode() == &input.getLogicNode())
        {
            errorReporting.add("SourceNode and TargetNode are equal", nullptr);
            return false;
        }

        if (!(output.isOutput() && input.isInput()))
        {
            std::string_view lhsType = output.isOutput() ? "output" : "input";
            std::string_view rhsType = input.isOutput() ? "output" : "input";
            errorReporting.add(fmt::format("Failed to link {} property '{}' to {} property '{}'. Only outputs can be linked to inputs", lhsType, output.getName(), rhsType, input.getName()), nullptr);
            return false;
        }

        if (output.getType() != input.getType())
        {
            errorReporting.add(fmt::format("Types of source property '{}:{}' does not match target property '{}:{}'",
                output.getName(),
                GetLuaPrimitiveTypeName(output.getType()),
                input.getName(),
                GetLuaPrimitiveTypeName(input.getType())), nullptr);
            return false;
        }

        // No need to also test input type, above check already makes sure output and input are of the same type
        if (!TypeUtils::IsPrimitiveType(output.getType()))
        {
            errorReporting.add(fmt::format("Can't link properties of complex types directly, currently only primitive properties can be linked"), nullptr);
            return false;
        }

        auto& targetNode = input.getLogicNode();
        auto& node = output.getLogicNode();

        if (!m_logicNodeConnector.link(output, input))
        {
            errorReporting.add(fmt::format("The property '{}' of LogicNode '{}' is already linked to the property '{}' of LogicNode '{}'",
                output.getName(),
                node.getName(),
                input.getName(),
                targetNode.getName()
            ), nullptr);
            return false;
        }
        input.setIsLinkedInput(true);

        // TODO Violin below code sets two different things to dirty. Try to not have redundant dirty
        // flags and consolidate dirtiness to one place
        const bool isNewEdge = m_logicNodeDAG.addEdge(node, targetNode);
        if (isNewEdge)
        {
            m_nodeTopologyChanged = true;
        }
        targetNode.setDirty(true);

        return true;
    }

    bool LogicNodeDependencies::unlink(PropertyImpl& output, PropertyImpl& input, ErrorReporting& errorReporting)
    {
        if (TypeUtils::CanHaveChildren(input.getType()))
        {
            errorReporting.add(fmt::format("Can't unlink properties of complex types directly!"), nullptr);
            return false;
        }

        if (!m_logicNodeConnector.unlinkPrimitiveInput(input))
        {
            errorReporting.add(fmt::format("No link available from source property '{}' to target property '{}'", output.getName(), input.getName()), nullptr);
            return false;
        }

        auto& node = output.getLogicNode();
        auto& targetNode = input.getLogicNode();
        input.setIsLinkedInput(false);

        m_logicNodeDAG.removeEdge(node, targetNode);

        return true;
    }
}
