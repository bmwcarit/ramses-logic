//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/Property.h"
#include "internals/LuaScriptPropertyExtractor.h"
#include "internals/SolHelper.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/impl/LuaStateImpl.h"
#include <algorithm>

namespace rlogic::internal
{
    LuaScriptPropertyExtractor::LuaScriptPropertyExtractor(LuaStateImpl& state, PropertyImpl& propertyDescription)
        : LuaScriptHandler(state)
        , m_propertyDescription(propertyDescription)
    {
    }

    void LuaScriptPropertyExtractor::NewIndex(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& rhs)
    {
        data.addEntryToPropertyDescription(index, rhs, data.m_propertyDescription);
    }

    sol::object LuaScriptPropertyExtractor::Index(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& /*rhs*/)
    {
        return data.getProperty(index);
    }

    sol::object LuaScriptPropertyExtractor::getProperty(const sol::object& index)
    {
        const auto childName = GetIndexAsString(index);
        const auto child = m_propertyDescription.getChild(childName);

        if (nullptr != child)
        {
            return m_state.createUserObject(LuaScriptPropertyExtractor(m_state, *child->m_impl));
        }

        sol_helper::throwSolException("Trying to access not available property {} in interface!", childName);
        return sol::nil;
    }

    void LuaScriptPropertyExtractor::addEntryToPropertyDescription(const sol::object& index, const sol::object& value, PropertyImpl& property)
    {
        const auto name = GetIndexAsString(index);

        if (nullptr != property.getChild(name))
        {
            sol_helper::throwSolException("Property '{}' already exists! Can't declare the same property twice!", name);
        }

        const auto solType = value.get_type();
        if (solType == sol::type::number)
        {
            const auto type = value.as<EPropertyType>();
            if (type == EPropertyType::Float ||
                type == EPropertyType::Vec2f ||
                type == EPropertyType::Vec3f ||
                type == EPropertyType::Vec4f ||
                type == EPropertyType::Int32 ||
                type == EPropertyType::Vec2i ||
                type == EPropertyType::Vec3i ||
                type == EPropertyType::Vec4i ||
                type == EPropertyType::String ||
                type == EPropertyType::Bool)
            {
                property.addChild(std::make_unique<PropertyImpl>(name, type));
            }
            else
            {
                sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", name);
            }
        }
        else if (solType == sol::type::table)
        {
            const auto table          = value.as<sol::table>();
            auto propertyStruct = std::make_unique<PropertyImpl>(name, EPropertyType::Struct);

            for (auto& tableEntry : table)
            {
                addEntryToPropertyDescription(tableEntry.first, tableEntry.second, *propertyStruct);
            }

            property.addChild(std::move(propertyStruct));
        }
        else
        {
            sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", name);
        }
    }
}
