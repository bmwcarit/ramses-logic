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
    LuaScriptImpl::LuaScriptImpl(LuaCompiledScript compiledScript, std::string_view name)
        : LogicNodeImpl(name)
        , m_source(std::move(compiledScript.source.sourceCode))
        , m_luaPrintFunction(&LuaScriptImpl::DefaultLuaPrintFunction)
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
        // override the lua print function to handle it by ourselves
        env.set_function("print", &LuaScriptImpl::luaPrint, this);
    }

    flatbuffers::Offset<rlogic_serialization::LuaScript> LuaScriptImpl::Serialize(const LuaScriptImpl& luaScript, flatbuffers::FlatBufferBuilder& builder, SerializationMap& serializationMap)
    {
        // TODO Violin investigate options to save byte code, instead of plain text, e.g.:
        //sol::bytecode scriptCode = m_solFunction.dump();

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>>> modulesOffset = 0;
        if (!luaScript.m_modules.empty())
        {
            std::vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> modulesFB;
            modulesFB.reserve(luaScript.m_modules.size());
            for (const auto& module : luaScript.m_modules)
            {
                modulesFB.push_back(
                    rlogic_serialization::CreateLuaModuleUsage(builder,
                        builder.CreateString(module.first),
                        serializationMap.resolveLuaModuleOffset(*module.second)));
            }

            modulesOffset = builder.CreateVector(modulesFB);
        }

        std::vector<uint8_t> stdModules;
        stdModules.reserve(luaScript.m_stdModules.size());
        for (const EStandardModule stdModule : luaScript.m_stdModules)
        {
            stdModules.push_back(static_cast<uint8_t>(stdModule));
        }

        auto script = rlogic_serialization::CreateLuaScript(builder,
            builder.CreateString(luaScript.getName()),
            0,
            builder.CreateString(luaScript.m_source),
            PropertyImpl::Serialize(*luaScript.getInputs()->m_impl, builder, serializationMap),
            PropertyImpl::Serialize(*luaScript.getOutputs()->m_impl, builder, serializationMap),
            modulesOffset,
            builder.CreateVector(stdModules)
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
        // TODO Violin make optional - no need to always serialize string if not used
        if (!luaScript.name())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing name!", nullptr);
            return nullptr;
        }

        static_assert(g_FileFormatVersion == 2, "Remove filename from schema - search for 'VersionBreak' in schema files");

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

        StandardModules stdModules;
        if (luaScript.standardModules())
        {
            stdModules.reserve(luaScript.standardModules()->size());
            for (const uint8_t stdModule : *luaScript.standardModules())
            {
                stdModules.push_back(static_cast<EStandardModule>(stdModule));
            }
        }
        else
        {
            static_assert(g_FileFormatVersion == 2, "Remove this with next file version break; also consider making stdModules required");
            stdModules = { EStandardModule::Base, EStandardModule::String, EStandardModule::Table, EStandardModule::Math, EStandardModule::Debug };
        }

        static_assert(g_FileFormatVersion == 2, "Consider making modules mandatory with next file version break");
        ModuleMapping userModules;
        if (luaScript.dependencies())
        {
            userModules.reserve(luaScript.dependencies()->size());
            for (const auto* module : *luaScript.dependencies())
            {
                if (!module->name() || !module->module_())
                {
                    errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' module data: missing name or module!", name), nullptr);
                    return nullptr;
                }
                const LuaModule& moduleUsed = deserializationMap.resolveLuaModule(*module->module_());
                userModules.emplace(module->name()->str(), &moduleUsed);
            }
        }

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
            name);
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

    void LuaScriptImpl::DefaultLuaPrintFunction(std::string_view scriptName, std::string_view message)
    {
        std::cout << scriptName << ": " << message << std::endl;
    }

    void LuaScriptImpl::luaPrint(sol::variadic_args args)
    {
        for (uint32_t i = 0; i < args.size(); ++i)
        {
            const auto solType = args.get_type(i);
            if(solType == sol::type::string)
            {
                m_luaPrintFunction(getName(), args.get<std::string_view>(i));
            }
            else
            {
                sol_helper::throwSolException("Called 'print' with wrong argument type '{}'. Only string is allowed", sol_helper::GetSolTypeName(solType));
            }
        }
    }

    void LuaScriptImpl::overrideLuaPrint(LuaPrintFunction luaPrintFunction)
    {
        m_luaPrintFunction = std::move(luaPrintFunction);
    }

    const ModuleMapping& LuaScriptImpl::getModules() const
    {
        return m_modules;
    }
}
