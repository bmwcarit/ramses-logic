//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

#include "impl/PropertyImpl.h"
#include "internals/SolState.h"

namespace rlogic
{
    class Property;
}

namespace rlogic::internal
{
    // This class provides a Lua-like interface to the logic engine types
    class LuaScriptPropertyHandler
    {
    public:
        LuaScriptPropertyHandler(SolState& state, PropertyImpl& propertyDescription);
        ~LuaScriptPropertyHandler() = default;

        static void        NewIndex(LuaScriptPropertyHandler& data, const sol::object& index, const sol::object& rhs);
        static sol::object Index(LuaScriptPropertyHandler& data, const sol::object& index);

        size_t size() const;

        sol::object getChildPropertyAsSolObject(std::string_view childName);

        [[nodiscard]] const PropertyImpl& getPropertyImpl() const;

        // TODO Violin remove this exposure
        SolState& getSolState();

    private:
        SolState& m_solState;
        PropertyImpl& m_propertyDescription;

        void        setChildProperty(const sol::object& propertyIndex, const sol::object& rhs);
        sol::object convertPropertyToSolObject(PropertyImpl& property);

        sol::object getChildPropertyAsSolObject(const sol::object& index);

        Property* getStructProperty(const sol::object& propertyIndex);
        Property* getStructProperty(std::string_view propertyName);
        Property* getArrayProperty(const sol::object& propertyIndex);

        template <typename T> sol::object convertPropertyToSolObject(PropertyImpl& property)
        {
            auto value = property.get<T>();
            assert (value && "Lua type mismatch while converting object!");
            return m_solState.createUserObject(*value);
        }
    };
}
