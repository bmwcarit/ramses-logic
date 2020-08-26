//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/LogicNodeImpl.h"
#include "internals/impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{

    LogicNodeImpl::LogicNodeImpl(std::string_view name) noexcept
        : m_name(name)
    {
    }

    LogicNodeImpl::LogicNodeImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs)
        : m_name(name)
    {
        setInputs(std::move(inputs));
        setOutputs(std::move(outputs));
    }

    LogicNodeImpl::~LogicNodeImpl() noexcept = default;

    void LogicNodeImpl::setInputs(std::unique_ptr<PropertyImpl> inputs)
    {
        m_inputs = std::make_unique<Property>(std::move(inputs));
    }

    void LogicNodeImpl::setOutputs(std::unique_ptr<PropertyImpl> outputs)
    {
        m_outputs = std::make_unique<Property>(std::move(outputs));
    }

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

    void LogicNodeImpl::addError(std::string_view error)
    {
        m_errors.emplace_back(error);
    }

    const std::vector<std::string>& LogicNodeImpl::getErrors() const
    {
        return m_errors;
    }

    void LogicNodeImpl::clearErrors()
    {
        m_errors.clear();
    }

    flatbuffers::Offset<serialization::LogicNode> LogicNodeImpl::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        return serialization::CreateLogicNode(
            builder,
            builder.CreateString(m_name),
            nullptr != m_inputs ? m_inputs->m_impl->serialize(builder) : 0,
            nullptr != m_outputs ? m_outputs->m_impl->serialize(builder) : 0
        );
    }

    std::string_view LogicNodeImpl::getName() const
    {
        return m_name;
    }
}
