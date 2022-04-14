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
#include "internals/PropertyTypeExtractor.h"
#include "internals/EnvironmentProtection.h"

#include "generated/LuaScriptGen.h"

#include <iostream>

namespace rlogic::internal
{
    LuaScriptImpl::LuaScriptImpl(LuaCompiledScript compiledScript, std::string_view name, uint64_t id)
        : LogicNodeImpl(name, id)
        , m_source(std::move(compiledScript.source.sourceCode))
        , m_wrappedRootInput(*compiledScript.rootInput->m_impl)
        , m_wrappedRootOutput(*compiledScript.rootOutput->m_impl)
        , m_runFunction(std::move(compiledScript.runFunction))
        , m_modules(std::move(compiledScript.source.userModules))
        , m_stdModules(std::move(compiledScript.source.stdModules))
    {
        setRootProperties(std::move(compiledScript.rootInput), std::move(compiledScript.rootOutput));
    }

    void LuaScriptImpl::createRootProperties()
    {
        // unlike other logic objects, luascript properties created outside of it (from script or deserialized)
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
                    module.second->getId()));
        }

        std::vector<uint8_t> stdModules;
        stdModules.reserve(luaScript.m_stdModules.size());
        for (const EStandardModule stdModule : luaScript.m_stdModules)
        {
            stdModules.push_back(static_cast<uint8_t>(stdModule));
        }

        auto script = rlogic_serialization::CreateLuaScript(builder,
            LogicObjectImpl::Serialize(luaScript, builder),
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
        std::string name;
        uint64_t id = 0u;
        uint64_t userIdHigh = 0u;
        uint64_t userIdLow = 0u;
        if (!LogicObjectImpl::Deserialize(luaScript.base(), name, id, userIdHigh, userIdLow, errorReporting))
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing name and/or ID!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!luaScript.luaSourceCode())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing Lua source code!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }
        std::string sourceCode = luaScript.luaSourceCode()->str();

        if (!luaScript.rootInput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root input!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootInput = PropertyImpl::Deserialize(*luaScript.rootInput(), EPropertySemantics::ScriptInput, errorReporting, deserializationMap);
        if (!rootInput)
        {
            return nullptr;
        }

        if (!luaScript.rootOutput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root output!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootOutput = PropertyImpl::Deserialize(*luaScript.rootOutput(), EPropertySemantics::ScriptOutput, errorReporting, deserializationMap);
        if (!rootOutput)
        {
            return nullptr;
        }

        if (rootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root input has unexpected type!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (rootOutput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root output has unexpected type!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        // TODO Violin we use 'name' here, and not 'chunkname' as in Create(). This is inconsistent! Investigate closer
        sol::load_result load_result = solState.loadScript(sourceCode, name);
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed parsing Lua source code:\n{}", name, error.what()), nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        if (!luaScript.userModules())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing user module dependencies!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }
        ModuleMapping userModules;
        userModules.reserve(luaScript.userModules()->size());
        for (const auto* module : *luaScript.userModules())
        {
            if (!module->name())
            {
                errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' module data: missing name!", name), nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }
            const LuaModule* moduleUsed = deserializationMap.resolveLuaModule(module->moduleId());

            if (!moduleUsed)
            {
                errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' module data: could not resolve dependent module with id={}!", name, module->moduleId()), nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }

            userModules.emplace(module->name()->str(), moduleUsed);
        }

        if (!luaScript.standardModules())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing standard module dependencies!", nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }
        StandardModules stdModules;
        stdModules.reserve(luaScript.standardModules()->size());
        for (const uint8_t stdModule : *luaScript.standardModules())
            stdModules.push_back(static_cast<EStandardModule>(stdModule));

        sol::protected_function mainFunction = load_result;
        sol::environment env = solState.createEnvironment(stdModules, userModules);
        sol::table internalEnv = EnvironmentProtection::GetProtectedEnvironmentTable(env);

        env.set_on(mainFunction);

        sol::protected_function_result main_result {};
        {
            ScopedEnvironmentProtection p(env, EEnvProtectionFlag::LoadScript);
            main_result = mainFunction();
        }

        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed executing script:\n{}!", name, error.what()), nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        env["GLOBAL"] = solState.createTable();
        sol::protected_function init = internalEnv["init"];

        if (init.valid())
        {
            // in order to support interface definitions in globals we need to register the symbols for init section
            PropertyTypeExtractor::RegisterTypes(internalEnv);
            sol::protected_function_result initResult {};
            {
                ScopedEnvironmentProtection p(env, EEnvProtectionFlag::InitFunction);
                initResult = init();
            }
            PropertyTypeExtractor::UnregisterTypes(internalEnv);

            if (!initResult.valid())
            {
                sol::error error = initResult;
                errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed initializing script:\n{}!", name, error.what()), nullptr, EErrorType::BinaryVersionMismatch);
                return nullptr;
            }
        }

        sol::protected_function run = internalEnv["run"];

        if (!run.valid())
        {
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: scripts contains no run() function!", name), nullptr, EErrorType::BinaryVersionMismatch);
            return nullptr;
        }

        auto inputs = std::make_unique<Property>(std::move(rootInput));
        auto outputs = std::make_unique<Property>(std::move(rootOutput));

        EnvironmentProtection::SetEnvironmentProtectionLevel(env, EEnvProtectionFlag::RunFunction);

        auto deserialized = std::make_unique<LuaScriptImpl>(
            LuaCompiledScript{
                LuaSource{
                    std::move(sourceCode),
                    solState,
                    std::move(stdModules),
                    std::move(userModules)
                },
                std::move(run),
                std::move(inputs),
                std::move(outputs)
            },
            name, id);

        deserialized->setUserId(userIdHigh, userIdLow);

        return deserialized;
    }

    std::optional<LogicNodeRuntimeError> LuaScriptImpl::update()
    {
        sol::protected_function_result result = m_runFunction(std::ref(m_wrappedRootInput), std::ref(m_wrappedRootOutput));

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
