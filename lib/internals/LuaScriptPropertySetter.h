//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

namespace rlogic
{
    class Property;
}

namespace rlogic::internal
{
    class LuaScriptPropertyHandler;

    class LuaScriptPropertySetter
    {
    public:
        static void Set(Property& property, const sol::object& value);

    private:
        static void SetNumber(Property& property, const sol::object& number);
        static void SetTable(Property& property, const sol::table& table);
        static void SetString(Property& property, std::string_view string);
        static void SetBool(Property& property, bool boolean);
        static void SetStruct(Property& property, LuaScriptPropertyHandler& structPropertyHandler);
        static void SetArray(Property& property, LuaScriptPropertyHandler& structPropertyHandler);
    };
}
