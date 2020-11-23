//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LuaScriptHandler.h"
#include "internals/SolState.h"
#include "internals/SolHelper.h"

#include <cmath>

namespace rlogic::internal
{
    LuaScriptHandler::LuaScriptHandler(SolState& state)
        : m_state(state)
    {
    }

    std::string_view LuaScriptHandler::GetIndexAsString(const sol::object& index)
    {
        if (!index.valid() || index.get_type() != sol::type::string)
        {
            sol_helper::throwSolException("Only strings supported as table key type!");
        }
        return index.as<std::string_view>();
    }

    size_t LuaScriptHandler::GetMaxIndexForVectorType(rlogic::EPropertyType type)
    {
        switch (type)
        {
        case EPropertyType::Vec2i:
        case EPropertyType::Vec2f:
            return 2u;
        case EPropertyType::Vec3i:
        case EPropertyType::Vec3f:
            return 3u;
        case EPropertyType::Vec4f:
        case EPropertyType::Vec4i:
            return 4u;
        // TODO Violin/Sven/Tobias this kind of a bad design, and the reason for it lies
        // with the fact that we handle 3 different things in the same base class - "Property"
        // Discuss whether we want this pattern, or maybe there are some other ideas how to deal
        // with type abstraction and polymorphy where we would not have this problem
        case EPropertyType::Struct:
        case EPropertyType::Array:
        case EPropertyType::Float:
        case EPropertyType::Int32:
        case EPropertyType::String:
        case EPropertyType::Bool:
            assert(false && "Should not have reached this code!");
        }

        assert(false && "Vector type not handled!");
        return 0;
    }

    template <> std::optional<float> LuaScriptHandler::ExtractSpecificType<float>(const sol::object& solObject)
    {
        if (!solObject.valid() || solObject.get_type() != sol::type::number)
        {
            return std::nullopt;
        }

        return std::make_optional(solObject.as<float>());
    }

    template <>
    std::optional<int32_t> LuaScriptHandler::ExtractSpecificType<int32_t>(const sol::object& solObject)
    {
        if (!solObject.valid() || solObject.get_type() != sol::type::number)
        {
            return std::nullopt;
        }

        const float floatVal = solObject.as<float>();
        // Check that float is round-able to an integer, with type-specific tolerance
        if (std::fmod(floatVal, 1) > std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;
        }

        return std::make_optional(static_cast<int32_t>(floatVal));
    }

    template <>
    std::optional<size_t> LuaScriptHandler::ExtractSpecificType<size_t>(const sol::object& solObject)
    {
        if (!solObject.valid() || solObject.get_type() != sol::type::number)
        {
            return std::nullopt;
        }

        const float floatVal = solObject.as<float>();

        // Check that float value is not negative, with some tolerance
        if (floatVal < -std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;
        }

        // Check that float is round-able to an integer, with some tolerance
        if (std::fmod(floatVal, 1) > std::numeric_limits<float>::epsilon())
        {
            return std::nullopt;
        }

        return std::make_optional(static_cast<size_t>(floatVal));
    }


    template <typename T, size_t size> std::array<T, size> LuaScriptHandler::ExtractArray(const sol::table& solTable)
    {
        // table.size() does return 0, but iterating over the table does work
        // therefore we have to count ourselves.
        // TODO check if there is a better way or find out what's the issue with table.size()
        size_t tableFieldCount = 0u;
        solTable.for_each([&](const std::pair<sol::object, sol::object>& /*key_value*/) { ++tableFieldCount; });

        if (tableFieldCount != size)
        {
            sol_helper::throwSolException("Expected {} array components in table but got {} instead!", size, tableFieldCount);
        }

        std::array<T, size> data{};
        for (size_t i = 1; i <= size; ++i)
        {
            const sol::object& tableEntry = solTable[i];

            std::optional<T> maybeValue = ExtractSpecificType<T>(tableEntry);

            if (!maybeValue)
            {
                sol_helper::throwSolException("Unexpected type {} at array element # {}!", sol_helper::GetSolTypeName(tableEntry.get_type()), i);
            }

            data[i-1] = *maybeValue;
        }

        return data;
    }

    SolState& LuaScriptHandler::getState()
    {
        return m_state;
    }

    // Explicitly instantiate types we use
    template std::array<int32_t, 2> LuaScriptHandler::ExtractArray<int32_t, 2>(const sol::table& solTable);
    template std::array<int32_t, 3> LuaScriptHandler::ExtractArray<int32_t, 3>(const sol::table& solTable);
    template std::array<int32_t, 4> LuaScriptHandler::ExtractArray<int32_t, 4>(const sol::table& solTable);
    template std::array<float, 2>   LuaScriptHandler::ExtractArray<float, 2>(const sol::table& solTable);
    template std::array<float, 3>   LuaScriptHandler::ExtractArray<float, 3>(const sol::table& solTable);
    template std::array<float, 4>   LuaScriptHandler::ExtractArray<float, 4>(const sol::table& solTable);
}
