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
     * priority of the log messages (lower means more important, thus higher priority).
     */
    enum class ELogMessageType : int
    {
        Off = 0,
        Fatal = 1,
        Error = 2,
        Warn = 3,
        Info = 4,
        Debug = 5,
        Trace = 6
    };
}
