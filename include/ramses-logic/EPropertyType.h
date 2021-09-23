//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include <array>
#include <string>
#include <cassert>

namespace rlogic
{
    /**
    * #EPropertyType lists the types of properties created and managed by the #rlogic::LogicNode
    * class and its derivates.
    */
    enum class EPropertyType : int
    {
        Float,  ///< corresponds to float
        Vec2f,  ///< corresponds to [float, float]
        Vec3f,  ///< corresponds to [float, float, float]
        Vec4f,  ///< corresponds to [float, float, float, float]
        Int32,  ///< corresponds to int32_t
        Vec2i,  ///< corresponds to [int32_t, int32_t]
        Vec3i,  ///< corresponds to [int32_t, int32_t, int32_t]
        Vec4i,  ///< corresponds to [int32_t, int32_t, int32_t, int32_t]
        Struct, ///< Has no value itself, but can have named child properties
        String, ///< corresponds to std::string
        Bool,   ///< corresponds to bool
        Array   ///< Has no value itself, but can have unnamed child properties of homogeneous types (primitive or structs)
    };

    using vec2f = std::array<float, 2>;
    using vec3f = std::array<float, 3>;
    using vec4f = std::array<float, 4>;
    using vec2i = std::array<int, 2>;
    using vec3i = std::array<int, 3>;
    using vec4i = std::array<int, 4>;

    /**
    * Type trait which converts C++ types to #rlogic::EPropertyType enum for primitive types.
    */
    template <typename T> struct PropertyTypeToEnum;

    template <> struct PropertyTypeToEnum<float>
    {
        static const EPropertyType TYPE = EPropertyType::Float;
    };

    template <> struct PropertyTypeToEnum<vec2f>
    {
        static const EPropertyType TYPE = EPropertyType::Vec2f;
    };

    template <> struct PropertyTypeToEnum<vec3f>
    {
        static const EPropertyType TYPE = EPropertyType::Vec3f;
    };

    template <> struct PropertyTypeToEnum<vec4f>
    {
        static const EPropertyType TYPE = EPropertyType::Vec4f;
    };

    template <> struct PropertyTypeToEnum<int>
    {
        static const EPropertyType TYPE = EPropertyType::Int32;
    };

    template <> struct PropertyTypeToEnum<vec2i>
    {
        static const EPropertyType TYPE = EPropertyType::Vec2i;
    };

    template <> struct PropertyTypeToEnum<vec3i>
    {
        static const EPropertyType TYPE = EPropertyType::Vec3i;
    };

    template <> struct PropertyTypeToEnum<vec4i>
    {
        static const EPropertyType TYPE = EPropertyType::Vec4i;
    };

    template <> struct PropertyTypeToEnum<std::string>
    {
        static const EPropertyType TYPE = EPropertyType::String;
    };

    template <> struct PropertyTypeToEnum<bool>
    {
        static const EPropertyType TYPE = EPropertyType::Bool;
    };

    /**
    * Type trait which can be used to check if a type is primitive or not.
    * "primitive" in this context means the type can be used as a template for
    * #rlogic::Property::set() and #rlogic::Property::get(), i.e. it has a
    * value which can be directly set/obtained, as opposed to non-primitive types
    * like structs or arrays which don't have a singular settable value.
    */
    template <typename T> struct IsPrimitiveProperty
    {
        static const bool value = false;
    };

    template <> struct IsPrimitiveProperty<float>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec2f>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec3f>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec4f>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<int>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec2i>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec3i>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<vec4i>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<std::string>
    {
        static const bool value = true;
    };

    template <> struct IsPrimitiveProperty<bool>
    {
        static const bool value = true;
    };

    /**
    * Helper to determine if given property type can be stored in a #rlogic::DataArray.
    */
    constexpr bool CanPropertyTypeBeStoredInDataArray(EPropertyType type)
    {
        switch (type)
        {
        case EPropertyType::Float:
        case EPropertyType::Vec2f:
        case EPropertyType::Vec3f:
        case EPropertyType::Vec4f:
        case EPropertyType::Int32:
        case EPropertyType::Vec2i:
        case EPropertyType::Vec3i:
        case EPropertyType::Vec4i:
            return true;
        case EPropertyType::Bool:
        case EPropertyType::Struct:
        case EPropertyType::String:
        case EPropertyType::Array:
            return false;
        }

        return false;
    }

    /**
    * Helper to determine if given property type can be animated using #rlogic::AnimationNode.
    */
    constexpr bool CanPropertyTypeBeAnimated(EPropertyType type)
    {
        // currently all types that can be stored in DataArray
        return CanPropertyTypeBeStoredInDataArray(type);
    }

    /**
    * Returns the string representation of a property type. This string corresponds to the syntax
    * that has to be used in the Lua source code used to create scripts with properties with
    * the corresponding type.
    */
    constexpr const char* GetLuaPrimitiveTypeName(EPropertyType type)
    {
        switch (type)
        {
        case EPropertyType::Float:
            return "FLOAT";
        case EPropertyType::Vec2f:
            return "VEC2F";
        case EPropertyType::Vec3f:
            return "VEC3F";
        case EPropertyType::Vec4f:
            return "VEC4F";
        case EPropertyType::Int32:
            return "INT";
        case EPropertyType::Vec2i:
            return "VEC2I";
        case EPropertyType::Vec3i:
            return "VEC3I";
        case EPropertyType::Vec4i:
            return "VEC4I";
        case EPropertyType::Struct:
            return "STRUCT";
        case EPropertyType::String:
            return "STRING";
        case EPropertyType::Bool:
            return "BOOL";
        case EPropertyType::Array:
            return "ARRAY";
        }
        return "STRUCT";
    }
}
