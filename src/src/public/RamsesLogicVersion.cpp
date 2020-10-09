//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/RamsesLogicVersion.h"
#include "ramses-logic-build-config.h"

namespace rlogic
{
    RamsesLogicVersion GetRamsesLogicVersion()
    {
        return RamsesLogicVersion
        {
            g_PROJECT_VERSION,
            g_PROJECT_VERSION_MAJOR,
            g_PROJECT_VERSION_MINOR,
            g_PROJECT_VERSION_PATCH,
        };
    }
}
