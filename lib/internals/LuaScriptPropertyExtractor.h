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
    class PropertyImpl;
    class SolState;

    class LuaScriptPropertyExtractor
    {
    public:
        explicit LuaScriptPropertyExtractor(PropertyImpl& propertyDescription);

        // Overloaded Lua metamethods
        sol::object index(sol::this_state state, const sol::object& index);
        void        newIndex(const sol::object& index, const sol::object& rhs);
        static sol::object CreateArray(sol::this_state state, std::optional<size_t> size, std::optional<sol::object> arrayType);

    private:
        PropertyImpl& m_propertyDescription;

        void        addStructProperty(const sol::object& propertyName, const sol::object& propertyValue, PropertyImpl& parentStruct);
    };
}
