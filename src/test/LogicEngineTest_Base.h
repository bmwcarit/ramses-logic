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

        LuaScript* findLuaScriptByName(std::string_view name)
        {
            auto maybeScript = std::find_if(m_logicEngine.scripts().begin(), m_logicEngine.scripts().end(),
                [name](const LuaScript* script)
                {
                    return script->getName() == name;
                }
            );
            if (maybeScript == m_logicEngine.scripts().end())
            {
                return nullptr;
            }
            return *maybeScript;
        }

        RamsesNodeBinding* findRamsesNodeBindingByName(std::string_view name)
        {
            auto maybeBinding = std::find_if(m_logicEngine.ramsesNodeBindings().begin(), m_logicEngine.ramsesNodeBindings().end(),
                [name](const RamsesNodeBinding* binding)
                {
                    return binding->getName() == name;
                }
            );
            if (maybeBinding == m_logicEngine.ramsesNodeBindings().end())
            {
                return nullptr;
            }
            return *maybeBinding;
        }

        RamsesAppearanceBinding* findRamsesAppearanceBindingByName(std::string_view name)
        {
            auto maybeBinding = std::find_if(m_logicEngine.ramsesAppearanceBindings().begin(), m_logicEngine.ramsesAppearanceBindings().end(),
                [name](const RamsesAppearanceBinding* binding)
                {
                    return binding->getName() == name;
                }
            );
            if (maybeBinding == m_logicEngine.ramsesAppearanceBindings().end())
            {
                return nullptr;
            }
            return *maybeBinding;
        }
    };
}
