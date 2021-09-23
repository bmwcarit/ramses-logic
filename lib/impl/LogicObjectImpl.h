//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <string>

namespace rlogic::internal
{
    class LogicObjectImpl
    {
    public:
        explicit LogicObjectImpl(std::string_view name) noexcept;
        virtual ~LogicObjectImpl() noexcept;
        LogicObjectImpl(const LogicObjectImpl&) = delete;
        LogicObjectImpl& operator=(const LogicObjectImpl&) = delete;

        [[nodiscard]] std::string_view getName() const;
        void setName(std::string_view name);

    private:
        std::string m_name;
    };
}
