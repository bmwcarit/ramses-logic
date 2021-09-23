//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <string>

namespace rlogic
{
    class LogicObject;

    /**
     * Holds information about an error which occured during #rlogic::LogicEngine API calls
     */
    struct ErrorData
    {
        /**
         * Error description as human-readable text. For Lua errors, an extra stack
         * trace is contained in the error string with new-line separators.
        */
        std::string message;

        /**
         * The #rlogic::LogicObject which caused the error. Can be nullptr if the error was not originating from a specific object.
         */
        const LogicObject* object;
    };
}
