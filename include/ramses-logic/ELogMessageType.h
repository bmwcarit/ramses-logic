//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include <cassert>

namespace rlogic
{
    /**
     * ELogMessageType lists the types of available log messages. The integer represents the
     * priority of the log messages (lower means more important and higher priority).
     */
    enum class ELogMessageType : int
    {
        ERROR = 2,
        WARNING = 3,
        INFO = 4,
        DEBUG = 5,
    };
}
