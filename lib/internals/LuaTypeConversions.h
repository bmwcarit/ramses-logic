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
    class LuaTypeConversions
    {
    public:
        static std::string_view GetIndexAsString (const sol::object& index);

        template <typename T>
        static std::optional<T> ExtractSpecificType(const sol::object& solObject);

        static size_t           GetMaxIndexForVectorType(rlogic::EPropertyType type);

        template <typename T, size_t size>
        static std::array<T, size> ExtractArray(const sol::table& solTable);

        static_assert(std::is_same<LUA_NUMBER, double>::value, "This class assumes that Lua-internal numbers are double precision floats");
    };

}
