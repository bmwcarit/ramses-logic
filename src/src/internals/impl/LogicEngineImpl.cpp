//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/LogicEngineImpl.h"
#include "internals/impl/LuaScriptImpl.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/impl/RamsesNodeBindingImpl.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"

#include "generated/logicengine_gen.h"

#include <string>
#include <fstream>
#include <streambuf>
#include "flatbuffers/util.h"

namespace rlogic::internal
{
    LuaScript* LogicEngineImpl::createLuaScriptFromFile(std::string_view filename, std::string_view scriptName)
    {
        m_errors.clear();

        std::ifstream iStream;
        // ifstream has no support for string_view
        iStream.open(std::string(filename), std::ios_base::in);
        if (iStream.fail())
        {
            // TODO Violin need to report error here too! Error mechanism needs rethinking
            m_errors.emplace_back("Failed opening file " + std::string(filename) + "!");
            return nullptr;
        }

        std::string source(std::istreambuf_iterator<char>(iStream), std::istreambuf_iterator<char>{});
        return createLuaScriptInternal(source, filename, scriptName);
    }

    LuaScript* LogicEngineImpl::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        m_errors.clear();
        return createLuaScriptInternal(source, "", scriptName);
    }

    LuaScript* LogicEngineImpl::createLuaScriptInternal(std::string_view source, std::string_view filename, std::string_view scriptName)
    {
        auto scriptImpl = LuaScriptImpl::Create(m_luaState, source, scriptName, filename, m_errors);
        if (scriptImpl)
        {
            m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(scriptImpl)));
            return m_scripts.back().get();
        }

        return nullptr;
    }

    bool LogicEngineImpl::destroy(LuaScript& luaScript)
    {
        m_errors.clear();

        auto scriptIter = find_if(m_scripts.begin(), m_scripts.end(), [&](const std::unique_ptr<LuaScript>& script) {
            return script.get() == &luaScript;
        });
        if (scriptIter == m_scripts.end())
        {
            m_errors.emplace_back("Can't find script in logic engine!");
            return false;
        }
        m_scripts.erase(scriptIter);
        return true;
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(std::string_view name)
    {
        m_errors.clear();
        m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(RamsesNodeBindingImpl::Create(name)));
        return m_ramsesNodeBindings.back().get();
    }

    bool LogicEngineImpl::destroy(RamsesNodeBinding& ramsesNodeBinding)
    {
        m_errors.clear();

        auto nodeIter = find_if(m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), [&](const std::unique_ptr<RamsesNodeBinding>& nodeBinding)
        {
            return nodeBinding.get() == &ramsesNodeBinding;
        });

        if (nodeIter == m_ramsesNodeBindings.end())
        {
            m_errors.emplace_back("Can't find RamsesNodeBinding in logic engine!");
            return false;
        }
        return true;
    }

    bool LogicEngineImpl::update()
    {
        m_errors.clear();

        // TODO Sort the scripts before execution to get a valid order
        for (auto& script : m_scripts)
        {
            if(!script->m_impl.get().update())
            {
                const auto errors = script->m_impl.get().getErrors();
                m_errors.insert(m_errors.end(), errors.begin(), errors.end());
                return false;
            }
        }
        for (auto& ramsesNodeBinding : m_ramsesNodeBindings)
        {
            bool success = ramsesNodeBinding->m_impl.get().update();
            assert(success && "Bindings update can never fail!");
            (void)success;
        }
        return true;
    }

    const std::vector<std::string>& LogicEngineImpl::getErrors() const
    {
        return m_errors;
    }

    bool LogicEngineImpl::loadFromFile(std::string_view filename)
    {
        m_errors.clear();
        m_scripts.clear();
        m_ramsesNodeBindings.clear();

        std::string sFilename(filename);
        std::string buf;
        if(!flatbuffers::LoadFile(sFilename.c_str(), true, &buf))
        {
            m_errors.emplace_back(std::string("Error reading file: ").append(filename));
            return false;
        }

        const auto logicEngine = serialization::GetLogicEngine(buf.data());
        assert(nullptr != logicEngine);
        assert(nullptr != logicEngine->luascripts());
        assert(nullptr != logicEngine->ramsesnodebindings());

        const auto luascripts = logicEngine->luascripts();

        if (luascripts->size() != 0)
        {
            m_scripts.reserve(luascripts->size());
        }

        for (auto script : *luascripts)
        {
            // script can't be null. Already handled by flatbuffers
            assert(nullptr != script);
            auto newScript = LuaScriptImpl::Create(m_luaState, script, m_errors);
            if (nullptr != newScript)
            {
                m_scripts.emplace_back(std::make_unique<LuaScript>(std::move(newScript)));
            }
        }

        const auto ramsesnodebindings = logicEngine->ramsesnodebindings();

        if (ramsesnodebindings->size() != 0)
        {
            m_ramsesNodeBindings.reserve(ramsesnodebindings->size());
        }

        for (auto rNodeBinding : *ramsesnodebindings)
        {
            // rNodeBinding can't be null. Already handled by flatbuffers
            assert(nullptr != ramsesnodebindings);
            auto newBinding = RamsesNodeBindingImpl::Create(*rNodeBinding);
            if (nullptr != newBinding)
            {
                m_ramsesNodeBindings.emplace_back(std::make_unique<RamsesNodeBinding>(std::move(newBinding)));
            }
        }

        return true;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename) const
    {
        flatbuffers::FlatBufferBuilder builder;

        std::vector<flatbuffers::Offset<serialization::LuaScript>> luascripts;
        luascripts.reserve(m_scripts.size());

        std::transform(m_scripts.begin(), m_scripts.end(), std::back_inserter(luascripts), [&builder](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) {
            return it->m_script->serialize(builder);
        });

        std::vector<flatbuffers::Offset<serialization::RamsesNodeBinding>> ramsesnodebindings;
        ramsesnodebindings.reserve(m_ramsesNodeBindings.size());

        std::transform(m_ramsesNodeBindings.begin(),
                       m_ramsesNodeBindings.end(),
                       std::back_inserter(ramsesnodebindings),
                       [&builder](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) { return it->m_nodeBinding->serialize(builder); });

        auto logicEngine = serialization::CreateLogicEngine(builder, builder.CreateVector(luascripts), builder.CreateVector(ramsesnodebindings));

        builder.Finish(logicEngine);
        std::string sFileName(filename);

        return flatbuffers::SaveFile(sFileName.c_str(), reinterpret_cast<const char*>(builder.GetBufferPointer()), builder.GetSize(), true); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) We know what we do and this is only solvable with reinterpret-cast
    }

    LuaScript* LogicEngineImpl::findLuaScriptByName(std::string_view name)
    {
        auto iter = std::find_if(m_scripts.begin(), m_scripts.end(), [&name](const std::vector<std::unique_ptr<LuaScript>>::value_type& it) { return name == it->getName();
        });
        return iter != m_scripts.end() ? iter->get() : nullptr;
    }

    RamsesNodeBinding* LogicEngineImpl::findRamsesNodeBindingByName(std::string_view name)
    {
        auto iter = std::find_if(
            m_ramsesNodeBindings.begin(), m_ramsesNodeBindings.end(), [&name](const std::vector<std::unique_ptr<RamsesNodeBinding>>::value_type& it) { return name == it->getName(); });
        return iter != m_ramsesNodeBindings.end() ? iter->get() : nullptr;
    }
}
