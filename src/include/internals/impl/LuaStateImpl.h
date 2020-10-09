//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

#include <vector>
#include <memory>
#include <string>
#include <string_view>

namespace rlogic::internal
{
    class LuaStateImpl
    {
    public:
        LuaStateImpl();

        // Move-able (noexcept); Not copy-able
        ~LuaStateImpl() noexcept = default;
        LuaStateImpl(LuaStateImpl&& other) noexcept = default;
        LuaStateImpl& operator=(LuaStateImpl&& other) noexcept = default;
        LuaStateImpl(const LuaStateImpl& other) = delete;
        LuaStateImpl& operator=(const LuaStateImpl& other) = delete;

        sol::load_result loadScript(std::string_view source, std::string_view scriptName);
        std::optional<sol::environment> createEnvironment(const sol::protected_function& rootScript);

        template <typename T> sol::object createUserObject(const T& instance);

    private:
        std::unique_ptr<sol::state>     m_sol;

    };

    template <typename T> inline sol::object LuaStateImpl::createUserObject(const T& instance)
    {
        return sol::object(*m_sol, sol::in_place_type<T>, instance);
    }
}
