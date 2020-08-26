//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/impl/LuaStateImpl.h"

#include <optional>
#include <vector>
#include <string>
#include <string_view>

namespace rlogic
{
    class RamsesNodeBinding;
    class LuaScript;
}

namespace rlogic::internal
{
    class LogicEngineImpl
    {
    public:
        // Move-able (noexcept); Not copy-able
        LogicEngineImpl() noexcept                        = default;
        ~LogicEngineImpl() noexcept                       = default;
        LogicEngineImpl(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl& operator=(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl(const LogicEngineImpl& other)                = delete;
        LogicEngineImpl& operator=(const LogicEngineImpl& other) = delete;

        LuaScript* createLuaScriptFromFile(std::string_view filename, std::string_view scriptName);
        LuaScript* createLuaScriptFromSource(std::string_view source, std::string_view scriptName);
        LuaScript* findLuaScriptByName(std::string_view name);
        bool       destroy(LuaScript& luaScript);

        RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);
        RamsesNodeBinding* findRamsesNodeBindingByName(std::string_view name);
        bool               destroy(RamsesNodeBinding& ramsesNodeBinding);

        bool                            update();
        const std::vector<std::string>& getErrors() const;


        bool loadFromFile(std::string_view filename);
        bool saveToFile(std::string_view filename) const;

    private:
        LuaStateImpl                                    m_luaState;
        std::vector<std::string>                        m_errors;
        std::vector<std::unique_ptr<LuaScript>>         m_scripts;
        std::vector<std::unique_ptr<RamsesNodeBinding>> m_ramsesNodeBindings;

        LuaScript* createLuaScriptInternal(std::string_view source, std::string_view filename, std::string_view scriptName);
    };
}
