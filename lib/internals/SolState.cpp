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
        // No 'fallback' symbol table, the environment has no access to the
        // global symbols of the default environment
        sol::environment newEnv(m_solState, sol::create);

        // Set itself as a global variable registry
        newEnv["_G"] = newEnv;

        const std::vector<std::string> safeBaseSymbols = {
            "assert",
            "error",
            "ipairs",
            "next",
            "pairs",
            "print",
            "select",
            "tonumber",
            "tostring",
            "type",
            "unpack",
            "pcall",
            "xpcall",
            "_VERSION",

            // Potentially less safe, but allows for advanced Lua use cases
            "rawequal",
            "rawget",
            "rawset",
            "setmetatable",
            "getmetatable",
        };

        for (const auto& name : safeBaseSymbols)
        {
            newEnv[name] = m_solState[name];
        }

        // TODO Violin when we implement modules, this list should be taken from the explicit lists of base + custom modules
        const std::vector<std::string> baseLibs = { "string", "math", "table", "debug" };

        for (const auto& name : baseLibs)
        {
            const sol::table& moduleAsTable = m_solState[name];
            sol::table copy(m_solState, sol::create);
            for (const auto& pair : moduleAsTable)
            {
                // first is the name of a function in module, second is the function
                copy[pair.first] = pair.second;
            }
            newEnv[name] = std::move(copy);
        }

        return newEnv;
    }

    sol::environment& SolState::getInterfaceExtractionEnvironment()
    {
        return m_interfaceExtractionEnvironment;
    }
}
