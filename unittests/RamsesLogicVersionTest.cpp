//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-logic/RamsesLogicVersion.h"
#include "ramses-logic-build-config.h"
#include "fmt/format.h"

namespace rlogic
{
    TEST(GetRamsesLogicVersion, ReturnsCorrectVersion)
    {
        auto version = GetRamsesLogicVersion();
        EXPECT_EQ(fmt::format("{}.{}.{}", g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR, g_PROJECT_VERSION_PATCH), version.string);
        EXPECT_EQ(version.major, g_PROJECT_VERSION_MAJOR);
        EXPECT_EQ(version.minor, g_PROJECT_VERSION_MINOR);
        EXPECT_EQ(version.patch, g_PROJECT_VERSION_PATCH);
    }
}
