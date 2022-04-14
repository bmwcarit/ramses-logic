//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------
#pragma once

#include <string>

namespace rlogic
{
    /**
     * @brief Result class as a return object for functions
     */
    class Result
    {
    public:
        Result() = default;

        explicit Result(std::string msg)
            : m_message(std::move(msg))
        {
        }

        [[nodiscard]] bool ok() const
        {
            return m_message.empty();
        }

        [[nodiscard]] const std::string& getMessage() const
        {
            return m_message;
        }

    private:
        std::string m_message;
    };
}
