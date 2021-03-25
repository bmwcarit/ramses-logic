//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicNode.h"
#include "impl/LogicNodeImpl.h"
#include "ramses-logic/Property.h"

namespace rlogic
{
    LogicNode::LogicNode(std::reference_wrapper<internal::LogicNodeImpl> impl) noexcept
        : m_impl(impl)
    {
    }

    LogicNode::~LogicNode() noexcept = default;

    Property* LogicNode::getInputs()
    {
        return m_impl.get().getInputs();
    }

    const Property* LogicNode::getInputs() const
    {
        return m_impl.get().getInputs();
    }

    const Property* LogicNode::getOutputs() const
    {
        return m_impl.get().getOutputs();
    }

    std::string_view LogicNode::getName() const
    {
        return m_impl.get().getName();
    }

    void LogicNode::setName(std::string_view name)
    {
        m_impl.get().setName(name);
    }

}
