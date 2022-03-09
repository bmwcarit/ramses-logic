//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/LuaCompilationUtils.h"

#include "ramses-logic/Property.h"
#include "ramses-logic/LuaModule.h"
#include "impl/PropertyImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/LoggerImpl.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"
#include "internals/PropertyTypeExtractor.h"
#include "internals/EPropertySemantics.h"
#include "internals/EnvironmentProtection.h"
#include "fmt/format.h"
#include "SolHelper.h"

namespace rlogic::internal
{
    std::optional<LuaCompiledScript> LuaCompilationUtils::CompileScript(
        SolState& solState,
        const ModuleMapping& userModules,
        const StandardModules& stdModules,
        std::string source,
        std::string_view name,
        ErrorReporting& errorReporting)
    {
        const std::string chunkname = BuildChunkName(name);

        sol::load_result load_result = solState.loadScript(source, chunkname);
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
            return std::nullopt;
        }

        if (!CrossCheckDeclaredAndProvidedModules(source, userModules, chunkname, errorReporting))
            return std::nullopt;

        sol::environment env = solState.createEnvironment(stdModules, userModules);
        sol::table internalEnv = EnvironmentProtection::GetProtectedEnvironmentTable(env);

        internalEnv["GLOBAL"] = solState.createTable();

        sol::protected_function mainFunction = load_result;
        env.set_on(mainFunction);

        sol::protected_function_result main_result {};
        {
            ScopedEnvironmentProtection p(env, EEnvProtectionFlag::LoadScript);
            main_result = mainFunction();
        }

        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(error.what(), nullptr);
            return std::nullopt;
        }

        if (main_result.get_type() != sol::type::none)
        {
            errorReporting.add(fmt::format("[{}] Expected no return value in script source, but a value of type '{}' was returned!",
                chunkname, sol_helper::GetSolTypeName(main_result.get_type())), nullptr);
            return std::nullopt;
        }

        sol::protected_function intf = internalEnv["interface"];
        if (!intf.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'interface' function defined!", chunkname), nullptr);
            return std::nullopt;
        }

        sol::protected_function init = internalEnv["init"];
        if (init.valid())
        {
            // in order to support interface definitions in globals we need to register the symbols for init section
            sol::protected_function_result initResult {};
            {
                PropertyTypeExtractor::RegisterTypes(env);
                ScopedEnvironmentProtection p(env, EEnvProtectionFlag::InitFunction);
                initResult = init();
                PropertyTypeExtractor::UnregisterTypes(env);
            }

            if (!initResult.valid())
            {
                sol::error error = initResult;
                errorReporting.add(fmt::format("[{}] Error while initializing script. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
                return std::nullopt;
            }
        }

        sol::protected_function run = internalEnv["run"];

        if (!run.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'run' function defined!", chunkname), nullptr);
            return std::nullopt;
        }

        PropertyTypeExtractor inputsExtractor("IN", EPropertyType::Struct);
        PropertyTypeExtractor outputsExtractor("OUT", EPropertyType::Struct);

        sol::environment interfaceEnvironment = solState.createEnvironment(stdModules, userModules);
        PropertyTypeExtractor::RegisterTypes(interfaceEnvironment);
        interfaceEnvironment["IN"] = std::ref(inputsExtractor);
        interfaceEnvironment["OUT"] = std::ref(outputsExtractor);
        // Expose globals to interface function
        EnvironmentProtection::GetProtectedEnvironmentTable(interfaceEnvironment)["GLOBAL"] = internalEnv["GLOBAL"];

        interfaceEnvironment.set_on(intf);
        sol::protected_function_result intfResult {};
        {
            ScopedEnvironmentProtection p(interfaceEnvironment, EEnvProtectionFlag::InterfaceFunction);
            intfResult = intf();
        }

        interfaceEnvironment["IN"] = sol::lua_nil;
        interfaceEnvironment["OUT"] = sol::lua_nil;
        for (const auto& module : userModules)
            interfaceEnvironment[module.first] = sol::lua_nil;
        PropertyTypeExtractor::UnregisterTypes(interfaceEnvironment);

        if (!intfResult.valid())
        {
            sol::error error = intfResult;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
            return std::nullopt;
        }

        auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>(inputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptInput));
        auto outputs = std::make_unique<Property>(std::make_unique<PropertyImpl>(outputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptOutput));

        EnvironmentProtection::SetEnvironmentProtectionLevel(env, EEnvProtectionFlag::RunFunction);

        return LuaCompiledScript{
            LuaSource{
                std::move(source),
                solState,
                stdModules,
                userModules
            },
            std::move(run),
            std::move(inputs),
            std::move(outputs)
        };
    }

    std::optional<LuaCompiledModule> LuaCompilationUtils::CompileModule(
        SolState& solState,
        const ModuleMapping& userModules,
        const StandardModules& stdModules,
        std::string source,
        std::string_view name,
        ErrorReporting& errorReporting)
    {
        const std::string chunkname = BuildChunkName(name);

        sol::load_result load_result = solState.loadScript(source, chunkname);
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("[{}] Error while loading module. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
            return std::nullopt;
        }

        if (!CrossCheckDeclaredAndProvidedModules(source, userModules, chunkname, errorReporting))
            return std::nullopt;

        sol::environment env = solState.createEnvironment(stdModules, userModules);
        // interface definitions can be provided within module, in order to be able to extract them
        // when used in LuaScript interface the necessary user types need to be provided
        PropertyTypeExtractor::RegisterTypes(env);

        sol::protected_function mainFunction = load_result;
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(error.what(), nullptr);
            return std::nullopt;
        }

        // user types for interface extraction are no longer needed
        // but type constants are still needed otherwise extraction in script using the module is not correct (not clear why)
        PropertyTypeExtractor::UnregisterTypes(env, true);

        sol::object resultObj = main_result;
        // TODO Violin check and test for abuse: yield, more than one result
        if (!resultObj.is<sol::table>())
        {
            errorReporting.add(fmt::format("[{}] Error while loading module. Module script must return a table!", chunkname), nullptr);
            return std::nullopt;
        }

        sol::table moduleTable = resultObj;

        return LuaCompiledModule{
            LuaSource{
                std::move(source),
                solState,
                stdModules,
                userModules
            },
            LuaCompilationUtils::MakeTableReadOnly(solState, moduleTable)
        };
    }

    // Implements https://www.lua.org/pil/13.4.4.html
    sol::table LuaCompilationUtils::MakeTableReadOnly(SolState& solState, sol::table table)
    {
        auto denyAccess = []() {
            sol_helper::throwSolException("Modifying module data is not allowed!");
        };

        for (auto [childKey, childObject] : table)
        {
            if (childObject.is<sol::table>())
            {
                table[childKey] = LuaCompilationUtils::MakeTableReadOnly(solState, childObject);
            }
        }

        // create metatable which denies write access but allows reading
        sol::table metatable = solState.createTable();
        metatable[sol::meta_function::new_index] = denyAccess;
        metatable[sol::meta_function::index] = table;

        // overwrite assigned module with new table and point its metatable to read-only table
        sol::table readOnlyTable = solState.createTable();
        readOnlyTable[sol::metatable_key] = metatable;

        return readOnlyTable;
    }

    std::string LuaCompilationUtils::BuildChunkName(std::string_view scriptName)
    {
        return std::string(scriptName.empty() ? "unknown" : scriptName);
    }

    bool LuaCompilationUtils::CheckModuleName(std::string_view name)
    {
        if (name.empty())
            return false;

        // Check if any non-alpha-numeric char is contained
        const bool onlyAlphanumChars = std::all_of(name.cbegin(), name.cend(), [](char c) { return c == '_' || (0 != std::isalnum(c)); });
        if (!onlyAlphanumChars)
            return false;

        // First letter is a digit -> error
        if (0 != std::isdigit(name[0]))
            return false;

        return true;
    }

    bool LuaCompilationUtils::CrossCheckDeclaredAndProvidedModules(std::string_view source, const ModuleMapping& modules, std::string_view chunkname, ErrorReporting& errorReporting)
    {
        std::optional<std::vector<std::string>> declaredModules = LuaCompilationUtils::ExtractModuleDependencies(source, errorReporting);
        if (!declaredModules) // failed extraction
            return false;
        if (modules.empty() && declaredModules->empty()) // early out if no modules
            return true;

        std::vector<std::string> providedModules;
        providedModules.reserve(modules.size());
        for (const auto& m : modules)
            providedModules.push_back(m.first);
        std::sort(declaredModules->begin(), declaredModules->end());
        std::sort(providedModules.begin(), providedModules.end());
        if (providedModules != declaredModules)
        {
            std::string errMsg = fmt::format("[{}] Error while loading script/module. Module dependencies declared in source code do not match those provided by LuaConfig.\n", chunkname);
            errMsg += fmt::format("  Module dependencies declared in source code: {}\n", fmt::join(*declaredModules, ", "));
            errMsg += fmt::format("  Module dependencies provided on create API: {}", fmt::join(providedModules, ", "));
            errorReporting.add(errMsg, nullptr);
            return false;
        }

        return true;
    }

    std::optional<std::vector<std::string>> LuaCompilationUtils::ExtractModuleDependencies(std::string_view source, ErrorReporting& errorReporting)
    {
        sol::state tempLuaState;

        std::vector<std::string> extractedModules;
        bool success = true;
        int timesCalled = 0;
        tempLuaState.set_function("modules", [&](sol::variadic_args va) {
            ++timesCalled;
            int argIdx = 0;
            for (const auto& v : va)
            {
                if (v.is<std::string>())
                {
                    extractedModules.push_back(v.as<std::string>());
                }
                else
                {
                    const auto argTypeName = sol::type_name(v.lua_state(), v.get_type());
                    errorReporting.add(
                        fmt::format(R"(Error while extracting module dependencies: argument {} is of type '{}', string must be provided: ex. 'modules("moduleA", "moduleB")')",
                        argIdx, argTypeName), nullptr);
                    success = false;
                }
                ++argIdx;
            }
        });

        const sol::load_result load_result = tempLuaState.load(source, "temp");
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("Error while extracting module dependencies:\n{}", error.what()), nullptr);
            return std::nullopt;
        }

        sol::protected_function scriptFunc = load_result;
        const sol::protected_function_result scriptFuncResult = scriptFunc();
        if (!scriptFuncResult.valid())
        {
            const sol::error error = scriptFuncResult;
            LOG_DEBUG(fmt::format("Lua runtime error while extracting module dependencies, this is ignored for the actual extraction but might affect its result:\n{}", error.what()), nullptr);
        }

        if (!success)
            return std::nullopt;

        if (timesCalled > 1)
        {
            errorReporting.add("Error while extracting module dependencies: 'modules' function was executed more than once", nullptr);
            return std::nullopt;
        }

        auto sortedDependencies = extractedModules;
        std::sort(sortedDependencies.begin(), sortedDependencies.end());
        const auto duplicateIt = std::adjacent_find(sortedDependencies.begin(), sortedDependencies.end());
        if (duplicateIt != sortedDependencies.end())
        {
            errorReporting.add(fmt::format("Error while extracting module dependencies: '{}' appears more than once in dependency list", *duplicateIt), nullptr);
            return std::nullopt;
        }

        return extractedModules;
    }
}
