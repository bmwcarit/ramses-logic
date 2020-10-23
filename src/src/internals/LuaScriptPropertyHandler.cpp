//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/SolHelper.h"
#include "internals/LuaScriptPropertyHandler.h"
#include "internals/LuaScriptPropertySetter.h"

#include "internals/impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{
    LuaScriptPropertyHandler::LuaScriptPropertyHandler(SolState& state, PropertyImpl& propertyDescription)
        : LuaScriptHandler(state)
        , m_propertyDescription(propertyDescription)
    {
    }

    void LuaScriptPropertyHandler::NewIndex(LuaScriptPropertyHandler& data, const sol::object& index, const sol::object& rhs)
    {
        data.setChildProperty(index, rhs);
    }

    sol::object LuaScriptPropertyHandler::Index(LuaScriptPropertyHandler& data, const sol::object& index)
    {
        return data.getChildPropertyAsSolObject(index);
    }

    Property* LuaScriptPropertyHandler::getStructProperty(const sol::object& propertyIndex)
    {
        std::string_view childPropertyName = GetIndexAsString(propertyIndex);
        return getStructProperty(childPropertyName);
    }

    Property* LuaScriptPropertyHandler::getArrayProperty(const sol::object& propertyIndex)
    {
        const std::optional<size_t> maybeUInt = ExtractSpecificType<size_t>(propertyIndex);
        if (!maybeUInt)
        {
            std::string indexInfo;
            if (propertyIndex.get_type() == sol::type::number)
            {
                indexInfo = std::to_string(propertyIndex.as<int>());
            }
            else
            {
                indexInfo = sol_helper::GetSolTypeName(propertyIndex.get_type());
            }

            sol_helper::throwSolException("Only non-negative integers supported as array index type! Received {}", indexInfo);
        }
        const size_t childCount = m_propertyDescription.getChildCount();
        const size_t indexAsUInt = *maybeUInt;
        if (indexAsUInt == 0 || indexAsUInt > childCount)
        {
            sol_helper::throwSolException("Index out of range! Expected 0 < index <= {} but received index == {}", childCount, indexAsUInt);
        }
        return m_propertyDescription.getChild(indexAsUInt - 1);
    }

    Property* LuaScriptPropertyHandler::getStructProperty(std::string_view propertyName)
    {
        Property* structProperty = m_propertyDescription.getChild(propertyName);
        if (nullptr == structProperty)
        {
            sol_helper::throwSolException("Tried to access undefined struct property '{}'", propertyName);
        }
        return structProperty;
    }

    void LuaScriptPropertyHandler::setChildProperty(const sol::object& propertyIndex, const sol::object& rhs)
    {
        Property* childProperty = nullptr;

        if (m_propertyDescription.getType() == EPropertyType::Struct)
        {
            childProperty = getStructProperty(propertyIndex);
        }
        else
        {
            childProperty = getArrayProperty(propertyIndex);
        }

        LuaScriptPropertySetter::Set(*childProperty, rhs);
    }

    sol::object LuaScriptPropertyHandler::getChildPropertyAsSolObject(const sol::object& propertyIndex)
    {
        const EPropertyType propertyType = m_propertyDescription.getType();
        if (propertyType == EPropertyType::Struct)
        {
            const Property* structProperty = getStructProperty(propertyIndex);
            return convertPropertyToSolObject(*structProperty->m_impl);
        }

        if (propertyType == EPropertyType::Array)
        {
            const Property* arrayProperty = getArrayProperty(propertyIndex);
            return convertPropertyToSolObject(*arrayProperty->m_impl);
        }
        // Not a struct -> assume it's an array-like type
        const size_t maxIndex   = GetMaxIndexForVectorType(propertyType);
        std::optional<size_t> maybeUInt = ExtractSpecificType<size_t>(propertyIndex);
        if (!maybeUInt)
        {
            sol_helper::throwSolException("Only non-negative integers supported as array index type! Received value: {}", propertyIndex.as<float>());
        }

        size_t indexAsUInt = *maybeUInt;
        if (indexAsUInt == 0 || indexAsUInt > maxIndex)
        {
            sol_helper::throwSolException("Index out of range! Expected 0 < index <= {} but received index == {}", maxIndex, indexAsUInt);
        }

        // Compensate for Lua's indexing which starts from 1
        --indexAsUInt;

        switch (propertyType)
        {
        case EPropertyType::Vec2f:
            return m_state.createUserObject((*m_propertyDescription.get<vec2f>()).at(indexAsUInt));
        case EPropertyType::Vec3f:
            return m_state.createUserObject((*m_propertyDescription.get<vec3f>()).at(indexAsUInt));
        case EPropertyType::Vec4f:
            return m_state.createUserObject((*m_propertyDescription.get<vec4f>()).at(indexAsUInt));
        case EPropertyType::Vec2i:
            return m_state.createUserObject((*m_propertyDescription.get<vec2i>()).at(indexAsUInt));
        case EPropertyType::Vec3i:
            return m_state.createUserObject((*m_propertyDescription.get<vec3i>()).at(indexAsUInt));
        case EPropertyType::Vec4i:
            return m_state.createUserObject((*m_propertyDescription.get<vec4i>()).at(indexAsUInt));
        // TODO Violin/Sven/Tobias this kind of a bad design, and the reason for it lies
        // with the fact that we handle 3 different things in the same base class - "Property"
        // Discuss whether we want this pattern, or maybe there are some other ideas how to deal
        // with type abstraction and polymorphy where we would not have this problem
        case EPropertyType::Struct:
        case EPropertyType::Array:
        case EPropertyType::Float:
        case EPropertyType::Int32:
        case EPropertyType::String:
        case EPropertyType::Bool:
            assert(false && "Should not have reached this code!");
        }

        return sol::nil;
    }

    sol::object LuaScriptPropertyHandler::getChildPropertyAsSolObject(std::string_view childName)
    {
        const Property* structProperty = getStructProperty(childName);
        return convertPropertyToSolObject(*structProperty->m_impl);
    }

    sol::object LuaScriptPropertyHandler::convertPropertyToSolObject(PropertyImpl& propertyToConvert)
    {
        switch (propertyToConvert.getType())
        {
        case EPropertyType::Float:
            return convertPropertyToSolObject<float>(propertyToConvert);
        case EPropertyType::Int32:
            return convertPropertyToSolObject<int32_t>(propertyToConvert);
        case EPropertyType::String:
            return convertPropertyToSolObject<std::string>(propertyToConvert);
        case EPropertyType::Bool:
            return convertPropertyToSolObject<bool>(propertyToConvert);
        case EPropertyType::Vec2f:
        case EPropertyType::Vec3f:
        case EPropertyType::Vec4f:
        case EPropertyType::Vec2i:
        case EPropertyType::Vec3i:
        case EPropertyType::Vec4i:
        case EPropertyType::Array:
        case EPropertyType::Struct:
            // TODO Violin/Sven this is probably a performance hit, we create a new lua table every
            // time a property is resolved
            return m_state.createUserObject(LuaScriptPropertyHandler(m_state, propertyToConvert));
        }

        assert(false && "Missing type implementation!");
        return sol::lua_nil;
    }

    const PropertyImpl& LuaScriptPropertyHandler::getPropertyImpl() const
    {
        return m_propertyDescription;
    }
}
