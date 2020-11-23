//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-logic/EPropertyType.h"

namespace rlogic
{
    TEST(PropertyTypeToEnumTypeTrait, ConvertsSupportedTypesToCorrectEnum)
    {
        EPropertyType floatType = PropertyTypeToEnum<float>::TYPE;
        EPropertyType vec2fType = PropertyTypeToEnum<vec2f>::TYPE;
        EPropertyType vec3fType = PropertyTypeToEnum<vec3f>::TYPE;
        EPropertyType vec4fType = PropertyTypeToEnum<vec4f>::TYPE;
        EPropertyType intType = PropertyTypeToEnum<int>::TYPE;
        EPropertyType vec2iType = PropertyTypeToEnum<vec2i>::TYPE;
        EPropertyType vec3iType = PropertyTypeToEnum<vec3i>::TYPE;
        EPropertyType vec4iType = PropertyTypeToEnum<vec4i>::TYPE;
        EPropertyType boolType = PropertyTypeToEnum<bool>::TYPE;
        EPropertyType stringType = PropertyTypeToEnum<std::string>::TYPE;
        EXPECT_EQ(floatType, EPropertyType::Float);
        EXPECT_EQ(vec2fType, EPropertyType::Vec2f);
        EXPECT_EQ(vec3fType, EPropertyType::Vec3f);
        EXPECT_EQ(vec4fType, EPropertyType::Vec4f);
        EXPECT_EQ(intType, EPropertyType::Int32);
        EXPECT_EQ(vec2iType, EPropertyType::Vec2i);
        EXPECT_EQ(vec3iType, EPropertyType::Vec3i);
        EXPECT_EQ(vec4iType, EPropertyType::Vec4i);
        EXPECT_EQ(boolType, EPropertyType::Bool);
        EXPECT_EQ(stringType, EPropertyType::String);
    }

    TEST(IsPrimitivePropertyTypeTrait, IsTrueOnlyForPrimitiveProperties)
    {
        bool floatIsPrimitive = IsPrimitiveProperty<float>::value;
        bool vec2fIsPrimitive = IsPrimitiveProperty<vec2f>::value;
        bool vec3fIsPrimitive = IsPrimitiveProperty<vec3f>::value;
        bool vec4fIsPrimitive = IsPrimitiveProperty<vec4f>::value;
        bool intIsPrimitive = IsPrimitiveProperty<int>::value;
        bool vec2iIsPrimitive = IsPrimitiveProperty<vec2i>::value;
        bool vec3iIsPrimitive = IsPrimitiveProperty<vec3i>::value;
        bool vec4iIsPrimitive = IsPrimitiveProperty<vec4i>::value;
        bool boolIsPrimitive = IsPrimitiveProperty<bool>::value;
        bool stringIsPrimitive = IsPrimitiveProperty<std::string>::value;
        bool unsupportedTypeIsPrimitive = IsPrimitiveProperty<size_t>::value;
        bool complexTypeIsPrimitive = IsPrimitiveProperty<std::vector<float>>::value;

        EXPECT_TRUE(floatIsPrimitive);
        EXPECT_TRUE(vec2fIsPrimitive);
        EXPECT_TRUE(vec3fIsPrimitive);
        EXPECT_TRUE(vec4fIsPrimitive);
        EXPECT_TRUE(intIsPrimitive);
        EXPECT_TRUE(vec2iIsPrimitive);
        EXPECT_TRUE(vec3iIsPrimitive);
        EXPECT_TRUE(vec4iIsPrimitive);
        EXPECT_TRUE(boolIsPrimitive);
        EXPECT_TRUE(stringIsPrimitive);

        EXPECT_FALSE(unsupportedTypeIsPrimitive);
        EXPECT_FALSE(complexTypeIsPrimitive);
    }

    TEST(GetLuaPrimitiveTypeNameFunction, ProvidesNameForSupportedTypeEnumValues)
    {
        EXPECT_STREQ("FLOAT", GetLuaPrimitiveTypeName(EPropertyType::Float));
        EXPECT_STREQ("VEC2F", GetLuaPrimitiveTypeName(EPropertyType::Vec2f));
        EXPECT_STREQ("VEC3F", GetLuaPrimitiveTypeName(EPropertyType::Vec3f));
        EXPECT_STREQ("VEC4F", GetLuaPrimitiveTypeName(EPropertyType::Vec4f));
        EXPECT_STREQ("INT", GetLuaPrimitiveTypeName(EPropertyType::Int32));
        EXPECT_STREQ("VEC2I", GetLuaPrimitiveTypeName(EPropertyType::Vec2i));
        EXPECT_STREQ("VEC3I", GetLuaPrimitiveTypeName(EPropertyType::Vec3i));
        EXPECT_STREQ("VEC4I", GetLuaPrimitiveTypeName(EPropertyType::Vec4i));
        EXPECT_STREQ("BOOL", GetLuaPrimitiveTypeName(EPropertyType::Bool));
        EXPECT_STREQ("STRING", GetLuaPrimitiveTypeName(EPropertyType::String));
        EXPECT_STREQ("STRUCT", GetLuaPrimitiveTypeName(EPropertyType::Struct));
        EXPECT_STREQ("ARRAY", GetLuaPrimitiveTypeName(EPropertyType::Array));
    }
}
