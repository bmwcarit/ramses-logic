//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LogicNodeConnector.h"
#include "impl/PropertyImpl.h"
#include "impl/LogicNodeImpl.h"

#include "ramses-logic/RamsesBinding.h"
#include "ramses-logic/Property.h"

namespace rlogic::internal
{

    bool LogicNodeConnector::link(const PropertyImpl& sourceProperty, const PropertyImpl& targetProperty)
    {
        if (m_links.end() != m_links.find(&targetProperty))
        {
            return false;
        }
        m_links.insert({&targetProperty, &sourceProperty});

        return true;
    }

    bool LogicNodeConnector::unlink( const PropertyImpl & targetProperty)
    {
        return m_links.erase(&targetProperty) != 0;
    }

    void LogicNodeConnector::unlinkAll(const LogicNodeImpl& logicNode)
    {
        auto inputs = logicNode.getInputs();
        if (nullptr != inputs)
        {
            const auto inputCount = inputs->getChildCount();
            for (size_t i = 0; i < inputCount; ++i)
            {
                auto input = inputs->getChild(i);
                unlink(*input->m_impl);
            }
        }

        const auto outputs = logicNode.getOutputs();
        if (nullptr != outputs)
        {
            const auto outputCount = outputs->getChildCount();
            for (size_t i = 0; i < outputCount; ++i)
            {
                const auto output = outputs->getChild(i);

                // Remove all links which uses this output as input
                for (auto iter = m_links.begin(); iter != m_links.end();)
                {
                    if (iter->second == output->m_impl.get())
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
    }

    const PropertyImpl* LogicNodeConnector::getLinkedOutput(const PropertyImpl & input) const
    {
        const auto iter = m_links.find(&input);
        if (iter != m_links.end())
        {
            return iter->second;
        }
        return nullptr;
    }

    const PropertyImpl* LogicNodeConnector::getLinkedInput(const PropertyImpl& output) const
    {
        auto iter = std::find_if(m_links.begin(), m_links.end(), [&output](const std::unordered_map<const PropertyImpl*, const PropertyImpl*>::value_type& it){
            return it.second == &output;
        });
        if (iter != m_links.end())
        {
            return iter->first;
        }
        return nullptr;
    }

    bool LogicNodeConnector::isLinked(const LogicNodeImpl & logicNode) const
    {
        auto       inputs     = logicNode.getInputs();
        if (isSourceLinked(*inputs->m_impl))
        {
            return true;
        }

        const auto outputs     = logicNode.getOutputs();
        if (nullptr != outputs)
        {
            return isTargetLinked(*outputs->m_impl);
        }
        return false;
    }

    bool LogicNodeConnector::isSourceLinked(PropertyImpl& input) const
    {
        const auto inputCount = input.getChildCount();
        // check if an input of this node is a target of another node
        for (size_t i = 0; i < inputCount; ++i)
        {
            const auto child = input.getChild(i);
            if (child->getType() == EPropertyType::Struct)
            {
                if(isSourceLinked(*child->m_impl))
                {
                    return true;
                }
            }
            else
            {
                if (m_links.end() != m_links.find(child->m_impl.get()))
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool LogicNodeConnector::isTargetLinked(PropertyImpl& output) const
    {
        const auto outputCount = output.getChildCount();
        for (size_t i = 0; i < outputCount; ++i)
        {
            const auto child = output.getChild(i);

            if (child->getType() == EPropertyType::Struct)
            {
                if (isTargetLinked(*child->m_impl))
                {
                    return true;
                }
            }
            else
            {
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
