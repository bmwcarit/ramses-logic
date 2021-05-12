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

#include "internals/LuaScriptPropertyExtractor.h"
#include "internals/LuaScriptPropertyHandler.h"
#include "internals/SolHelper.h"
#include "internals/ErrorReporting.h"

#include "generated/luascript_gen.h"

#include <iostream>

namespace rlogic::internal
{
    std::unique_ptr<LuaScriptImpl> LuaScriptImpl::Create(SolState& solState, std::string_view source, std::string_view scriptName, std::string_view filename, ErrorReporting& errorReporting)
    {
        const std::string chunkname = BuildChunkName(scriptName, filename);
        sol::load_result load_result = solState.loadScript(source, chunkname);

        if (!load_result.valid())
        {
            sol::error error = load_result;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()));
            return nullptr;
        }

        sol::protected_function mainFunction = load_result;

        sol::environment env = solState.createEnvironment();
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        if (!main_result.valid())
        {
            sol::error error = main_result;
            errorReporting.add(error.what());
            return nullptr;
        }

        sol::protected_function intf = env["interface"];

        if (!intf.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'interface' function defined!", chunkname));
            return nullptr;
        }

        sol::protected_function run = env["run"];

        if (!run.valid())
        {
            errorReporting.add(fmt::format("[{}] No 'run' function defined!", chunkname));
            return nullptr;
        }

        auto inputsImpl = std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::ScriptInput);
        auto outputsImpl = std::make_unique<PropertyImpl>("OUT", EPropertyType::Struct, EPropertySemantics::ScriptOutput);

        env["IN"]  = solState.createUserObject(LuaScriptPropertyExtractor(*inputsImpl));
        env["OUT"] = solState.createUserObject(LuaScriptPropertyExtractor(*outputsImpl));

        sol::protected_function_result intfResult = intf();

        if (!intfResult.valid())
        {
            sol::error error = intfResult;
            errorReporting.add(fmt::format("[{}] Error while loading script. Lua stack trace:\n{}", chunkname, error.what()));
            return nullptr;
        }

        return std::unique_ptr<LuaScriptImpl>(new LuaScriptImpl (solState, sol::protected_function(std::move(load_result)),
            scriptName, filename, source, std::move(inputsImpl), std::move(outputsImpl)));
    }

    LuaScriptImpl::LuaScriptImpl(SolState& solState,
        sol::protected_function       solFunction,
        std::string_view              scriptName,
        std::string_view              filename,
        std::string_view              source,
        std::unique_ptr<PropertyImpl> inputs,
        std::unique_ptr<PropertyImpl> outputs)
        : LogicNodeImpl(scriptName, std::move(inputs), std::move(outputs))
        , m_filename(filename)
        , m_source(source)
        , m_state(solState)
        , m_solFunction(std::move(solFunction))
        , m_luaPrintFunction(&LuaScriptImpl::DefaultLuaPrintFunction)
    {
        initParameters();
    }

    flatbuffers::Offset<rlogic_serialization::LuaScript> LuaScriptImpl::Serialize(const LuaScriptImpl& luaScript, flatbuffers::FlatBufferBuilder& builder)
    {
        // TODO Violin investigate options to save byte code, instead of plain text, e.g.:
        //sol::bytecode scriptCode = m_solFunction.dump();

        auto luascript = rlogic_serialization::CreateLuaScript(
            builder,
            LogicNodeImpl::Serialize(luaScript, builder), // Serialize base class
            builder.CreateString(luaScript.m_filename),
            builder.CreateString(luaScript.m_source),
            0
        );

        builder.Finish(luascript);
        return luascript;
    }

    std::unique_ptr<LuaScriptImpl> LuaScriptImpl::Deserialize(SolState& solState, const rlogic_serialization::LuaScript& luaScript)
    {
        assert(nullptr != luaScript.filename());
        assert(nullptr != luaScript.logicnode());
        assert(nullptr != luaScript.logicnode()->name());

        const auto name = luaScript.logicnode()->name()->string_view();

        const auto filename = luaScript.filename()->string_view();

        std::string_view load_source;
        std::string_view string_source;

        assert (nullptr == luaScript.bytecode() && "Bytecode serialization not implemented yet!");
        // TODO Violin/Sven re-enable this once bytecode is implemented and tested
        //load_source = std::string_view(reinterpret_cast<const char*>(luaScript.bytecode()->data()) ,
        //luaScript.bytecode()->size()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) We know what we do and this is only solvable with reinterpret-cast
        assert(nullptr != luaScript.source());
        assert(nullptr != luaScript.logicnode()->inputs());
        assert(nullptr != luaScript.logicnode()->outputs());
        load_source = luaScript.source()->string_view();
        string_source = load_source;

        auto inputs = PropertyImpl::Deserialize(*luaScript.logicnode()->inputs(), EPropertySemantics::ScriptInput);
        assert(inputs);

        auto outputs = PropertyImpl::Deserialize(*luaScript.logicnode()->outputs(), EPropertySemantics::ScriptOutput);
        assert(outputs);

        sol::load_result load_result = solState.loadScript(load_source, name);
        assert (load_result.valid());

        sol::protected_function mainFunction = load_result;
        sol::environment env = solState.createEnvironment();
        env.set_on(mainFunction);

        sol::protected_function_result main_result = mainFunction();
        assert (main_result.valid());
        return std::unique_ptr<LuaScriptImpl>(new LuaScriptImpl(solState, sol::protected_function(std::move(load_result)), name, filename, string_source, std::move(inputs), std::move(outputs)));
    }

    // TODO Violin can we remove this init, e.g. move to Create?
    void LuaScriptImpl::initParameters()
    {
        sol::environment env = sol::get_environment(m_solFunction);

        env["IN"]  = m_state.get().createUserObject(LuaScriptPropertyHandler(m_state, *getInputs()->m_impl));
        env["OUT"] = m_state.get().createUserObject(LuaScriptPropertyHandler(m_state, *getOutputs()->m_impl));
        // override the lua print function to handle it by ourselves
        env.set_function("print", &LuaScriptImpl::luaPrint, this);
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
