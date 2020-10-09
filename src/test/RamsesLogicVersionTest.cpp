//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-logic/RamsesLogicVersion.h"

namespace rlogic
{
    TEST(GetRamsesLogicVersion, ReturnsCorrectVersion)
    {
        auto version = GetRamsesLogicVersion();
        EXPECT_EQ("0.2.0", version.string);
        EXPECT_EQ(version.major, 0u);
        EXPECT_EQ(version.minor, 2u);
        EXPECT_EQ(version.patch, 0u);
    }
}
