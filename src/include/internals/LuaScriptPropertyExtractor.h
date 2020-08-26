//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"
#include "internals/LuaScriptHandler.h"

namespace rlogic::internal
{
    class PropertyImpl;
    class LuaStateImpl;

    class LuaScriptPropertyExtractor : public LuaScriptHandler
    {
    public:
        LuaScriptPropertyExtractor(LuaStateImpl& state, PropertyImpl& propertyDescription);

        static sol::object Index(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& rhs);
        static void        NewIndex(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& rhs);

    private:
        PropertyImpl& m_propertyDescription;

        void        addEntryToPropertyDescription(const sol::object& index, const sol::object& value, PropertyImpl& property);
        sol::object getProperty(const sol::object& index);
    };
}
