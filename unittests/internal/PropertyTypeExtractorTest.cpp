//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "internals/PropertyTypeExtractor.h"

namespace rlogic::internal
{
    TEST(ThePropertyTypeExtractorGlobalSymbols, AreVisibleOnlyToSpecifiedEnvironment)
    {
        sol::state sol;
        sol::environment env(sol, sol::create, sol.globals());

        PropertyTypeExtractor::RegisterTypes(env);

        // Environment now has the type symbols (data types, declaration functions)
        EXPECT_TRUE(env["INT"].valid());
        EXPECT_TRUE(env["FLOAT"].valid());
        EXPECT_TRUE(env["VEC3F"].valid());
        EXPECT_TRUE(env["ARRAY"].valid());

        // Global lua state doesn't know these symbols
        EXPECT_FALSE(sol["INT"].valid());
        EXPECT_FALSE(sol["FLOAT"].valid());
        EXPECT_FALSE(sol["VEC3F"].valid());
        EXPECT_FALSE(sol["ARRAY"].valid());
    }

    class APropertyTypeExtractor : public ::testing::Test
    {
    protected:
        APropertyTypeExtractor()
            : m_env(m_sol, sol::create, m_sol.globals())
        {
            PropertyTypeExtractor::RegisterTypes(m_env);
        }

        HierarchicalTypeData extractTypeInfo(std::string_view source)
        {
            auto resultAndType = extractTypeInfo_WithResult(source);
            EXPECT_TRUE(resultAndType.second.valid());
            return resultAndType.first;
        }

        std::pair<HierarchicalTypeData, sol::protected_function_result>  extractTypeInfo_WithResult(std::string_view source)
        {
            return extractTypeInfo_ThroughEnvironment(source, m_env);
        }

        std::pair<HierarchicalTypeData, sol::protected_function_result> extractTypeInfo_ThroughEnvironment(std::string_view source, sol::environment& env)
        {
            // Reference temporary extractor
            PropertyTypeExtractor extractor("IN", EPropertyType::Struct);
            env["IN"] = std::ref(extractor);

            // Load script and apply environment
            sol::protected_function loaded = m_sol.load(source);
            assert(loaded.valid());
            env.set_on(loaded);

            // Execute script, nullify extractor reference, return results
            sol::protected_function_result result = loaded();
            env["IN"] = sol::lua_nil;
            return std::make_pair(extractor.getExtractedTypeData(), std::move(result));
        }

        sol::state m_sol;
        sol::environment m_env;
    };

    TEST_F(APropertyTypeExtractor, ExtractsSinglePrimitiveProperty)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo(R"(
            IN.newInt = INT
        )");

        const HierarchicalTypeData expected  = MakeStruct("IN", {{"newInt", EPropertyType::Int32}});

        EXPECT_EQ(typeInfo, expected);
    }

    TEST_F(APropertyTypeExtractor, ExtractsAllPrimitiveTypes_OrdersByPropertyNameLexicographically)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo(R"(
            IN.bool = BOOL
            IN.string = STRING
            IN.int32 = INT
            IN.int64 = INT64
            IN.vec2i = VEC2I
            IN.vec3i = VEC3I
            IN.vec4i = VEC4I
            IN.float = FLOAT
            IN.vec2f = VEC2F
            IN.vec3f = VEC3F
            IN.vec4f = VEC4F
        )");

        const HierarchicalTypeData expected = MakeStruct("IN",
            {
                {"bool", EPropertyType::Bool},
                {"float", EPropertyType::Float},
                {"int32", EPropertyType::Int32},
                {"int64", EPropertyType::Int64},
                {"string", EPropertyType::String},
                {"vec2f", EPropertyType::Vec2f},
                {"vec2i", EPropertyType::Vec2i},
                {"vec3f", EPropertyType::Vec3f},
                {"vec3i", EPropertyType::Vec3i},
                {"vec4f", EPropertyType::Vec4f},
                {"vec4i", EPropertyType::Vec4i},
            }
        );

        EXPECT_EQ(typeInfo, expected);
    }

    TEST_F(APropertyTypeExtractor, ExtractsNestedTypes_OrdersByPropertyNameLexicographically_WhenUsingLuaTable)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo(R"(
            IN.nested = {
                int = INT,
                vec4f = VEC4F,
                vec2i = VEC2I,
                bool = BOOL
            }
        )");

        const HierarchicalTypeData expected{
            TypeData{"IN", EPropertyType::Struct},
            {
                HierarchicalTypeData{
                TypeData{"nested", EPropertyType::Struct}, {
                    MakeType("bool", EPropertyType::Bool),
                    MakeType("int", EPropertyType::Int32),
                    MakeType("vec2i", EPropertyType::Vec2i),
                    MakeType("vec4f", EPropertyType::Vec4f)
                }
                }
            }
        };

        EXPECT_EQ(typeInfo, expected);
    }

    TEST_F(APropertyTypeExtractor, ExtractsNestedTypes_OrdersLexicographically_WhenDeclaredOneByOne)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo(R"(
            IN.nested = {}
            IN.nested.s2 = {}
            IN.nested.s2.i2 = INT
            IN.nested.s2.i1 = INT
            IN.nested.b1 = BOOL
        )");

        const HierarchicalTypeData expected {
            TypeData{"IN", EPropertyType::Struct},  // Root property
            {
                // Child properties
                HierarchicalTypeData{
                    TypeData{"nested", EPropertyType::Struct}, {
                        MakeType("b1", EPropertyType::Bool),
                        MakeStruct("s2", {
                            {"i1", EPropertyType::Int32},
                            {"i2", EPropertyType::Int32}
                            }),
                    }
                }
            }
        };

        EXPECT_EQ(typeInfo, expected);
    }

    class APropertyTypeExtractor_Errors : public APropertyTypeExtractor
    {
    protected:
        sol::error expectErrorDuringTypeExtraction(std::string_view luaCode)
        {
            const sol::protected_function_result result = extractTypeInfo_WithResult(luaCode).second;
            assert(!result.valid());
            sol::error error = result;
            return error;
        }
    };

    TEST_F(APropertyTypeExtractor_Errors, ProducesErrorWhenIndexingUndeclaredProperty)
    {
        const sol::error error = expectErrorDuringTypeExtraction("prop = IN.doesNotExist");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Field 'doesNotExist' does not exist in struct 'IN'"));
    }

    TEST_F(APropertyTypeExtractor_Errors, ProducesErrorWhenDeclaringPropertyTwice)
    {
        const sol::error error = expectErrorDuringTypeExtraction(
            R"(
                IN.property = INT
                IN.property = FLOAT
            )");

        EXPECT_THAT(error.what(), ::testing::HasSubstr("lua: error: Field 'property' already exists! Can't declare the same field twice!"));
    }

    TEST_F(APropertyTypeExtractor_Errors, ProducesErrorWhenTryingToAccessInterfaceProperties_WithNonStringIndex)
    {
        sol::error error = expectErrorDuringTypeExtraction("prop = IN[1]");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Bad index access to struct 'IN': Expected a string but got object of type number instead!"));
        error = expectErrorDuringTypeExtraction("prop = IN[true]");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Bad index access to struct 'IN': Expected a string but got object of type bool instead!"));
        error = expectErrorDuringTypeExtraction("prop = IN[{x=5}]");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Bad index access to struct 'IN': Expected a string but got object of type table instead!"));
        error = expectErrorDuringTypeExtraction("prop = IN[nil]");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Bad index access to struct 'IN': Expected a string but got object of type nil instead!"));
    }

    TEST_F(APropertyTypeExtractor_Errors, ProducesErrorWhenTryingToCreateInterfaceProperties_WithNonStringIndex)
    {
        sol::error error = expectErrorDuringTypeExtraction("IN[1] = INT");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Invalid index for new field on struct 'IN': Expected a string but got object of type number instead!"));
        error = expectErrorDuringTypeExtraction("IN[true] = INT");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Invalid index for new field on struct 'IN': Expected a string but got object of type bool instead!"));
        error = expectErrorDuringTypeExtraction("IN[{x=5}] = INT");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Invalid index for new field on struct 'IN': Expected a string but got object of type table instead!"));
        error = expectErrorDuringTypeExtraction("IN[nil] = INT");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Invalid index for new field on struct 'IN': Expected a string but got object of type nil instead!"));
    }

    TEST_F(APropertyTypeExtractor_Errors, InvalidTypeSpecifiers)
    {
        const std::vector<std::string> wrongStatements = {
            "IN.bad_type = nil",
            "IN.bad_type = 'not a type'",
            "IN.bad_type = true",
            "IN.bad_type = 150000",
        };

        for (const auto& statement : wrongStatements)
        {
            const sol::error error = expectErrorDuringTypeExtraction(statement);
            EXPECT_THAT(error.what(), ::testing::HasSubstr("Field 'bad_type' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!"));
        }
    }

    TEST_F(APropertyTypeExtractor_Errors, InvalidTypeSpecifiers_Nested)
    {
        const std::vector<std::string> wrongStatements = {
            "IN.parent = {bad_type = 'not a type'}",
            "IN.parent = {bad_type = true}",
            "IN.parent = {bad_type = 150000}",
        };

        for (const auto& statement : wrongStatements)
        {
            const sol::error error = expectErrorDuringTypeExtraction(statement);
            EXPECT_THAT(error.what(), ::testing::HasSubstr("Field 'bad_type' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!"));
        }
    }

    TEST_F(APropertyTypeExtractor_Errors, NoNameProvided_ForNestedProperty)
    {
        const sol::error error1 = expectErrorDuringTypeExtraction("IN.parent = {INT}");
        const sol::error error2 = expectErrorDuringTypeExtraction("IN.parent = {5}");
        EXPECT_THAT(error1.what(), ::testing::HasSubstr("Invalid index for new field on struct 'parent': Expected a string but got object of type number instead!"));
        EXPECT_THAT(error2.what(), ::testing::HasSubstr("Invalid index for new field on struct 'parent': Expected a string but got object of type number instead!"));
    }

    TEST_F(APropertyTypeExtractor_Errors, CorrectNameButWrongTypeProvided_ForNestedProperty)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.no_nested_type = { correct_key = 'but wrong type' }");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Field 'correct_key' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!"));
    }

    TEST_F(APropertyTypeExtractor_Errors, UserdataAssignedToPropertyCausesError)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.very_wrong = IN");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Field 'very_wrong' has invalid type! Only primitive types, arrays and nested tables obeying the same rules are supported!"));
    }

    class APropertyTypeExtractor_Arrays : public APropertyTypeExtractor
    {
    protected:
    };

    TEST_F(APropertyTypeExtractor_Arrays, DeclaresArrayOfPrimitives)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo("IN.primArray = ARRAY(3, INT)");

        const HierarchicalTypeData arrayType = MakeArray("primArray", 3, EPropertyType::Int32);
        const HierarchicalTypeData expected({"IN", EPropertyType::Struct}, {arrayType});

        EXPECT_EQ(typeInfo, expected);
    }

    TEST_F(APropertyTypeExtractor_Arrays, DeclaresArrayOfStructs)
    {
        const HierarchicalTypeData typeInfo = extractTypeInfo("IN.structArray = ARRAY(3, {a = INT, b = VEC3F})");

        EXPECT_EQ(1u, typeInfo.children.size());
        const HierarchicalTypeData& arrayType = typeInfo.children[0];

        EXPECT_EQ(arrayType.typeData, TypeData("structArray", EPropertyType::Array));

        for (const auto& arrayField : arrayType.children)
        {
            EXPECT_EQ(arrayField.typeData, TypeData("", EPropertyType::Struct));
            // Has all defined properties, but no particular ordering
            // TODO Violin would be probably a lot more robust to just order the fields based on e.g. lexicographic order, than to accept Lua behavior...
            EXPECT_THAT(arrayField.children,
                ::testing::UnorderedElementsAre(
                    MakeType("a", EPropertyType::Int32),
                    MakeType("b", EPropertyType::Vec3f)
                )
            );
        }

        // Order within a struct is arbitrary, BUT each two structs in the array have the exact same order of child properties!
        ASSERT_EQ(3u, arrayType.children.size());
        EXPECT_EQ(arrayType.children[0], arrayType.children[1]);
        EXPECT_EQ(arrayType.children[1], arrayType.children[2]);
    }

    class APropertyTypeExtractor_ArrayErrors : public APropertyTypeExtractor_Errors
    {
    protected:
    };

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayDefinedWithoutArguments)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY()");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with bad size argument! Error while extracting integer: expected a number, received 'nil'"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithFirstArgumentNotANumber)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY('not a number')");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with bad size argument! Error while extracting integer: expected a number, received 'string'"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithoutTypeArgument)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(5)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with invalid type parameter T!"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithInvalidTypeArgument)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(5, 9000)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Unsupported type id '9000' for array property 'array'!"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithZeroSize)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(0, INT)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with invalid size parameter N=0 (must be in the range [1, 255])!"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithOutOfBoundsSize)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(256, INT)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with invalid size parameter N=256 (must be in the range [1, 255])!"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithNegativeSize)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(-1, INT)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with bad size argument! Error while extracting integer: expected non-negative number, received '-1'"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithFloatSize)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(1.5, INT)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with bad size argument! Error while extracting integer: implicit rounding (fractional part '0.5' is not negligible)"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithUserDataInsteadOfSize)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(IN, INT)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("ARRAY(N, T) invoked with bad size argument! Error while extracting integer: expected a number, received 'userdata'"));
    }

    TEST_F(APropertyTypeExtractor_ArrayErrors, ArrayWithUserDataInsteadOfTypeInfo)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(5, IN)");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Unsupported type 'userdata' for array property 'array'!"));
    }

    // TODO Violin but should be - there is no reason not to support them
    TEST_F(APropertyTypeExtractor_ArrayErrors, MultidimensionalArraysAreNotSupported)
    {
        const sol::error error = expectErrorDuringTypeExtraction("IN.array = ARRAY(5, ARRAY(2, FLOAT))");
        EXPECT_THAT(error.what(), ::testing::HasSubstr("Unsupported type 'userdata' for array property 'array'!"));
    }
}
