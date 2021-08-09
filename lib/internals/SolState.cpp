//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/SolState.h"

#include "internals/PropertyTypeExtractor.h"
#include "internals/WrappedLuaProperty.h"


#include <iostream>

namespace rlogic::internal
{
    // NOLINTNEXTLINE(performance-unnecessary-value-param) The signature is forced by SOL. Therefore we have to disable this warning.
    static int solExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description)
    {
        if (maybe_exception)
        {
            return sol::stack::push(L, description);
        }
        return sol::stack::top(L);
    }

    SolState::SolState()
    {
        m_solState.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::debug);
        m_solState.set_exception_handler(&solExceptionHandler);

        m_interfaceExtractionEnvironment = sol::environment(m_solState, sol::create, m_solState.globals());
        PropertyTypeExtractor::RegisterTypesToEnvironment(m_interfaceExtractionEnvironment);

        // TODO Violin only register wrappers to runtime environments, not in the global environment
        WrappedLuaProperty::RegisterTypes(m_solState);
    }

    sol::load_result SolState::loadScript(std::string_view source, std::string_view scriptName)
    {
        return m_solState.load(source, std::string(scriptName));
    }

    sol::environment SolState::createEnvironment()
    {
        return sol::environment(m_solState, sol::create, m_solState.globals());
    }

    sol::environment& SolState::getInterfaceExtractionEnvironment()
    {
        return m_interfaceExtractionEnvironment;
    }

}
