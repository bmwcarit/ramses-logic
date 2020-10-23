//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

namespace rlogic::internal
{
    // Used to carry over array type information from the ARRAY() function
    struct ArrayTypeInfo
    {
        size_t arraySize = 0u;
        sol::object arrayType = sol::nil;
    };
}
