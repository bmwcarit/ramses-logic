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
#include "internals/SolState.h"
#include "internals/ArrayTypeInfo.h"

#include "fmt/format.h"
#include <algorithm>

namespace rlogic::internal
{
    LuaScriptPropertyExtractor::LuaScriptPropertyExtractor(SolState& state, PropertyImpl& propertyDescription)
        : LuaScriptHandler(state)
        , m_propertyDescription(propertyDescription)
    {
    }

    void LuaScriptPropertyExtractor::NewIndex(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& rhs)
    {
        data.addStructProperty(index, rhs, data.m_propertyDescription);
    }

    sol::object LuaScriptPropertyExtractor::Index(LuaScriptPropertyExtractor& data, const sol::object& index, const sol::object& /*rhs*/)
    {
        return data.getProperty(index);
    }

    sol::object LuaScriptPropertyExtractor::CreateArray(sol::this_state state, std::optional<size_t> size, std::optional<sol::object> arrayType)
    {
        if (!size)
        {
            sol_helper::throwSolException("ARRAY() invoked with invalid size parameter (must be the first parameter)!");
        }
        // TODO Violin/Sven/Tobias discuss max array size
        // Putting a "sane" number here, but maybe worth discussing again
        constexpr size_t MaxArraySize = 255;
        if (*size == 0u || *size > MaxArraySize)
        {
            sol_helper::throwSolException("ARRAY() invoked with invalid size parameter (must be in the range [1, {}])!", MaxArraySize);
        }
        if (!arrayType)
        {
            sol_helper::throwSolException("ARRAY() invoked with invalid type parameter (must be the second parameter)!");
        }
        return sol::object(state, sol::in_place_type<ArrayTypeInfo>, ArrayTypeInfo{*size, *arrayType});
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

    void LuaScriptPropertyExtractor::addStructProperty(const sol::object& propertyName, const sol::object& propertyValue, PropertyImpl& parentStruct)
    {
        const std::string_view name = GetIndexAsString(propertyName);

        if (nullptr != parentStruct.getChild(name))
        {
            sol_helper::throwSolException("Property '{}' already exists! Can't declare the same property twice!", name);
        }

        const auto solType = propertyValue.get_type();
        if (solType == sol::type::number)
        {
            const auto type = propertyValue.as<EPropertyType>();
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
                parentStruct.addChild(std::make_unique<PropertyImpl>(name, type, parentStruct.getInputOutputProperty()));
            }
            else
            {
                sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", name);
            }
        }
        else if (solType == sol::type::table)
        {
            const auto table          = propertyValue.as<sol::table>();
            auto propertyStruct = std::make_unique<PropertyImpl>(name, EPropertyType::Struct, parentStruct.getInputOutputProperty());

            for (auto& tableEntry : table)
            {
                addStructProperty(tableEntry.first, tableEntry.second, *propertyStruct);
            }

            parentStruct.addChild(std::move(propertyStruct));
        }
        else if (solType == sol::type::userdata)
        {
            if (propertyValue.is<ArrayTypeInfo>())
            {
                auto& arrayTypeInfo = propertyValue.as<ArrayTypeInfo>();
                const sol::object& arrayType = arrayTypeInfo.arrayType;

                const sol::type solArrayType = arrayType.get_type();
                if (solArrayType == sol::type::number)
                {
                    const auto type = arrayType.as<EPropertyType>();
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
                        auto arrayProperty = std::make_unique<PropertyImpl>(name, EPropertyType::Array, parentStruct.getInputOutputProperty());
                        for (size_t i = 0; i < arrayTypeInfo.arraySize; ++i)
                        {
                            arrayProperty->addChild(std::make_unique<PropertyImpl>("", type, parentStruct.getInputOutputProperty()));
                        }
                        parentStruct.addChild(std::move(arrayProperty));
                    }
                    else
                    {
                        sol_helper::throwSolException("Unsupported type id '{}' for array property '{}'!", static_cast<uint32_t>(type), name);
                    }
                }
                // TODO Violin/Sven here is the place where we should implement arrays of structs
                else
                {
                    sol_helper::throwSolException("Unsupported type '{}' for array property '{}'!", sol_helper::GetSolTypeName(solArrayType), name);
                }
            }
            else
            {
                sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", name);
            }
        }
        else
        {
            sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", name);
        }
    }
}
