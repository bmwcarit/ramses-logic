//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicObject.h"
#include "impl/LogicObjectImpl.h"

namespace rlogic
{
    LogicObject::LogicObject(std::unique_ptr<internal::LogicObjectImpl> impl) noexcept
        : m_impl{ std::move(impl) }
    {
    }

    LogicObject::~LogicObject() noexcept = default;

    std::string_view LogicObject::getName() const
    {
        return m_impl->getName();
    }

    void LogicObject::setName(std::string_view name)
    {
        m_impl->setName(name);
    }
}
