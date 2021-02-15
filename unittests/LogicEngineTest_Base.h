//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"

namespace rlogic
{
    class ALogicEngine : public ::testing::Test
    {
    protected:
        LogicEngine m_logicEngine;

        const std::string_view m_valid_empty_script = R"(
            function interface()
            end
            function run()
            end
        )";

        const std::string_view m_invalid_empty_script = R"(
        )";
    };
}
