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

        [[nodiscard]] size_t size() const;

        [[nodiscard]] sol::object getChildPropertyAsSolObject(std::string_view childName);

        [[nodiscard]] const PropertyImpl& getPropertyImpl() const;

        // TODO Violin remove this exposure
        [[nodiscard]] SolState& getSolState();

    private:
        SolState& m_solState;
        PropertyImpl& m_propertyDescription;

        void        setChildProperty(const sol::object& propertyIndex, const sol::object& rhs);
        sol::object convertPropertyToSolObject(PropertyImpl& property);

        sol::object getChildPropertyAsSolObject(const sol::object& index);

        [[nodiscard]] Property* getStructProperty(const sol::object& propertyIndex);
        [[nodiscard]] Property* getStructProperty(std::string_view propertyName);
        [[nodiscard]] Property* getArrayProperty(const sol::object& propertyIndex);

        // TODO Violin this method is not needed and can be optimized away
        template <typename T> sol::object convertPropertyToSolObject(PropertyImpl& property)
        {
            return m_solState.createUserObject(property.getValueAs<T>());
        }
    };
}
