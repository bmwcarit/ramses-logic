//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

namespace rlogic::internal
{
    class LuaScriptPropertyHandler;
    class PropertyImpl;

    class LuaScriptPropertySetter
    {
    public:
        static void Set(PropertyImpl& property, const sol::object& value);

    private:
        static void SetNumber(PropertyImpl& property, const sol::object& number);
        static void SetTable(PropertyImpl& property, const sol::table& table);
        static void SetString(PropertyImpl& property, std::string_view string);
        static void SetBool(PropertyImpl& property, bool boolean);
        static void SetStruct(PropertyImpl& property, LuaScriptPropertyHandler& structPropertyHandler);
        static void SetArray(PropertyImpl& property, LuaScriptPropertyHandler& structPropertyHandler);
    };
}
