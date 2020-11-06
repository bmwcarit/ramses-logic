//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <vector>
#include <string>

namespace rlogic::internal
{
    class ErrorReporting
    {
    public:

        void clear();
        void add(std::string errorMessage);

        const std::vector<std::string>& getErrors() const;

    private:
        std::vector<std::string> m_errors;
    };
}
