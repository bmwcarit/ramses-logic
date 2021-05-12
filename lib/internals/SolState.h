//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

#include <string_view>

namespace rlogic::internal
{
    class SolState
    {
    public:
        SolState();

        // Move-able (noexcept); Not copy-able
        ~SolState() noexcept = default;
        SolState(SolState&& other) noexcept = default;
        SolState& operator=(SolState&& other) noexcept = default;
        SolState(const SolState& other) = delete;
        SolState& operator=(const SolState& other) = delete;

        sol::load_result loadScript(std::string_view source, std::string_view scriptName);
        sol::environment createEnvironment();

        template <typename T> sol::object createUserObject(const T& instance);

    private:
        sol::state m_solState;

    };

    template <typename T> inline sol::object SolState::createUserObject(const T& instance)
    {
        return sol::object(m_solState, sol::in_place_type<T>, instance);
    }
}
