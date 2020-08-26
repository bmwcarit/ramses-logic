//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"

#include "internals/impl/LogicEngineImpl.h"
#include "internals/impl/LuaScriptImpl.h"
#include "internals/impl/RamsesNodeBindingImpl.h"

#include <string>

namespace rlogic
{
    LogicEngine::LogicEngine() noexcept
        : m_impl(std::make_unique<internal::LogicEngineImpl>())
    {
    }

    LogicEngine::~LogicEngine() noexcept = default;

    rlogic::LuaScript* LogicEngine::createLuaScriptFromFile(std::string_view filename, std::string_view scriptName)
    {
        return m_impl->createLuaScriptFromFile(filename, scriptName);
    }

    rlogic::LuaScript* LogicEngine::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        return m_impl->createLuaScriptFromSource(source, scriptName);
    }

    rlogic::RamsesNodeBinding* LogicEngine::createRamsesNodeBinding(std::string_view name)
    {
        return m_impl->createRamsesNodeBinding(name);
    }

    bool LogicEngine::destroy(RamsesNodeBinding& ramsesNodeBinding)
    {
        return m_impl->destroy(ramsesNodeBinding);
    }

    bool LogicEngine::destroy(LuaScript& luaScript)
    {
        return m_impl->destroy(luaScript);
    }

    const std::vector<std::string>& LogicEngine::getErrors() const
    {
        return m_impl->getErrors();
    }

    bool LogicEngine::update()
    {
        return m_impl->update();
    }

    bool LogicEngine::loadFromFile(std::string_view filename)
    {
        return m_impl->loadFromFile(filename);
    }

    bool LogicEngine::saveToFile(std::string_view filename) const
    {
        return m_impl->saveToFile(filename);
    }
}
