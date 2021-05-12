//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/SolState.h"

#include "internals/SolWrapper.h"

#include "ramses-logic/EPropertyType.h"

#include "internals/LuaScriptPropertyExtractor.h"
#include "internals/LuaScriptPropertyHandler.h"
#include "internals/SolHelper.h"
#include "internals/ArrayTypeInfo.h"


#include <iostream>

namespace rlogic::internal
{
    // NOLINTNEXTLINE(performance-unnecessary-value-param) The signature is forced by SOL. Therefore we have to disable this warning.
    static int solExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description)
    {
        if (maybe_exception)
        {
            return sol::stack::push(L, description);
        }
        return sol::stack::top(L);
    }

    SolState::SolState()
    {
        m_solState.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::debug);
        m_solState.set_exception_handler(&solExceptionHandler);

        m_solState.new_usertype<ArrayTypeInfo>("ArrayTypeInfo");
        m_solState.new_usertype<LuaScriptPropertyExtractor>(
            "LuaScriptPropertyExtractor", sol::meta_method::new_index, &LuaScriptPropertyExtractor::newIndex, sol::meta_method::index, &LuaScriptPropertyExtractor::index);
        m_solState.new_usertype<LuaScriptPropertyHandler>(
            "LuaScriptPropertyHandler", sol::meta_method::new_index, &LuaScriptPropertyHandler::NewIndex, sol::meta_method::index, &LuaScriptPropertyHandler::Index);
        // TODO check if sol3 can handle enums more elegantly
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Float)]  = static_cast<int>(EPropertyType::Float);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec2f)]   = static_cast<int>(EPropertyType::Vec2f);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec3f)]   = static_cast<int>(EPropertyType::Vec3f);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec4f)]   = static_cast<int>(EPropertyType::Vec4f);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Int32)] = static_cast<int>(EPropertyType::Int32);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec2i)]   = static_cast<int>(EPropertyType::Vec2i);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec3i)]   = static_cast<int>(EPropertyType::Vec3i);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Vec4i)]   = static_cast<int>(EPropertyType::Vec4i);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::String)] = static_cast<int>(EPropertyType::String);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Bool)]   = static_cast<int>(EPropertyType::Bool);
        m_solState[GetLuaPrimitiveTypeName(EPropertyType::Struct)] = static_cast<int>(EPropertyType::Struct);
        m_solState.set_function(GetLuaPrimitiveTypeName(EPropertyType::Array), &LuaScriptPropertyExtractor::CreateArray);
    }

    sol::load_result SolState::loadScript(std::string_view source, std::string_view scriptName)
    {
        return m_solState.load(source, std::string(scriptName));
    }

    sol::environment SolState::createEnvironment()
    {
        return sol::environment(m_solState, sol::create, m_solState.globals());
    }
}
