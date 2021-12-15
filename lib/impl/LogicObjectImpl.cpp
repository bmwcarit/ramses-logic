//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicObjectImpl.h"

namespace rlogic::internal
{
    LogicObjectImpl::LogicObjectImpl(std::string_view name, uint64_t id) noexcept
        : m_name(name)
        , m_id(id)
    {
    }

    LogicObjectImpl::~LogicObjectImpl() noexcept = default;

    std::string_view LogicObjectImpl::getName() const
    {
        return m_name;
    }

    void LogicObjectImpl::setName(std::string_view name)
    {
        m_name = name;
    }

    uint64_t LogicObjectImpl::getId() const
    {
        return m_id;
    }
}
