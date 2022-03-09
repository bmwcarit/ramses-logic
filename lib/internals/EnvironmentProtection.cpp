//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/EnvironmentProtection.h"

#include "ramses-logic/EPropertyType.h"

#include "internals/SolHelper.h"

namespace rlogic::internal
{
    sol::table EnvironmentProtection::GetProtectedEnvironmentTable(const sol::environment& environmentTable) noexcept
    {
        sol::object protectedTable = environmentTable[sol::metatable_key]["__sensitive"];
        assert(protectedTable != sol::nil && "No protection added!");
        return protectedTable;
    }

    void EnvironmentProtection::AddProtectedEnvironmentTable(sol::environment& env, sol::state& state)
    {
        assert(env[sol::metatable_key] == sol::nil && "Already has protection!");
        sol::table sensitiveTable(state, sol::create);
        sol::table metatable = state.create_table();
        metatable["__sensitive"] = sensitiveTable;
        env[sol::metatable_key] = metatable;
    }

    void EnvironmentProtection::SetEnvironmentProtectionLevel(sol::environment& env, EEnvProtectionFlag protectionFlag)
    {
        sol::table protectedMetatable = env.traverse_raw_get<sol::table>("_G", sol::metatable_key);

        switch (protectionFlag)
        {
        case EEnvProtectionFlag::None:
            protectedMetatable[sol::meta_function::new_index] = sol::nil;
            protectedMetatable[sol::meta_function::index] = sol::nil;
            break;
        case EEnvProtectionFlag::LoadScript:
            protectedMetatable[sol::meta_function::new_index] = EnvironmentProtection::protectedNewIndex_LoadScript;
            protectedMetatable[sol::meta_function::index] = EnvironmentProtection::protectedIndex_LoadScript;
            break;
        case EEnvProtectionFlag::InitFunction:
            protectedMetatable[sol::meta_function::new_index] = EnvironmentProtection::protectedNewIndex_InitializeFunction;
            protectedMetatable[sol::meta_function::index] = EnvironmentProtection::protectedIndex_InitializeFunction;
            break;
        case EEnvProtectionFlag::InterfaceFunction:
            protectedMetatable[sol::meta_function::new_index] = EnvironmentProtection::protectedNewIndex_InterfaceFunction;
            protectedMetatable[sol::meta_function::index] = EnvironmentProtection::protectedIndex_InterfaceFunction;
            break;
        case EEnvProtectionFlag::RunFunction:
            protectedMetatable[sol::meta_function::new_index] = EnvironmentProtection::protectedNewIndex_RunFunction;
            protectedMetatable[sol::meta_function::index] = EnvironmentProtection::protectedIndex_RunFunction;
            break;
        }
    }


    void EnvironmentProtection::EnsureStringKey(const sol::object& key)
    {
        const sol::type keyType = key.get_type();
        if (keyType != sol::type::string)
        {
            sol_helper::throwSolException("Assigning global variables with a non-string index is prohibited! (key type used '{}')", sol_helper::GetSolTypeName(keyType));
        }
    }

    void EnvironmentProtection::protectedNewIndex_LoadScript(const sol::lua_table& tbl, const sol::object& key, const sol::object& value)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());
        const sol::type valueType = value.get_type();

        if (valueType != sol::type::function)
        {
            sol_helper::throwSolException("Declaring global variables is forbidden (exceptions: the functions 'init', 'interface' and 'run')! (found value of type '{}')",
                sol_helper::GetSolTypeName(valueType));
        }

        if (keyStr != "init" && keyStr != "interface" && keyStr != "run")
        {
            sol_helper::throwSolException("Unexpected function name '{}'! Allowed names: 'init', 'interface', 'run'", keyStr);
        }

        if (GetProtectedEnvironmentTable(tbl).raw_get<sol::object>(key) != sol::nil)
        {
            sol_helper::throwSolException("Function '{}' can only be declared once!", keyStr);
        }

        GetProtectedEnvironmentTable(tbl).raw_set(key, value);
    };

    sol::object EnvironmentProtection::protectedIndex_LoadScript(const sol::lua_table& tbl, const sol::object& key)
    {
        EnsureStringKey(key);

        const std::string keyStr(key.as<std::string>());
        if (keyStr != "modules")
        {
            sol_helper::throwSolException(
                "Trying to read global variable '{}' outside the scope of init(), interface() and run() functions! This can cause undefined behavior and is forbidden!", keyStr);
        }

        return GetProtectedEnvironmentTable(tbl).raw_get<sol::object>(key);
    };

    void EnvironmentProtection::protectedNewIndex_InitializeFunction(const sol::lua_table& /*tbl*/, const sol::object& key, const sol::object& /*value*/)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());

        if (keyStr == "GLOBAL")
        {
            sol_helper::throwSolException("Trying to override the GLOBAL table in init()! You can only add data, but not overwrite the table!");
        }
        else
        {
            sol_helper::throwSolException("Unexpected global variable definition '{}' in init()! Please use the GLOBAL table to declare global data and functions, or use modules!", keyStr);
        }
    }

    sol::object EnvironmentProtection::protectedIndex_InitializeFunction(const sol::lua_table& tbl, const sol::object& key)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());
        if (keyStr != "GLOBAL")
        {
            sol_helper::throwSolException(
                "Trying to read global variable '{}' in the init() function! This can cause undefined behavior and is forbidden! Use the GLOBAL table to read/write global data!", keyStr);
        }
        return GetProtectedEnvironmentTable(tbl).raw_get<sol::object>(key);
    }

    void EnvironmentProtection::protectedNewIndex_InterfaceFunction(const sol::lua_table& /*tbl*/, const sol::object& key, const sol::object& /*value*/)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());
        if (keyStr == "GLOBAL")
        {
            sol_helper::throwSolException("Trying to override the GLOBAL table in interface()! You can only read data, but not overwrite the GLOBAL table!");
        }
        else
        {
            sol_helper::throwSolException(
                "Unexpected global variable definition '{}' in interface()! Use the GLOBAL table inside the init() function to declare global data and functions, or use modules!", keyStr);
        }
    }

    sol::object EnvironmentProtection::protectedIndex_InterfaceFunction(const sol::lua_table& tbl, const sol::object& key)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());
        if (keyStr != "GLOBAL" && keyStr != "IN" && keyStr != "OUT")
        {
            sol_helper::throwSolException("Unexpected global access to key '{}' in interface()! Allowed keys: 'GLOBAL', 'IN', 'OUT'", keyStr);
        }
        return GetProtectedEnvironmentTable(tbl).raw_get<sol::object>(key);
    }

    void EnvironmentProtection::protectedNewIndex_RunFunction(const sol::lua_table& /*tbl*/, const sol::object& key, const sol::object& /*value*/)
    {
        EnsureStringKey(key);
        const std::string keyStr(key.as<std::string>());
        if (keyStr == "GLOBAL")
        {
            sol_helper::throwSolException("Trying to override the GLOBAL table in run()! You can only read data, but not overwrite the table!");
        }
        else
        {
            sol_helper::throwSolException("Unexpected global variable definition '{}' in run()! Use the init() function to declare global data and functions, or use modules!", keyStr);
        }
    }

    sol::object EnvironmentProtection::protectedIndex_RunFunction(const sol::lua_table& tbl, const sol::object& key)
    {
        EnsureStringKey(key);
        const std::string_view keyStr(key.as<std::string_view>());
        if (keyStr != "GLOBAL" && keyStr != "IN" && keyStr != "OUT")
        {
            sol_helper::throwSolException("Unexpected global access to key '{}' in run()! Allowed keys: 'GLOBAL', 'IN', 'OUT'", keyStr);
        }
        return GetProtectedEnvironmentTable(tbl).raw_get<sol::object>(key);
    }

    ScopedEnvironmentProtection::ScopedEnvironmentProtection(sol::environment& env, EEnvProtectionFlag protectionFlag)
        : m_env(env)
    {
        EnvironmentProtection::SetEnvironmentProtectionLevel(m_env, protectionFlag);
    }

    ScopedEnvironmentProtection::~ScopedEnvironmentProtection()
    {
        EnvironmentProtection::SetEnvironmentProtectionLevel(m_env, EEnvProtectionFlag::None);
    }
}
