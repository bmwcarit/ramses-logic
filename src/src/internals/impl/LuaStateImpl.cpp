//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/LuaStateImpl.h"

#include "internals/SolWrapper.h"

#include "ramses-logic/EPropertyType.h"

#include "internals/LuaScriptPropertyExtractor.h"
#include "internals/LuaScriptPropertyHandler.h"
#include "internals/SolHelper.h"

#include <iostream>

namespace rlogic::internal
{
    static int solExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description) // NOLINT(performance-unnecessary-value-param) The signature is forced by SOL. Therefore we have to disable this warning.
    {
        if (maybe_exception)
        {
            return sol::stack::push(L, description);
        }
        return sol::stack::top(L);
    }

    LuaStateImpl::LuaStateImpl()
        : m_sol(std::make_unique<sol::state>())
    {
        m_sol->open_libraries(sol::lib::base, sol::lib::string, sol::lib::io);
        m_sol->set_exception_handler(&solExceptionHandler);

        m_sol->new_usertype<LuaScriptPropertyExtractor>(
            "LuaScriptPropertyExtractor", sol::meta_method::new_index, &LuaScriptPropertyExtractor::NewIndex, sol::meta_method::index, &LuaScriptPropertyExtractor::Index);
        m_sol->new_usertype<LuaScriptPropertyHandler>(
            "LuaScriptPropertyHandler", sol::meta_method::new_index, &LuaScriptPropertyHandler::NewIndex, sol::meta_method::index, &LuaScriptPropertyHandler::Index);
        // TODO check if sol3 can handle enums more elegantly
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Float)]  = static_cast<int>(EPropertyType::Float);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec2f)]   = static_cast<int>(EPropertyType::Vec2f);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec3f)]   = static_cast<int>(EPropertyType::Vec3f);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec4f)]   = static_cast<int>(EPropertyType::Vec4f);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Int32)] = static_cast<int>(EPropertyType::Int32);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec2i)]   = static_cast<int>(EPropertyType::Vec2i);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec3i)]   = static_cast<int>(EPropertyType::Vec3i);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Vec4i)]   = static_cast<int>(EPropertyType::Vec4i);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::String)] = static_cast<int>(EPropertyType::String);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Bool)]   = static_cast<int>(EPropertyType::Bool);
        (*m_sol)[GetLuaPrimitiveTypeName(EPropertyType::Struct)] = static_cast<int>(EPropertyType::Struct);
    }

    sol::load_result LuaStateImpl::loadScript(std::string_view source, std::string_view scriptName)
    {
        return m_sol->load(source, std::string(scriptName));
    }

    std::optional<sol::environment> LuaStateImpl::createEnvironment(const sol::protected_function& rootScript)
    {
        if (!rootScript.valid())
        {
            return std::nullopt;
        }

        sol::environment        env(*m_sol, sol::create, m_sol->globals());
        env.set_on(rootScript);

        return std::make_optional(std::move(env));
    }
}
