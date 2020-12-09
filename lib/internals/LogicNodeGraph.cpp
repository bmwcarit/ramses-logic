//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LogicNodeGraph.h"

#include "impl/LogicNodeImpl.h"


#include <algorithm>
#include <unordered_set>
#include <stack>

namespace rlogic::internal
{
    void LogicNodeGraph::updateOrder()
    {
        if (!m_dirty)
        {
            return;
        }

        std::unordered_map<const LogicNodeImpl*, size_t> processed;
        processed.reserve(m_links.size());

        LogicNodeVector processVector = findUnboundInputs();

        for (size_t i = 0; i < processVector.size(); ++i)
        {
            auto& element = processVector[i];
            const auto pairIter = m_links.find(element);

            if (pairIter != m_links.end())
            {
                for (auto& binding : pairIter->second)
                {
                    processVector.emplace_back(binding.target);
                    const auto processIndex = processVector.size() - 1;

                    const auto processIter = processed.find(binding.target);
                    if (processIter == processed.end())
                    {
                        processed.insert({ binding.target, processIndex });
                    }
                    else
                    {
                        processVector[processIter->second] = nullptr;
                        processIter->second = processIndex;
                    }
                }
            }
        }

        m_order.clear();
        for (auto element : processVector)
        {
            if (element != nullptr)
            {
                m_order.push_back(element);
            }
        }

        m_dirty = false;
    }

    void LogicNodeGraph::addLink(LogicNodeImpl& source, LogicNodeImpl& target)
    {
        auto iter = m_links.find(&source);
        if (iter == m_links.end())
        {
            iter = m_links.insert({ &source, std::vector<Link>() }).first;
        }

        auto linkIter = std::find_if(iter->second.begin(), iter->second.end(), [&target](const Link& binding) {
            return &target == binding.target;
        });

        if (linkIter == iter->second.end())
        {
            iter->second.push_back({ &target, 0 });
            linkIter = std::prev(iter->second.end());
        }

        linkIter->bindingCount++;
        m_dirty = true;
    }

    const LogicNodeVector& LogicNodeGraph::getOrderedNodesCache() const
    {
        // TODO Violin merge update() and get() in one mutable method to avoid the assert
        // Currently keeping as-is because sorting used to be an expensive operation and can't affort to call too often
        assert(!m_dirty);
        return m_order;
    }

    void LogicNodeGraph::removeLinksForNode(LogicNodeImpl& source)
    {
        std::vector<std::pair<LogicNodeImpl*, LogicNodeImpl*>> linksToRemove;

        // Find all links where the node for removal is a source
        {
            const auto iter = m_links.find(&source);
            if (iter != m_links.end())
            {
                for (auto link : iter->second)
                {
                    linksToRemove.emplace_back(std::make_pair(&source, link.target));
                }
            }
        }

        // Find all links where the node for removal is a target
        {
            for (auto& links : m_links)
            {
                for (auto& link : links.second)
                {
                    if (link.target == &source)
                    {
                        linksToRemove.emplace_back(std::make_pair(links.first, &source));
                    }
                }
            }
        }

        // Remove all corresponding links
        for (auto link : linksToRemove)
        {
            removeLink(*link.first, *link.second);
        }
    }

    bool LogicNodeGraph::isLinked(LogicNodeImpl& node) const
    {
        const auto iter = m_links.find(&node);
        if (iter != m_links.end())
        {
            return true;
        }

        for (auto& links : m_links)
        {
            for (auto& link : links.second)
            {
                if (link.target == &node)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void LogicNodeGraph::removeLink(LogicNodeImpl& source, const LogicNodeImpl& target)
    {
        const auto iter = m_links.find(&source);
        if (iter != m_links.end())
        {
            const auto bindingIter = std::find_if(iter->second.begin(), iter->second.end(), [&target](const Link& binding) {
                return &target == binding.target;
            });
            if (bindingIter != iter->second.end())
            {
                bindingIter->bindingCount--;
                if (bindingIter->bindingCount == 0)
                {
                    iter->second.erase(bindingIter);

                    if (iter->second.empty())
                    {
                        m_links.erase(iter);
                    }
                    m_dirty = true;
                }
            }
        }
    }

    LogicNodeVector LogicNodeGraph::findUnboundInputs() const
    {
        LogicNodeVector processVector;
        processVector.reserve(m_links.size());

        std::unordered_set<LogicNodeImpl*> isUnbound;
        isUnbound.reserve(m_links.size());
        for (auto& wire : m_links)
        {
            for (auto& binding : wire.second)
            {
                isUnbound.insert(binding.target);
            }
        }

        for (auto& link : m_links)
        {
            if (isUnbound.end() == isUnbound.find(link.first))
            {
                processVector.emplace_back(link.first);
            }
        }

        return processVector;
    }
}
