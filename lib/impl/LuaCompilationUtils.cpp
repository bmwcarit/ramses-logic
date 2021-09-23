//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LuaCompilationUtils.h"

#include "ramses-logic/Property.h"
#include "impl/PropertyImpl.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"
#include "internals/PropertyTypeExtractor.h"
#include "internals/EPropertySemantics.h"
#include "fmt/format.h"

namespace rlogic::internal
{
    std::optional<LuaCompiledScript> LuaCompilationUtils::Compile(SolState& solState, std::string source, std::string_view scriptName, std::string filename, ErrorReporting& errorReporting)
    {
        const std::string chunkname = BuildChunkName(scriptName, filename);
        sol::load_result load_result = solState.loadScript(source, chunkname);

        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
            return std::nullopt;
        }

        sol::protected_function mainFunction = load_result;

        sol::environment env = solState.createEnvironment();
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(error.what(), nullptr);
            return std::nullopt;
        }

        sol::protected_function intf = env["interface"];

        if (!intf.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'interface' function defined!", chunkname), nullptr);
            return std::nullopt;
        }

        sol::protected_function run = env["run"];

        if (!run.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'run' function defined!", chunkname), nullptr);
            return std::nullopt;
        }

        PropertyTypeExtractor inputsExtractor("IN", EPropertyType::Struct);
        PropertyTypeExtractor outputsExtractor("OUT", EPropertyType::Struct);

        sol::environment& interfaceEnvironment = solState.getInterfaceExtractionEnvironment();

        interfaceEnvironment["IN"] = std::ref(inputsExtractor);
        interfaceEnvironment["OUT"] = std::ref(outputsExtractor);

        interfaceEnvironment.set_on(intf);
        sol::protected_function_result intfResult = intf();

        interfaceEnvironment["IN"] = sol::lua_nil;
        interfaceEnvironment["OUT"] = sol::lua_nil;

        if (!intfResult.valid())
        {
            sol::error error = intfResult;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()), nullptr);
            return std::nullopt;
        }

        return LuaCompiledScript{
            std::move(source),
            std::move(filename),
            solState,
            std::move(load_result),
            std::make_unique<Property>(std::make_unique<PropertyImpl>(inputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptInput)),
            std::make_unique<Property>(std::make_unique<PropertyImpl>(outputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptOutput))
        };
    }

    std::string LuaCompilationUtils::BuildChunkName(std::string_view scriptName, std::string_view fileName)
    {
        std::string chunkname;
        if (scriptName.empty())
        {
            chunkname = fileName.empty() ? "unknown" : fileName;
        }
        else
        {
            if (fileName.empty())
            {
                chunkname = scriptName;
            }
            else
            {
                chunkname = fileName;
                chunkname.append(":");
                chunkname.append(scriptName);
            }
        }
        return chunkname;
    }
}
