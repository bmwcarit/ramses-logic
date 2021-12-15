//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LuaConfigImpl.h"
#include "internals/SolWrapper.h"

#include <string>
#include <memory>
#include <optional>

namespace rlogic
{
    class Property;
    class LuaModule;
}

namespace rlogic::internal
{
    class SolState;
    class ErrorReporting;

    struct LuaSource
    {
        // Metadata
        std::string sourceCode;
        std::reference_wrapper<SolState> solState;
        StandardModules stdModules;
        ModuleMapping userModules;
    };

    struct LuaCompiledScript
    {
        LuaSource source;

        // The main function (holding interface() and run() functions)
        sol::protected_function mainFunction;

        // Parsed interface properties
        std::unique_ptr<Property> rootInput;
        std::unique_ptr<Property> rootOutput;
    };

    struct LuaCompiledModule
    {
        LuaSource source;
        sol::table moduleTable;
    };

    class LuaCompilationUtils
    {
    public:
        [[nodiscard]] static std::optional<LuaCompiledScript> CompileScript(
            SolState& solState,
            const ModuleMapping& userModules,
            const StandardModules& stdModules,
            std::string source,
            std::string_view name,
            ErrorReporting& errorReporting);

        [[nodiscard]] static std::optional<LuaCompiledModule> CompileModule(
            SolState& solState,
            const ModuleMapping& userModules,
            const StandardModules& stdModules,
            std::string source,
            std::string_view name,
            ErrorReporting& errorReporting);

        [[nodiscard]] static bool CheckModuleName(std::string_view name);

        [[nodiscard]] static std::optional<std::vector<std::string>> ExtractModuleDependencies(
            std::string_view source,
            ErrorReporting& errorReporting);

        [[nodiscard]] static sol::table MakeTableReadOnly(SolState& solState, sol::table table);

    private:
        [[nodiscard]] static std::string BuildChunkName(std::string_view scriptName);
        [[nodiscard]] static bool CrossCheckDeclaredAndProvidedModules(
            std::string_view source,
            const ModuleMapping& modules,
            std::string_view chunkname,
            ErrorReporting& errorReporting);
    };
}
