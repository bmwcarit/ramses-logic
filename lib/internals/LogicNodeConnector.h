//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <unordered_set>
#include <unordered_map>

#include <optional>

namespace rlogic::internal
{
    class PropertyImpl;
    class LogicNodeImpl;

    using LinksMap = std::unordered_map<const PropertyImpl*, const PropertyImpl*>;

    class LogicNodeConnector
    {
    public:
        bool link(const PropertyImpl& sourceProperty, const PropertyImpl& targetProperty);
        bool unlink(const PropertyImpl& targetProperty);
        void unlinkAll(const LogicNodeImpl& logicNode);
        bool isLinked(const LogicNodeImpl& logicNode) const;
        const PropertyImpl* getLinkedOutput(const PropertyImpl& input) const;
        const PropertyImpl* getLinkedInput(const PropertyImpl& output) const;

        // TODO Violin refactor this (class should not have to expose internal data). Currently still used for serialization
        const LinksMap& getLinks() const
        {
            return m_links;
        }

    private:
        LinksMap m_links;

        bool isSourceLinked(PropertyImpl& input) const;
        bool isTargetLinked(PropertyImpl& output) const;
    };
}
