//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LuaScript.h"
#include "impl/LuaScriptImpl.h"
#include "impl/PropertyImpl.h"
#include "ramses-logic/Property.h"

namespace rlogic
{
    LuaScript::LuaScript(std::unique_ptr<internal::LuaScriptImpl> impl) noexcept
        // The impl pointer is owned by this class, but a reference to the data is passed to the base class
        // TODO MSVC2017 fix for std::reference_wrapper with base class
        : LogicNode(std::ref(static_cast<internal::LogicNodeImpl&>(*impl)))
        , m_script(std::move(impl))
    {
    }

    LuaScript::~LuaScript() noexcept = default;

    std::string_view LuaScript::getFilename() const
    {
        return m_script->getFilename();
    }

    void LuaScript::overrideLuaPrint(LuaPrintFunction luaPrintFunction)
    {
        m_script->overrideLuaPrint(std::move(luaPrintFunction));
    }
}
