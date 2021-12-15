//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LuaScriptImpl.h"

#include "ramses-logic/LuaModule.h"
#include "internals/SolState.h"
#include "impl/PropertyImpl.h"
#include "impl/LuaModuleImpl.h"

#include "internals/WrappedLuaProperty.h"
#include "internals/SolHelper.h"
#include "internals/ErrorReporting.h"
#include "internals/FileFormatVersions.h"

#include "generated/LuaScriptGen.h"

#include <iostream>

namespace rlogic::internal
{
    LuaScriptImpl::LuaScriptImpl(LuaCompiledScript compiledScript, std::string_view name, uint64_t id)
        : LogicNodeImpl(name, id)
        , m_source(std::move(compiledScript.source.sourceCode))
        , m_wrappedRootInput(*compiledScript.rootInput->m_impl)
        , m_wrappedRootOutput(*compiledScript.rootOutput->m_impl)
        , m_solFunction(std::move(compiledScript.mainFunction))
        , m_modules(std::move(compiledScript.source.userModules))
        , m_stdModules(std::move(compiledScript.source.stdModules))
    {
        setRootProperties(std::move(compiledScript.rootInput), std::move(compiledScript.rootOutput));

        sol::environment env = sol::get_environment(m_solFunction);

        env["IN"] = std::ref(m_wrappedRootInput);
        env["OUT"] = std::ref(m_wrappedRootOutput);
    }

    flatbuffers::Offset<rlogic_serialization::LuaScript> LuaScriptImpl::Serialize(const LuaScriptImpl& luaScript, flatbuffers::FlatBufferBuilder& builder, SerializationMap& serializationMap)
    {
        // TODO Violin investigate options to save byte code, instead of plain text, e.g.:
        //sol::bytecode scriptCode = m_solFunction.dump();

        std::vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> userModules;
        userModules.reserve(luaScript.m_modules.size());
        for (const auto& module : luaScript.m_modules)
        {
            userModules.push_back(
                rlogic_serialization::CreateLuaModuleUsage(builder,
                    builder.CreateString(module.first),
                    serializationMap.resolveLuaModuleOffset(*module.second)));
        }

        std::vector<uint8_t> stdModules;
        stdModules.reserve(luaScript.m_stdModules.size());
        for (const EStandardModule stdModule : luaScript.m_stdModules)
        {
            stdModules.push_back(static_cast<uint8_t>(stdModule));
        }

        auto script = rlogic_serialization::CreateLuaScript(builder,
            builder.CreateString(luaScript.getName()),
            luaScript.getId(),
            builder.CreateString(luaScript.m_source),
            builder.CreateVector(userModules),
            builder.CreateVector(stdModules),
            PropertyImpl::Serialize(*luaScript.getInputs()->m_impl, builder, serializationMap),
            PropertyImpl::Serialize(*luaScript.getOutputs()->m_impl, builder, serializationMap)
        );
        builder.Finish(script);

        return script;
    }

    std::unique_ptr<LuaScriptImpl> LuaScriptImpl::Deserialize(
        SolState& solState,
        const rlogic_serialization::LuaScript& luaScript,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (luaScript.id() == 0u)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing id!", nullptr);
            return nullptr;
        }

        // TODO Violin make optional - no need to always serialize string if not used
        if (!luaScript.name())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing name!", nullptr);
            return nullptr;
        }

        const std::string_view name = luaScript.name()->string_view();

        if (!luaScript.luaSourceCode())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing Lua source code!", nullptr);
            return nullptr;
        }
        std::string sourceCode = luaScript.luaSourceCode()->str();

        if (!luaScript.rootInput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root input!", nullptr);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootInput = PropertyImpl::Deserialize(*luaScript.rootInput(), EPropertySemantics::ScriptInput, errorReporting, deserializationMap);
        if (!rootInput)
        {
            return nullptr;
        }

        if (!luaScript.rootOutput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root output!", nullptr);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootOutput = PropertyImpl::Deserialize(*luaScript.rootOutput(), EPropertySemantics::ScriptOutput, errorReporting, deserializationMap);
        if (!rootOutput)
        {
            return nullptr;
        }

        if (rootInput->getName() != "IN" || rootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root input has unexpected name or type!", nullptr);
            return nullptr;
        }

        if (rootOutput->getName() != "OUT" || rootOutput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root output has unexpected name or type!", nullptr);
            return nullptr;
        }

        // TODO Violin we use 'name' here, and not 'chunkname' as in Create(). This is inconsistent! Investigate closer
        sol::load_result load_result = solState.loadScript(sourceCode, name);
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed parsing Lua source code:\n{}", name, error.what()), nullptr);
            return nullptr;
        }

        if (!luaScript.userModules())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing user module dependencies!", nullptr);
            return nullptr;
        }
        ModuleMapping userModules;
        userModules.reserve(luaScript.userModules()->size());
        for (const auto* module : *luaScript.userModules())
        {
            if (!module->name() || !module->module_())
            {
                errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' module data: missing name or module!", name), nullptr);
                return nullptr;
            }
            const LuaModule& moduleUsed = deserializationMap.resolveLuaModule(*module->module_());
            userModules.emplace(module->name()->str(), &moduleUsed);
        }

        if (!luaScript.standardModules())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing standard module dependencies!", nullptr);
            return nullptr;
        }
        StandardModules stdModules;
        stdModules.reserve(luaScript.standardModules()->size());
        for (const uint8_t stdModule : *luaScript.standardModules())
            stdModules.push_back(static_cast<EStandardModule>(stdModule));

        sol::protected_function mainFunction = load_result;
        sol::environment env = solState.createEnvironment(stdModules, userModules);

        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed executing script:\n{}!", name, error.what()), nullptr);
            return nullptr;
        }

        env["GLOBAL"] = solState.createTable();
        sol::protected_function init = env["init"];

        if (init.valid())
        {
            sol::protected_function_result initResult = init();

            if (!initResult.valid())
            {
                sol::error error = initResult;
                errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed initializing script:\n{}!", name, error.what()), nullptr);
                return nullptr;
            }
        }

        return std::make_unique<LuaScriptImpl>(
            LuaCompiledScript{
                LuaSource{
                    std::move(sourceCode),
                    solState,
                    std::move(stdModules),
                    std::move(userModules)
                },
                sol::protected_function(std::move(load_result)),
                std::make_unique<Property>(std::move(rootInput)),
                std::make_unique<Property>(std::move(rootOutput))
            },
            name, luaScript.id());
    }

    std::optional<LogicNodeRuntimeError> LuaScriptImpl::update()
    {
        sol::environment        env  = sol::get_environment(m_solFunction);
        sol::protected_function runFunction = env["run"];
        sol::protected_function_result result = runFunction();

        if (!result.valid())
        {
            sol::error error = result;
            return LogicNodeRuntimeError{error.what()};
        }

        return std::nullopt;
    }

    const ModuleMapping& LuaScriptImpl::getModules() const
    {
        return m_modules;
    }
}
