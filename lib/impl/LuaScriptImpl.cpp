//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LuaScriptImpl.h"

#include "internals/SolState.h"
#include "impl/PropertyImpl.h"

#include "internals/PropertyTypeExtractor.h"
#include "internals/WrappedLuaProperty.h"
#include "internals/SolHelper.h"
#include "internals/ErrorReporting.h"

#include "generated/LuaScriptGen.h"

#include <iostream>

namespace rlogic::internal
{
    std::optional<CompiledScript> LuaScriptImpl::Compile(SolState& solState, std::string_view source, std::string_view scriptName, std::string_view filename, ErrorReporting& errorReporting)
    {
        const std::string chunkname = BuildChunkName(scriptName, filename);
        sol::load_result load_result = solState.loadScript(source, chunkname);

        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()));
            return std::nullopt;
        }

        sol::protected_function mainFunction = load_result;

        sol::environment env = solState.createEnvironment();
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(error.what());
            return std::nullopt;
        }

        sol::protected_function intf = env["interface"];

        if (!intf.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'interface' function defined!", chunkname));
            return std::nullopt;
        }

        sol::protected_function run = env["run"];

        if (!run.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'run' function defined!", chunkname));
            return std::nullopt;
        }

        PropertyTypeExtractor inputsExtractor("IN", EPropertyType::Struct);
        PropertyTypeExtractor outputsExtractor("OUT", EPropertyType::Struct);

        sol::environment& interfaceEnvironment = solState.getInterfaceExtractionEnvironment();

        interfaceEnvironment["IN"]  = std::ref(inputsExtractor);
        interfaceEnvironment["OUT"] = std::ref(outputsExtractor);

        interfaceEnvironment.set_on(intf);
        sol::protected_function_result intfResult = intf();

        interfaceEnvironment["IN"] = sol::lua_nil;
        interfaceEnvironment["OUT"] = sol::lua_nil;

        if (!intfResult.valid())
        {
            sol::error error = intfResult;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()));
            return std::nullopt;
        }

        return CompiledScript {
            source,
            scriptName,
            filename,
            solState,
            std::move(load_result),
            std::make_unique<Property>(std::make_unique<PropertyImpl>(inputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptInput)),
            std::make_unique<Property>(std::make_unique<PropertyImpl>(outputsExtractor.getExtractedTypeData(), EPropertySemantics::ScriptOutput))
        };
    }

    LuaScriptImpl::LuaScriptImpl(CompiledScript compiledScript)
        : LogicNodeImpl(compiledScript.scriptName)
        , m_filename(compiledScript.fileName)
        , m_source(compiledScript.sourceCode)
        , m_luaPrintFunction(&LuaScriptImpl::DefaultLuaPrintFunction)
        , m_wrappedRootInput(*compiledScript.rootInput->m_impl)
        , m_wrappedRootOutput(*compiledScript.rootOutput->m_impl)
        , m_solFunction(std::move(compiledScript.mainFunction))
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

        auto script = rlogic_serialization::CreateLuaScript(builder,
            builder.CreateString(luaScript.getName()),
            builder.CreateString(luaScript.getFilename()),
            builder.CreateString(luaScript.m_source),
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
        // TODO Violin make optional - no need to always serialize string if not used
        if (!luaScript.name())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing name!");
            return nullptr;
        }

        if (!luaScript.filename())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing filename!");
            return nullptr;
        }

        const std::string_view name = luaScript.name()->string_view();
        const std::string_view filename = luaScript.filename()->string_view();

        if (!luaScript.luaSourceCode())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing Lua source code!");
            return nullptr;
        }

        const std::string_view sourceCode = luaScript.luaSourceCode()->string_view();

        if (!luaScript.rootInput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root input!");
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootInput = PropertyImpl::Deserialize(*luaScript.rootInput(), EPropertySemantics::ScriptInput, errorReporting, deserializationMap);
        if (!rootInput)
        {
            return nullptr;
        }

        if (!luaScript.rootOutput())
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: missing root output!");
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> rootOutput = PropertyImpl::Deserialize(*luaScript.rootOutput(), EPropertySemantics::ScriptOutput, errorReporting, deserializationMap);
        if (!rootOutput)
        {
            return nullptr;
        }

        if (rootInput->getName() != "IN" || rootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root input has unexpected name or type!");
            return nullptr;
        }

        if (rootOutput->getName() != "OUT" || rootOutput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of LuaScript from serialized data: root output has unexpected name or type!");
            return nullptr;
        }

        // TODO Violin we use 'name' here, and not 'chunkname' as in Create(). This is inconsistent! Investigate closer
        sol::load_result load_result = solState.loadScript(sourceCode, name);
        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed parsing Lua source code:\n{}", name, error.what()));
            return nullptr;
        }

        sol::protected_function mainFunction = load_result;
        sol::environment env = solState.createEnvironment();
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(fmt::format("Fatal error during loading of LuaScript '{}' from serialized data: failed executing script:\n{}!", name, error.what()));
            return nullptr;
        }

        return std::make_unique<LuaScriptImpl>(
            CompiledScript{
                sourceCode,
                name,
                filename,
                solState,
                sol::protected_function(std::move(load_result)),
                std::make_unique<Property>(std::move(rootInput)),
                std::make_unique<Property>(std::move(rootOutput))
            }
            );
    }

    std::string LuaScriptImpl::BuildChunkName(std::string_view scriptName, std::string_view fileName)
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

    std::string_view LuaScriptImpl::getFilename() const
    {
        return m_filename;
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

}
