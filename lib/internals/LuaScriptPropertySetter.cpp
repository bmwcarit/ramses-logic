//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------


#include "internals/LuaScriptPropertySetter.h"
#include "internals/LuaScriptPropertyHandler.h"
#include "internals/SolHelper.h"
#include "internals/LuaTypeConversions.h"

#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{
    // TODO Violin/Sven we need to redesign this class a bit. The type polymorphy is a bit too spread across several functions
    // and not consistent enough - at some places we check the incoming lua type, other places our internal type... Ideas:
    // - maybe start with the C++ side of things - what do we expect? Then proceed to check if the lua object is of compatible type
    // - it's probably possible to unify the "table" handling between tables and user types using sol mechanics
    // - maybe possible to simplify some of the code by using sol iterators instead of for loops and array checks

    void LuaScriptPropertySetter::Set(PropertyImpl& property, const sol::object& value)
    {
        switch (value.get_type())
        {
        case sol::type::number:
            SetNumber(property, value);
            break;
        case sol::type::table:
            SetTable(property, value.as<sol::table>());
            break;
        case sol::type::string:
            SetString(property, value.as<std::string_view>());
            break;
        case sol::type::boolean:
            SetBool(property, value.as<bool>());
            break;
        case sol::type::userdata:
        {
            // This is equivalent to SetTable, but is needed when the right side is a custom type, not a Lua table
            // For example when assigning OUT.someStruct = IN.someStruct

            sol::optional<LuaScriptPropertyHandler> maybeStructPropertyHandler = value.as<sol::optional<LuaScriptPropertyHandler>>();
            if (!maybeStructPropertyHandler)
            {
                // TODO Violin this error message can be made more concrete if we refactor how we deal with userdata (See TODO at the top of the file)
                sol_helper::throwSolException("Unexpected object type assigned to property '{}'!", property.getName());
            }

            LuaScriptPropertyHandler& structPropertyHandler = *maybeStructPropertyHandler;
            // TODO (Violin/Sven) this error check and switch can be probably simplified a lot by overloading the PropertyImpl assignment operator and using it, instead of
            // going over Lua/Sol here - in the end, we are juggling C++ objects, might as well treat them as such

            const EPropertyType expectedType = property.getType();
            const EPropertyType receivedType = structPropertyHandler.getPropertyImpl().getType();
            if (expectedType != receivedType)
            {
                sol_helper::throwSolException("Type mismatch while assigning property '{}'! Expected {} but received {}",
                    property.getName(),
                    GetLuaPrimitiveTypeName(expectedType),
                    GetLuaPrimitiveTypeName(receivedType)
                    );
            }

            switch (property.getType())
            {
            case EPropertyType::Array:
                SetArray(property, structPropertyHandler);
                break;
            case EPropertyType::Struct:
                SetStruct(property, structPropertyHandler);
                break;
            case EPropertyType::Vec2f:
            case EPropertyType::Vec3f:
            case EPropertyType::Vec4f:
            case EPropertyType::Vec2i:
            case EPropertyType::Vec3i:
            case EPropertyType::Vec4i:
                property.setOutputValue_FromScript(structPropertyHandler.getPropertyImpl().getValue());
                break;
            case EPropertyType::String:
            case EPropertyType::Bool:
            case EPropertyType::Float:
            case EPropertyType::Int32:
                // TODO Violin this should be tested more! (Or we refactor the code that this switch doesn't exist in the first place)
                sol_helper::throwSolException("Assigning non-primitive value to property '{}'!", property.getName());
            }
            break;
        }
        case sol::type::nil:
            sol_helper::throwSolException("Assigning nil to {} output '{}'!", GetLuaPrimitiveTypeName(property.getType()), property.getName());
            break;
        default:
            sol_helper::throwSolException("Tried to set unsupported type");
            break;
        }
    }

    void LuaScriptPropertySetter::SetNumber(PropertyImpl& property, const sol::object& number)
    {
        if (property.getPropertySemantics() != EPropertySemantics::ScriptOutput)
        {
            sol_helper::throwSolException("Error while writing to '{}'. Writing input values is not allowed, only outputs!", property.getName());
        }

        switch (property.getType())
        {
        case EPropertyType::Float:
            property.setOutputValue_FromScript(number.as<float>());
            break;
        case EPropertyType::Int32:
        {
            std::optional<int32_t> potentiallyInt32 = LuaTypeConversions::ExtractSpecificType<int32_t>(number);
            if (potentiallyInt32)
            {
                property.setOutputValue_FromScript(*potentiallyInt32);
            }
            else
            {
                sol_helper::throwSolException("Implicit rounding during assignment of integer output '{}' (value: {})!", property.getName(), number.as<float>());
            }
        }
            break;
        default:
            sol_helper::throwSolException(
                "Assigning wrong type ({}) to output '{}'!", sol_helper::GetSolTypeName(number.get_type()), property.getName());
        }
    }

    void LuaScriptPropertySetter::SetTable(PropertyImpl& property, const sol::table& table)
    {
        // table.size() does return 0, but iterating over the table does work
        // therefore we have to count ourselves.
        // TODO check if there is a better way or find out what's the issue with table.size()
        size_t tableFieldCount = 0u;
        table.for_each([&](const std::pair<sol::object, sol::object>& /*key_value*/) { ++tableFieldCount; });

        const EPropertyType propType = property.getType();
        if (propType == EPropertyType::Struct)
        {
            if (tableFieldCount != property.getChildCount())
            {
                sol_helper::throwSolException("Element size mismatch when assigning struct property '{}'! Expected: {} Received: {}",
                    property.getName(), property.getChildCount(), tableFieldCount);
                return;
            }

            for (auto& key_value : table)
            {
                const std::string_view rhsPropertyName = key_value.first.as<std::string_view>();
                Property* potentialChild = property.getChild(rhsPropertyName);
                if (!potentialChild)
                {
                    sol_helper::throwSolException("Unexpected property '{}' while assigning values to struct '{}'",
                        rhsPropertyName, property.getName());
                    return;
                }
                PropertyImpl& child = *potentialChild->m_impl;
                Set(child, key_value.second);
            }
        }
        else if (propType == EPropertyType::Array)
        {
            // TODO Violin refactor this with struct case - lots of similarities, only index type is different
            if (tableFieldCount != property.getChildCount())
            {
                sol_helper::throwSolException("Element size mismatch when assigning array property '{}'! Expected: {} Received: {}",
                    property.getName(), property.getChildCount(), tableFieldCount);
                return;
            }

            for (size_t i = 0; i < tableFieldCount; ++i)
            {
                const size_t luaIndex = i+1;
                const sol::object& solObject = table[luaIndex];
                if (solObject == sol::nil)
                {
                    sol_helper::throwSolException("Error during assignment of array property '{}'! Expected a value at index {}",
                        property.getName(), luaIndex);
                }
                Set(*property.getChild(i)->m_impl, solObject);
            }
        }
        else
        {
            // TODO Violin is there a better way for this?
            switch (propType)
            {
            case EPropertyType::Vec2f:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<float, 2>(table));
                break;
            case EPropertyType::Vec3f:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<float, 3>(table));
                break;
            case EPropertyType::Vec4f:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<float, 4>(table));
                break;
            case EPropertyType::Vec2i:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<int32_t, 2>(table));
                break;
            case EPropertyType::Vec3i:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<int32_t, 3>(table));
                break;
            case EPropertyType::Vec4i:
                property.setOutputValue_FromScript(LuaTypeConversions::ExtractArray<int32_t, 4>(table));
                break;
                // TODO Violin/Sven/Tobias this kind of a bad design, and the reason for it lies
                // with the fact that we handle 3 different things in the same base class - "Property"
                // Discuss whether we want this pattern, or maybe there are some other ideas how to deal
                // with type abstraction and polymorphy where we would not have this problem
            case EPropertyType::Float:
            case EPropertyType::Int32:
            case EPropertyType::String:
            case EPropertyType::Bool:
                // TODO Violin refactor this code, this should have failed earlier
                // TODO Violin test the other types too
                sol_helper::throwSolException("Assigning a table to property '{}' of type '{}'!", property.getName(), GetLuaPrimitiveTypeName(propType));
                break;
            case EPropertyType::Array:
            case EPropertyType::Struct:
                assert(false && "Should not have reached this code!");
            }
        }
    }

    void LuaScriptPropertySetter::SetString(PropertyImpl& property, std::string_view string)
    {
        if(property.getType() == EPropertyType::String)
        {
            property.setOutputValue_FromScript(std::string(string));
        }
        else
        {
            sol_helper::throwSolException("Assigning '{}' to string output '{}'!", GetLuaPrimitiveTypeName(property.getType()), property.getName());
        }
    }

    void LuaScriptPropertySetter::SetBool(PropertyImpl& property, bool boolean)
    {
        if (property.getType() == EPropertyType::Bool)
        {
            property.setOutputValue_FromScript(boolean);
        }
        else
        {
            sol_helper::throwSolException("Assigning boolean to '{}' output '{}' !", GetLuaPrimitiveTypeName(property.getType()), property.getName());
        }
    }

    void LuaScriptPropertySetter::SetStruct(PropertyImpl& property, LuaScriptPropertyHandler& structPropertyHandler)
    {
        const size_t childCount = property.getChildCount();
        for (size_t i = 0u; i < childCount; ++i)
        {
            PropertyImpl& child = *property.getChild(i)->m_impl;
            const sol::object rhs   = structPropertyHandler.getChildPropertyAsSolObject(child.getName());
            Set(child, rhs);
        }
    }

    void LuaScriptPropertySetter::SetArray(PropertyImpl& property, LuaScriptPropertyHandler& structPropertyHandler)
    {
        const PropertyImpl& rhsProperty = structPropertyHandler.getPropertyImpl();
        assert (rhsProperty.getType() == EPropertyType::Array);
        const size_t childCount = property.getChildCount();
        const size_t rhsChildCount = rhsProperty.getChildCount();

        if (rhsChildCount != childCount)
        {
            sol_helper::throwSolException("Element size mismatch when assigning array property '{}'! Expected: {} Received: {}",
                property.getName(),
                childCount,
                rhsChildCount);
        }

        for (size_t i = 0u; i < childCount; ++i)
        {
            PropertyImpl& child = *property.getChild(i)->m_impl;
            PropertyImpl& rhsChild = *rhsProperty.getChild(i)->m_impl;
            const EPropertyType expectedElementType = child.getType();
            const EPropertyType rhsElementType = rhsChild.getType();

            if(rhsElementType != expectedElementType)
            {
                sol_helper::throwSolException("Array element type mismatch (expected {} but received {})!",
                    GetLuaPrimitiveTypeName(expectedElementType),
                    GetLuaPrimitiveTypeName(rhsElementType));
            }

            // TODO violin we have too many switches like this one - needs some refactoring
            // Start at LuaScriptPropertySetter::Set(), follow all submethods, and unify these switch-cases (also check the other sol wrapper classes for similar code)
            // Idea: instead of checking the right hand side type, check the left-hand side and abort on mismatch
            // This way we should not have to delay type resolving further down the line
            switch (expectedElementType)
            {
            case EPropertyType::Bool:
            case EPropertyType::Float:
            case EPropertyType::Int32:
            case EPropertyType::String:
            case EPropertyType::Vec2f:
            case EPropertyType::Vec3f:
            case EPropertyType::Vec4f:
            case EPropertyType::Vec2i:
            case EPropertyType::Vec3i:
            case EPropertyType::Vec4i:
                child.setOutputValue_FromScript(rhsChild.getValue());
                break;
            case EPropertyType::Array:
                assert(false && "Array children can never be of type array themselves, that's handled during array declaration");
                break;
            case EPropertyType::Struct:
            {
                // TODO Violin this can be improved by refactoring the "handler" classes
                // For now, use the "LuaScriptPropertyHandler" class to assign structs, to reduce code duplication
                LuaScriptPropertyHandler childPropertyHandler(structPropertyHandler.getSolState(), rhsChild);
                SetStruct(child, childPropertyHandler);
                break;
            }
            }
        }
    }
}
