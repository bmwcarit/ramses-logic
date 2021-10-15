//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/PropertyTypeExtractor.h"
#include "internals/SolHelper.h"
#include "internals/TypeUtils.h"
#include "internals/ArrayTypeInfo.h"
#include "internals/LuaTypeConversions.h"

#include "fmt/format.h"
#include <algorithm>

namespace rlogic::internal
{
    PropertyTypeExtractor::PropertyTypeExtractor(std::string rootName, EPropertyType rootType)
        : m_typeData(std::move(rootName), rootType)
    {
    }

    std::reference_wrapper<PropertyTypeExtractor> PropertyTypeExtractor::index(const sol::object& propertyName)
    {
        const std::string_view childName = LuaTypeConversions::GetIndexAsString(propertyName);
        auto childIter = findChild(childName);
        if (childIter == m_children.end())
        {
            sol_helper::throwSolException("Trying to access not available property {} in interface!", childName);
        }

        PropertyTypeExtractor& refToChild = *childIter;
        return refToChild;
    }

    void PropertyTypeExtractor::newIndex(const sol::object& propertyName, const sol::object& propertyValue)
    {
        const std::string_view childName = LuaTypeConversions::GetIndexAsString(propertyName);
        auto childIter = findChild(childName);
        if (childIter != m_children.end())
        {
            sol_helper::throwSolException("Property '{}' already exists! Can't declare the same property twice!", childName);
        }

        // TODO Violin improve error messages below (more specific errors instead of generic 'wrong type' error)
        const auto solType = propertyValue.get_type();
        if (solType == sol::type::number)
        {
            const auto type = propertyValue.as<EPropertyType>();
            if (TypeUtils::IsValidType(type) && TypeUtils::IsPrimitiveType(type))
            {
                m_children.emplace_back(PropertyTypeExtractor{ std::string(childName), type });
            }
            else
            {
                sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", childName);
            }
        }
        else if (solType == sol::type::table)
        {
            PropertyTypeExtractor structProperty(std::string(childName), EPropertyType::Struct);
            structProperty.extractPropertiesFromTable(propertyValue.as<sol::table>());
            m_children.emplace_back(std::move(structProperty));
        }
        else if (solType == sol::type::userdata)
        {
            const sol::optional<ArrayTypeInfo> potentiallyArrayTypeInfo = propertyValue.as<sol::optional<ArrayTypeInfo>>();
            if (potentiallyArrayTypeInfo)
            {
                const ArrayTypeInfo& arrayTypeInfo = *potentiallyArrayTypeInfo;
                const sol::object& arrayType = arrayTypeInfo.arrayType;

                PropertyTypeExtractor arrayProperty(std::string(childName), EPropertyType::Array);

                const sol::type solArrayType = arrayType.get_type();
                // Handles ARRAY(n, T) where T is a primitive type (int, float etc.)
                if (solArrayType == sol::type::number)
                {
                    const auto type = arrayType.as<EPropertyType>();
                    if (TypeUtils::IsValidType(type) && TypeUtils::IsPrimitiveType(type))
                    {
                        arrayProperty.m_children.resize(arrayTypeInfo.arraySize, PropertyTypeExtractor("", type));
                    }
                    else
                    {
                        sol_helper::throwSolException("Unsupported type id '{}' for array property '{}'!", static_cast<uint32_t>(type), childName);
                    }
                }
                // Handles ARRAY(n, T) where T is a complex type (only structs currently supported)
                else if (solArrayType == sol::type::table)
                {
                    PropertyTypeExtractor structInArray("", EPropertyType::Struct);
                    structInArray.extractPropertiesFromTable(arrayType.as<sol::table>());
                    arrayProperty.m_children.resize(arrayTypeInfo.arraySize, structInArray);
                }
                // TODO Violin consider whether we should add support for nested arrays. Should be easy to implement, and would be more consistent for users
                else
                {
                    sol_helper::throwSolException("Unsupported type '{}' for array property '{}'!", sol_helper::GetSolTypeName(solArrayType), childName);
                }

                m_children.emplace_back(std::move(arrayProperty));
            }
            else
            {
                sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", childName);
            }
        }
        else
        {
            sol_helper::throwSolException("Field '{}' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!", childName);
        }
    }

    PropertyTypeExtractor::ChildProperties::iterator PropertyTypeExtractor::findChild(const std::string_view name)
    {
        return std::find_if(m_children.begin(), m_children.end(), [&name](const PropertyTypeExtractor& child) {return child.m_typeData.name == name; });
    }

    void PropertyTypeExtractor::extractPropertiesFromTable(const sol::table& table)
    {
        for (auto& tableEntry : table)
        {
            newIndex(tableEntry.first, tableEntry.second);
        }
    }

    sol::object PropertyTypeExtractor::CreateArray(sol::this_state state, const sol::object& size, std::optional<sol::object> arrayType)
    {
        const std::optional<size_t> potentialUInt = LuaTypeConversions::ExtractSpecificType<size_t>(size);
        if (!potentialUInt)
        {
            sol_helper::throwSolException("ARRAY(N, T) invoked with size parameter N which is not a positive integer!");
        }
        // TODO Violin/Sven/Tobias discuss max array size
        // Putting a "sane" number here, but maybe worth discussing again
        const size_t arraySize = *potentialUInt;
        constexpr size_t MaxArraySize = 255;
        if (arraySize == 0u || arraySize > MaxArraySize)
        {
            sol_helper::throwSolException("ARRAY(N, T) invoked with invalid size parameter N={} (must be in the range [1, {}])!", arraySize, MaxArraySize);
        }
        if (!arrayType)
        {
            sol_helper::throwSolException("ARRAY(N, T) invoked with invalid type parameter T!");
        }
        return sol::object(state, sol::in_place_type<ArrayTypeInfo>, ArrayTypeInfo{arraySize, *arrayType});
    }

    void PropertyTypeExtractor::RegisterTypes(sol::environment& environment)
    {
        environment.new_usertype<ArrayTypeInfo>("ArrayTypeInfo");
        environment.new_usertype<PropertyTypeExtractor>("LuaScriptPropertyExtractor",
            sol::meta_method::new_index, &PropertyTypeExtractor::newIndex,
            sol::meta_method::index, &PropertyTypeExtractor::index);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Float)] = static_cast<int>(EPropertyType::Float);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec2f)] = static_cast<int>(EPropertyType::Vec2f);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec3f)] = static_cast<int>(EPropertyType::Vec3f);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec4f)] = static_cast<int>(EPropertyType::Vec4f);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Int32)] = static_cast<int>(EPropertyType::Int32);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec2i)] = static_cast<int>(EPropertyType::Vec2i);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec3i)] = static_cast<int>(EPropertyType::Vec3i);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Vec4i)] = static_cast<int>(EPropertyType::Vec4i);
        environment[GetLuaPrimitiveTypeName(EPropertyType::String)] = static_cast<int>(EPropertyType::String);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Bool)] = static_cast<int>(EPropertyType::Bool);
        environment[GetLuaPrimitiveTypeName(EPropertyType::Struct)] = static_cast<int>(EPropertyType::Struct);
        environment.set_function(GetLuaPrimitiveTypeName(EPropertyType::Array), &PropertyTypeExtractor::CreateArray);
    }

    HierarchicalTypeData PropertyTypeExtractor::getExtractedTypeData() const
    {
        // extract children type info first
        std::vector<HierarchicalTypeData> children;
        children.reserve(m_children.size());
        std::transform(m_children.begin(), m_children.end(), std::back_inserter(children),
            [](const PropertyTypeExtractor& childExtractor) { return childExtractor.getExtractedTypeData(); });

        // sort struct children by name lexicographically
        if (m_typeData.type == EPropertyType::Struct)
        {
            std::sort(children.begin(), children.end(), [](const HierarchicalTypeData& child1, const HierarchicalTypeData& child2)
                {
                    assert(!child1.typeData.name.empty() && !child2.typeData.name.empty());
                    return child1.typeData.name < child2.typeData.name;
                });
        }

        return HierarchicalTypeData(m_typeData, std::move(children));
    }

}
