//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"
#include "ramses-logic/EPropertyType.h"

#include <optional>
#include <array>

namespace rlogic::internal
{
    class LuaStateImpl;

    class LuaScriptHandler
    {
    public:
        explicit LuaScriptHandler(LuaStateImpl& state);

    protected:
        LuaStateImpl& m_state;

        static std::string_view GetIndexAsString (const sol::object& index);

        // TODO Violin/Sven investigate sol macro "SOL_SAFE_NUMERICS " and "SOL_NO_CHECK_NUMBER_PRECISION
        // Could be a more elegant solution than this
        template <typename T>
        static std::optional<T> ExtractSpecificType(const sol::object& solObject);

        static size_t           GetMaxIndexForVectorType(rlogic::EPropertyType type);

        template <typename T, size_t size>
        static std::array<T, size> ExtractArray(const sol::table& solTable);
    };

}
