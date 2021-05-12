//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{

    LogicNodeImpl::LogicNodeImpl(std::string_view name) noexcept
        : m_name(name)
    {
    }

    flatbuffers::Offset<rlogic_serialization::LogicNode> LogicNodeImpl::Serialize(const LogicNodeImpl& logicNode, flatbuffers::FlatBufferBuilder& builder)
    {
        // Inputs can never be null, only outputs
        assert(logicNode.m_inputs);

        return rlogic_serialization::CreateLogicNode(
            builder,
            builder.CreateString(logicNode.m_name),
            PropertyImpl::Serialize(*logicNode.m_inputs->m_impl, builder),
            nullptr != logicNode.m_outputs ? PropertyImpl::Serialize(*logicNode.m_outputs->m_impl, builder) : 0
        );
    }

    // This is the deserialization code of LogicNodeImpl (properties were already loaded in sub-classes)
    LogicNodeImpl::LogicNodeImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs)
        : m_name(name)
    {
        // We don't have LogicNodes currently without any inputs
        assert(inputs);

        m_inputs = std::make_unique<Property>(std::move(inputs));
        m_inputs->m_impl->setLogicNode(*this);

        if (outputs)
        {
            m_outputs = std::make_unique<Property>(std::move(outputs));
            m_outputs->m_impl->setLogicNode(*this);
        }
    }

    LogicNodeImpl::~LogicNodeImpl() noexcept = default;

    Property* LogicNodeImpl::getInputs()
    {
        return m_inputs.get();
    }

    const Property* LogicNodeImpl::getInputs() const
    {
        return m_inputs.get();
    }

    const Property* LogicNodeImpl::getOutputs() const
    {
        return m_outputs.get();
    }

    Property* LogicNodeImpl::getOutputs()
    {
        return m_outputs.get();
    }

    std::string_view LogicNodeImpl::getName() const
    {
        return m_name;
    }

    void LogicNodeImpl::setName(std::string_view name)
    {
        m_name = name;
    }

    void LogicNodeImpl::setDirty(bool dirty)
    {
        m_dirty = dirty;
    }

    bool LogicNodeImpl::isDirty() const
    {
        return m_dirty;
    }
}
