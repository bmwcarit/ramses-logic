//  -------------------------------------------------------------------------
//  Copyright (C) 2022 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"

namespace rlogic::internal
{
    enum class EEnvProtectionFlag
    {
        None,
        LoadScript,
        InitFunction,
        InterfaceFunction,
        RunFunction
    };

    class EnvironmentProtection
    {
    public:
        static void AddProtectedEnvironmentTable(sol::environment& env, sol::state& state);
        static void SetEnvironmentProtectionLevel(sol::environment& env, EEnvProtectionFlag protectionFlag);

        // Used by logic engine internals to bypass environment protection
        [[nodiscard]] static sol::table GetProtectedEnvironmentTable(const sol::environment& environmentTable) noexcept;

    private:
        static void protectedNewIndex_LoadScript(const sol::lua_table& tbl, const sol::object& key, const sol::object& value);
        [[nodiscard]] static sol::object protectedIndex_LoadScript(const sol::lua_table& tbl, const sol::object& key);

        static void protectedNewIndex_InitializeFunction(const sol::lua_table& tbl, const sol::object& key, const sol::object& value);
        [[nodiscard]] static sol::object protectedIndex_InitializeFunction(const sol::lua_table& tbl, const sol::object& key);

        static void protectedNewIndex_InterfaceFunction(const sol::lua_table& tbl, const sol::object& key, const sol::object& value);
        [[nodiscard]] static sol::object protectedIndex_InterfaceFunction(const sol::lua_table& tbl, const sol::object& key);

        static void protectedNewIndex_RunFunction(const sol::lua_table& tbl, const sol::object& key, const sol::object& value);
        [[nodiscard]] static sol::object protectedIndex_RunFunction(const sol::lua_table& tbl, const sol::object& key);

        static void EnsureStringKey(const sol::object& key);
    };

    class ScopedEnvironmentProtection
    {
    public:
        ScopedEnvironmentProtection(sol::environment& env, EEnvProtectionFlag protectionFlag);
        ~ScopedEnvironmentProtection();

        ScopedEnvironmentProtection(const ScopedEnvironmentProtection&) = delete;
        ScopedEnvironmentProtection& operator=(const ScopedEnvironmentProtection&) = delete;

    private:
        sol::environment& m_env;
    };
}
