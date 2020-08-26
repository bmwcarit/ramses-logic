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
     * ELogMessageType lists the types of available log messages
     */
    enum class ELogMessageType : int
    {
        INFO,
        ERROR,
        WARNING,
        DEBUG
    };
}
