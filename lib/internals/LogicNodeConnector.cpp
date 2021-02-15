//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LogicNodeConnector.h"
#include "internals/TypeUtils.h"

#include "impl/PropertyImpl.h"
#include "impl/LogicNodeImpl.h"

#include "ramses-logic/RamsesBinding.h"
#include "ramses-logic/Property.h"

#include <cassert>

namespace rlogic::internal
{

    bool LogicNodeConnector::link(const PropertyImpl& output, const PropertyImpl& input)
    {
        assert(TypeUtils::IsPrimitiveType(output.getType()));
        assert(TypeUtils::IsPrimitiveType(input.getType()));

        if (m_links.end() != m_links.find(&input))
        {
            return false;
        }
        m_links.insert({&input, &output});

        return true;
    }

    bool LogicNodeConnector::unlinkPrimitiveInput(const PropertyImpl& input)
    {
        assert(TypeUtils::IsPrimitiveType(input.getType()));
        return m_links.erase(&input) != 0;
    }

    void LogicNodeConnector::unlinkInputRecursive(const PropertyImpl& input)
    {
        if (TypeUtils::CanHaveChildren(input.getType()))
        {
            for (size_t i = 0; i < input.getChildCount(); ++i)
            {
                unlinkInputRecursive(*input.getChild(i)->m_impl);
            }
        }
        else
        {
            assert(TypeUtils::IsPrimitiveType(input.getType()));
            unlinkPrimitiveInput(input);
        }
    }

    void LogicNodeConnector::unlinkOutputRecursive(const PropertyImpl& output)
    {
        if (TypeUtils::CanHaveChildren(output.getType()))
        {
            for (size_t i = 0; i < output.getChildCount(); ++i)
            {
                unlinkOutputRecursive(*output.getChild(i)->m_impl);
            }
        }
        else
        {
            assert(TypeUtils::IsPrimitiveType(output.getType()));
            // Remove all links which uses this primitive output as source for their corresponding input value
            for (auto iter = m_links.begin(); iter != m_links.end();)
            {
                if (iter->second == &output)
                {
                    iter = m_links.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }

    // TODO Violin this function has likely high asymptotic cost for large graphs
    // (iterates all m_links to search for both inputs and outputs -> scales liearly with nr of nodes
    // and linearly with nr of links (we don't have this case yet in real projects, but with larger
    // projects we will)
    // Create a benchmark and profile to verify or disprove suspicion, and if true, optimize it
    // (it is only used when node destroyed, so maybe there is a faster way to remove all links)
    void LogicNodeConnector::unlinkAll(const LogicNodeImpl& logicNode)
    {
        const PropertyImpl* inputs = logicNode.getInputs()->m_impl.get();
        assert (nullptr != inputs);
        unlinkInputRecursive(*inputs);

        const Property* outputs = logicNode.getOutputs();
        if (nullptr != outputs)
        {
            unlinkOutputRecursive(*outputs->m_impl);
        }
    }

    const PropertyImpl* LogicNodeConnector::getLinkedOutput(const PropertyImpl& input) const
    {
        const auto iter = m_links.find(&input);
        if (iter != m_links.end())
        {
            return iter->second;
        }
        return nullptr;
    }

    bool LogicNodeConnector::isLinked(const LogicNodeImpl& logicNode) const
    {
        auto inputs = logicNode.getInputs();
        if (isInputLinked(*inputs->m_impl))
        {
            return true;
        }

        const auto outputs = logicNode.getOutputs();
        if (nullptr != outputs)
        {
            return isOutputLinked(*outputs->m_impl);
        }
        return false;
    }

    bool LogicNodeConnector::isInputLinked(PropertyImpl& input) const
    {
        const auto inputCount = input.getChildCount();
        // check if an input of this node is a target of another node
        for (size_t i = 0; i < inputCount; ++i)
        {
            const auto child = input.getChild(i);
            if (TypeUtils::CanHaveChildren(child->getType()))
            {
                if(isInputLinked(*child->m_impl))
                {
                    return true;
                }
            }
            else
            {
                assert(TypeUtils::IsPrimitiveType(child->getType()));
                if (m_links.end() != m_links.find(child->m_impl.get()))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool LogicNodeConnector::isOutputLinked(PropertyImpl& output) const
    {
        const auto outputCount = output.getChildCount();
        for (size_t i = 0; i < outputCount; ++i)
        {
            const auto child = output.getChild(i);

            if (TypeUtils::CanHaveChildren(child->getType()))
            {
                if (isOutputLinked(*child->m_impl))
                {
                    return true;
                }
            }
            else
            {
                assert(TypeUtils::IsPrimitiveType(child->getType()));
                // check if a output of this node is an input of another node
                if (m_links.end() != std::find_if(m_links.begin(), m_links.end(), [child](const std::unordered_map<const PropertyImpl*, const PropertyImpl*>::value_type& it) {
                        return it.second == child->m_impl.get();
                    }))
                {
                    return true;
                }
            }
        }
        return false;
    }
}
