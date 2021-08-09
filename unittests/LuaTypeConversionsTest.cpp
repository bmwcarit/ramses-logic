//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "internals/LuaTypeConversions.h"

namespace rlogic::internal
{
    class TheLuaTypeConversions : public ::testing::Test
    {
        protected:
            sol::state m_sol;
    };

    TEST_F(TheLuaTypeConversions, ProvidesCorrectIndexUpperBoundsForVecTypes)
    {
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec2f), 2u);
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec3f), 3u);
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec4f), 4u);
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec2i), 2u);
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec3i), 3u);
        EXPECT_EQ(LuaTypeConversions::GetMaxIndexForVectorType(EPropertyType::Vec4i), 4u);
    }

    TEST_F(TheLuaTypeConversions, ExtractsStringViewFromSolObject)
    {
        m_sol["a_string"] = "string content";
        const sol::object solObject = m_sol["a_string"];

        const std::string_view asStringView = LuaTypeConversions::GetIndexAsString(solObject);
        EXPECT_EQ("string content", asStringView);
    }

    TEST_F(TheLuaTypeConversions, ThrowsExceptionWhenWrongTypeConvertedToString)
    {
        m_sol["not_a_string"] = 5;
        const sol::object solObject = m_sol["not_a_string"];

        std::string errorMsg;

        try
        {
            LuaTypeConversions::GetIndexAsString(solObject);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        EXPECT_EQ(errorMsg, std::string("lua: error: Only strings supported as table key type!"));
    }

    TEST_F(TheLuaTypeConversions, ExtractsSignedIntegers)
    {
        m_sol["positiveInt"] = 5;
        m_sol["negativeInt"] = -6;
        const sol::object positiveInt = m_sol["positiveInt"];
        const sol::object negativeInt = m_sol["negativeInt"];

        const std::optional<int32_t> positiveIntOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(positiveInt);
        const std::optional<int32_t> negativeIntOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(negativeInt);
        ASSERT_TRUE(positiveIntOpt);
        ASSERT_TRUE(negativeIntOpt);

        EXPECT_EQ(5, *positiveIntOpt);
        EXPECT_EQ(-6, *negativeIntOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsSignedIntegers_AllowsEpsilonRounding)
    {
        m_sol["positiveIntPlusEps"] = 5.0 + std::numeric_limits<double>::epsilon();
        m_sol["positiveIntMinusEps"] = 5.0 - std::numeric_limits<double>::epsilon();
        m_sol["negativeIntPlusEps"] = -6 + std::numeric_limits<double>::epsilon();
        m_sol["negativeIntMinusEps"] = -6 - std::numeric_limits<double>::epsilon();
        m_sol["zeroMinusEps"] = 0.0 - std::numeric_limits<double>::epsilon();
        const sol::object positiveIntPlusEps = m_sol["positiveIntPlusEps"];
        const sol::object positiveIntMinusEps = m_sol["positiveIntMinusEps"];
        const sol::object negativeIntPlusEps = m_sol["negativeIntPlusEps"];
        const sol::object negativeIntMinusEps = m_sol["negativeIntMinusEps"];
        const sol::object zeroMinusEps = m_sol["zeroMinusEps"];

        const std::optional<int32_t> positiveIntPlusEpsOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(positiveIntPlusEps);
        const std::optional<int32_t> positiveIntMinusEpsOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(positiveIntMinusEps);
        const std::optional<int32_t> negativeIntPlusEpsOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(negativeIntPlusEps);
        const std::optional<int32_t> negativeIntMinusEpsOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(negativeIntMinusEps);
        const std::optional<int32_t> zeroMinusEpsOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(zeroMinusEps);
        ASSERT_TRUE(positiveIntPlusEpsOpt);
        ASSERT_TRUE(positiveIntMinusEpsOpt);
        ASSERT_TRUE(negativeIntPlusEpsOpt);
        ASSERT_TRUE(negativeIntMinusEpsOpt);
        ASSERT_TRUE(zeroMinusEpsOpt);

        EXPECT_EQ(5, *positiveIntPlusEpsOpt);
        EXPECT_EQ(5, *positiveIntMinusEpsOpt);
        EXPECT_EQ(-6, *negativeIntPlusEpsOpt);
        EXPECT_EQ(-6, *negativeIntMinusEpsOpt);
        EXPECT_EQ(0, *zeroMinusEpsOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsSignedIntegers_ForbidsLargerThanEpsilonRounding)
    {
        m_sol["positiveIntPlusEps"] = 5.0 + 5*std::numeric_limits<double>::epsilon();
        m_sol["positiveIntMinusEps"] = 5.0 - 5*std::numeric_limits<double>::epsilon();
        m_sol["negativeIntPlusEps"] = -6 + 5*std::numeric_limits<double>::epsilon();
        m_sol["negativeIntMinusEps"] = -6 - 5 * std::numeric_limits<double>::epsilon();
        m_sol["zeroMinusEps"] = 0.0 - 5 * std::numeric_limits<double>::epsilon();
        const sol::object positiveIntPlusEps = m_sol["positiveIntPlusEps"];
        const sol::object positiveIntMinusEps = m_sol["positiveIntMinusEps"];
        const sol::object negativeIntPlusEps = m_sol["negativeIntPlusEps"];
        const sol::object negativeIntMinusEps = m_sol["negativeIntMinusEps"];
        const sol::object zeroMinusEps = m_sol["zeroMinusEps"];

        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(positiveIntPlusEps));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(positiveIntMinusEps));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(negativeIntPlusEps));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(negativeIntMinusEps));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(zeroMinusEps));
    }

    TEST_F(TheLuaTypeConversions, ExtractsUnsignedIntegers_AcceptsUpToEpsilonRounding)
    {
        m_sol["okRoundingPos"] = 5.0 + std::numeric_limits<double>::epsilon();
        m_sol["okRoundingNeg"] = 5.0 - std::numeric_limits<double>::epsilon();
        m_sol["zeroMinusEps"] = 0.0 - std::numeric_limits<double>::epsilon();
        m_sol["tooMuchRoundingPos"] = 5.0 + 5 * std::numeric_limits<double>::epsilon();
        m_sol["tooMuchRoundingNeg"] = 5.0 - 5 * std::numeric_limits<double>::epsilon();
        m_sol["zeroRoundingError"] = 0.0 - 5 * std::numeric_limits<double>::epsilon();
        const sol::object okRoundingPos = m_sol["okRoundingPos"];
        const sol::object okRoundingNeg = m_sol["okRoundingNeg"];
        const sol::object zeroMinusEps = m_sol["zeroMinusEps"];

        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(m_sol["tooMuchRoundingPos"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(m_sol["tooMuchRoundingNeg"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(m_sol["zeroRoundingError"]));
        const std::optional<size_t> okRoundingPosOpt = LuaTypeConversions::ExtractSpecificType<size_t>(okRoundingPos);
        const std::optional<size_t> okRoundingNegOpt = LuaTypeConversions::ExtractSpecificType<size_t>(okRoundingNeg);
        const std::optional<size_t> zeroMinusEpsOpt = LuaTypeConversions::ExtractSpecificType<size_t>(zeroMinusEps);

        ASSERT_TRUE(okRoundingPosOpt);
        ASSERT_TRUE(okRoundingNegOpt);
        ASSERT_TRUE(zeroMinusEpsOpt);

        EXPECT_EQ(5u, *okRoundingPosOpt);
        EXPECT_EQ(5u, *okRoundingNegOpt);
        EXPECT_EQ(0u, *zeroMinusEpsOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsUnsignedIntegers)
    {
        m_sol["uint"] = 5;
        const sol::object uint = m_sol["uint"];

        const std::optional<size_t> uintOpt = LuaTypeConversions::ExtractSpecificType<size_t>(uint);
        ASSERT_TRUE(uintOpt);

        EXPECT_EQ(5, *uintOpt);
    }

    TEST_F(TheLuaTypeConversions, CatchesErrorWhenCastingNegativeNumberToUnsignedInteger)
    {
        m_sol["negative"] = -5;

        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(m_sol["negative"]));
    }

    TEST_F(TheLuaTypeConversions, ExtractsUnsignedIntegers_AllowsEpsilonRounding)
    {
        m_sol["uint"] = 5.0 + std::numeric_limits<double>::epsilon();
        const sol::object uint = m_sol["uint"];

        const std::optional<size_t> uintOpt = LuaTypeConversions::ExtractSpecificType<size_t>(uint);
        ASSERT_TRUE(uintOpt);

        EXPECT_EQ(5, *uintOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsFloats)
    {
        m_sol["float"] = 0.5f;
        m_sol["negFloat"] = -0.5f;
        m_sol["floatWithIntegralPart"] = 1.5f;
        const sol::object float_ = m_sol["float"];
        const sol::object negFloat = m_sol["negFloat"];
        const sol::object floatWithIntegralPart = m_sol["floatWithIntegralPart"];

        const std::optional<float> floatOpt = LuaTypeConversions::ExtractSpecificType<float>(float_);
        const std::optional<float> negFloatOpt = LuaTypeConversions::ExtractSpecificType<float>(negFloat);
        const std::optional<float> floatWithIntegralPartOpt = LuaTypeConversions::ExtractSpecificType<float>(floatWithIntegralPart);
        ASSERT_TRUE(floatOpt);
        ASSERT_TRUE(negFloatOpt);
        ASSERT_TRUE(floatWithIntegralPartOpt);

        EXPECT_FLOAT_EQ(0.5f, *floatOpt);
        EXPECT_FLOAT_EQ(-0.5f, *negFloatOpt);
        EXPECT_FLOAT_EQ(1.5f, *floatWithIntegralPartOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsExtremeSignedIntegers)
    {
        constexpr int32_t maxIntValue = std::numeric_limits<int32_t>::max();
        constexpr int32_t lowestIntValue = std::numeric_limits<int32_t>::lowest();
        m_sol["maxInt"] = maxIntValue;
        m_sol["lowInt"] = lowestIntValue;
        const sol::object maxInt = m_sol["maxInt"];
        const sol::object lowInt = m_sol["lowInt"];

        const std::optional<int32_t> maxIntOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(maxInt);
        const std::optional<int32_t> lowIntOpt = LuaTypeConversions::ExtractSpecificType<int32_t>(lowInt);
        ASSERT_TRUE(maxIntOpt);
        ASSERT_TRUE(lowIntOpt);

        EXPECT_EQ(maxIntValue, *maxIntOpt);
        EXPECT_EQ(lowestIntValue, *lowIntOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsExtremeUnsignedIntegers)
    {
        // Maximum size_t is too large for Lua (throws exception as expected), use uint32_t instead
        constexpr size_t maxUIntValue = std::numeric_limits<uint32_t>::max();
        constexpr size_t lowestUIntValue = std::numeric_limits<uint32_t>::lowest();
        m_sol["maxUInt"] = maxUIntValue;
        m_sol["lowUInt"] = lowestUIntValue;
        const sol::object maxUInt = m_sol["maxUInt"];
        const sol::object lowUInt = m_sol["lowUInt"];

        const std::optional<size_t> maxUIntOpt = LuaTypeConversions::ExtractSpecificType<size_t>(maxUInt);
        const std::optional<size_t> lowUIntOpt = LuaTypeConversions::ExtractSpecificType<size_t>(lowUInt);
        ASSERT_TRUE(maxUIntOpt);
        ASSERT_TRUE(lowUIntOpt);

        EXPECT_EQ(maxUIntValue, *maxUIntOpt);
        EXPECT_EQ(lowestUIntValue, *lowUIntOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsExtremeFloats)
    {
        // Test numbers around the boundaries a) of the integral part and b) of the fractional part
        constexpr float maxFloatValue = std::numeric_limits<float>::max();
        constexpr float lowestFloatValue = std::numeric_limits<float>::lowest();
        constexpr float epsilonValue = std::numeric_limits<float>::epsilon();
        constexpr float negEpsilonValue = -std::numeric_limits<float>::epsilon();

        m_sol["maxFloat"] = maxFloatValue;
        m_sol["lowestFloat"] = lowestFloatValue;
        m_sol["epsilon"] = epsilonValue;
        m_sol["negEpsilon"] = negEpsilonValue;

        const sol::object maxFloat = m_sol["maxFloat"];
        const sol::object lowestFloat = m_sol["lowestFloat"];
        const sol::object epsilon = m_sol["epsilon"];
        const sol::object negEpsilon = m_sol["negEpsilon"];

        const std::optional<float> maxFloatOpt = LuaTypeConversions::ExtractSpecificType<float>(maxFloat);
        const std::optional<float> lowestFloatOpt = LuaTypeConversions::ExtractSpecificType<float>(lowestFloat);
        const std::optional<float> epsilonOpt = LuaTypeConversions::ExtractSpecificType<float>(epsilon);
        const std::optional<float> negEpsilonOpt = LuaTypeConversions::ExtractSpecificType<float>(negEpsilon);

        ASSERT_TRUE(maxFloatOpt);
        ASSERT_TRUE(lowestFloatOpt);
        ASSERT_TRUE(epsilonOpt);
        ASSERT_TRUE(negEpsilonOpt);

        EXPECT_FLOAT_EQ(maxFloatValue, *maxFloatOpt);
        EXPECT_FLOAT_EQ(lowestFloatValue, *lowestFloatOpt);
        EXPECT_FLOAT_EQ(epsilonValue, *epsilonOpt);
        EXPECT_FLOAT_EQ(negEpsilonValue, *negEpsilonOpt);
    }

    TEST_F(TheLuaTypeConversions, RoundsDoublesToFloats)
    {
        // Test numbers around the boundaries a) of the integral part and b) of the fractional part
        constexpr double dblEpsilon = std::numeric_limits<double>::epsilon() * 10;

        m_sol["onePlusEpsilon"] = 1.0 + dblEpsilon;
        m_sol["oneMinusEpsilon"] = 1.0 - dblEpsilon;

        const sol::object onePlusEpsilon = m_sol["onePlusEpsilon"];
        const sol::object oneMinusEpsilon = m_sol["oneMinusEpsilon"];

        const std::optional<float> onePlusEpsilonOpt = LuaTypeConversions::ExtractSpecificType<float>(onePlusEpsilon);
        const std::optional<float> oneMinusEpsilonOpt = LuaTypeConversions::ExtractSpecificType<float>(oneMinusEpsilon);

        ASSERT_TRUE(onePlusEpsilonOpt);
        ASSERT_TRUE(oneMinusEpsilonOpt);

        EXPECT_FLOAT_EQ(1.0f, *onePlusEpsilonOpt);
        EXPECT_FLOAT_EQ(1.0f, *oneMinusEpsilonOpt);
    }

    TEST_F(TheLuaTypeConversions, ExtractsTableOfFloatsToFloatArray)
    {
        m_sol.script(R"(
            floats = {0.1, 10000.42}
        )");

        const std::array<float, 2> floatArray = LuaTypeConversions::ExtractArray<float, 2>(m_sol["floats"]);

        EXPECT_FLOAT_EQ(0.1f, floatArray[0]);
        EXPECT_FLOAT_EQ(10000.42f, floatArray[1]);
    }

    TEST_F(TheLuaTypeConversions, ExtractsTableOfIntegersToSignedIntegerArray)
    {
        m_sol.script(R"(
            ints = {11, -12, (1.5 - 2.5)}
        )");

        const std::array<int32_t, 3> intsArray = LuaTypeConversions::ExtractArray<int32_t, 3>(m_sol["ints"]);

        EXPECT_EQ(11, intsArray[0]);
        EXPECT_EQ(-12, intsArray[1]);
        EXPECT_EQ(-1, intsArray[2]);
    }

    TEST_F(TheLuaTypeConversions, FailsValueExtractionWhenSymbolDoesNotExist)
    {
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(m_sol["noSuchSymbol"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<float>(m_sol["noSuchSymbol"]));
    }

    TEST_F(TheLuaTypeConversions, FailsValueExtractionWhenTypesDontMatch)
    {
        m_sol.script(R"(
            integer = 5
            aString = "string"
            aNil = nil
        )");

        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(m_sol["aString"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<float>(m_sol["aString"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(m_sol["aNil"]));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<float>(m_sol["aNil"]));
    }

    TEST_F(TheLuaTypeConversions, ThrowsExceptionWhenTableAndArraySizeDontMatch)
    {
        m_sol.script(R"(
            ints = {11, 12, 13, 14, 15}
        )");

        std::string errorMsg;

        try
        {
            LuaTypeConversions::ExtractArray<int32_t, 3>(m_sol["ints"]);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        // TODO Violin error message is not clear - fix this
        EXPECT_EQ(errorMsg, std::string("lua: error: Expected 3 array components in table but got 5 instead!"));
    }

    class TheLuaTypeConversions_CatchNumericErrors : public TheLuaTypeConversions
    {
    };

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, WhenNarrowingToSignedIntegers)
    {
        constexpr double largerThanMaxInt32Value = std::numeric_limits<int32_t>::max() + double(1.0);
        constexpr double smallerThanLowestInt32Value = std::numeric_limits<double>::lowest() - double(1.0);
        m_sol["largerThanMaxInt32"] = largerThanMaxInt32Value;
        m_sol["smallerThanLowestInt32"] = smallerThanLowestInt32Value;
        const sol::object largerThanMaxInt32 = m_sol["largerThanMaxInt32"];
        const sol::object smallerThanLowestInt32 = m_sol["smallerThanLowestInt32"];

        const std::optional<int32_t> largerThanMaxInt32Opt = LuaTypeConversions::ExtractSpecificType<int32_t>(largerThanMaxInt32);
        const std::optional<int32_t> smallerThanLowestInt32Opt = LuaTypeConversions::ExtractSpecificType<int32_t>(smallerThanLowestInt32);
        EXPECT_FALSE(largerThanMaxInt32Opt);
        EXPECT_FALSE(smallerThanLowestInt32Opt);
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, WhenNarrowingFloats)
    {
        // Adding is not enough, have to multiply to get out of float range
        constexpr double largerThanMaxFloatValue = std::numeric_limits<float>::max() * double(2.0);
        constexpr double smallerThanLowestFloatValue = std::numeric_limits<float>::lowest() * double(2.0);
        m_sol["largerThanMaxFloat"] = largerThanMaxFloatValue;
        m_sol["smallerThanLowestFloat"] = smallerThanLowestFloatValue;
        const sol::object largerThanMaxFloat = m_sol["largerThanMaxFloat"];
        const sol::object smallerThanLowestFloat = m_sol["smallerThanLowestFloat"];

        const std::optional<float> largerThanMaxFloatOpt = LuaTypeConversions::ExtractSpecificType<float>(largerThanMaxFloat);
        const std::optional<float> smallerThanLowestFloatOpt = LuaTypeConversions::ExtractSpecificType<float>(smallerThanLowestFloat);
        EXPECT_FALSE(largerThanMaxFloatOpt);
        EXPECT_FALSE(smallerThanLowestFloatOpt);
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, WhenNarrowingUnsignedIntegers)
    {
        // Adding is not enough, have to multiply to get out of range
        constexpr double largerThanMaxUIntValue = double(std::numeric_limits<size_t>::max()) * 2.0;
        m_sol["largerThanMaxUInt"] = largerThanMaxUIntValue;
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(m_sol["largerThanMaxUInt"]));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, WhenImplicitlyRoundingFloats)
    {
        // Combinations: positive and negative (X) with and without integral parts
        m_sol["float"] = 0.5f;
        m_sol["negFloat"] = -0.5f;
        m_sol["largerThanOneFloat"] = 1.5f;
        m_sol["smallerThanMinusOne"] = -1.5f;
        const sol::object float_ = m_sol["float"];
        const sol::object negFloat = m_sol["negFloat"];
        const sol::object largerThanOneFloat = m_sol["largerThanOneFloat"];
        const sol::object smallerThanMinusOne = m_sol["smallerThanMinusOne"];

        // Check signed and unsigned types alike, both should fail
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(float_));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(negFloat));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(largerThanOneFloat));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(smallerThanMinusOne));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(float_));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(negFloat));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(largerThanOneFloat));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(smallerThanMinusOne));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, WhenImplicitlyRoundingFloats_RoundingErrorLargerThanEpsilon)
    {
        // Test numbers around the boundaries a) of the integral part and b) of the fractional part
        constexpr double dblEpsilon = std::numeric_limits<double>::epsilon() * 2;

        m_sol["onePlusEpsilon"] = 1.0 + dblEpsilon;
        m_sol["oneMinusEpsilon"] = 1.0 - dblEpsilon;

        const sol::object onePlusEpsilon = m_sol["onePlusEpsilon"];
        const sol::object oneMinusEpsilon = m_sol["oneMinusEpsilon"];

        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(onePlusEpsilon));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<int32_t>(oneMinusEpsilon));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(onePlusEpsilon));
        EXPECT_FALSE(LuaTypeConversions::ExtractSpecificType<size_t>(oneMinusEpsilon));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, ThrowsExceptiion_WhenNarrowing_WhileExtractingIntegerArray)
    {
        m_sol["oneAboveLargestSignedInt"] = double(std::numeric_limits<int32_t>::max()) + 1;
        m_sol.script(R"(
            notOnlyInts = {11, 12, oneAboveLargestSignedInt}
        )");

        std::string errorMsg;

        try
        {
            LuaTypeConversions::ExtractArray<int32_t, 3>(m_sol["notOnlyInts"]);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        // TODO Violin error message is not clear - fix this
        EXPECT_EQ(errorMsg, std::string("lua: error: Unexpected value (type: 'number') at array element # 3!"));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, ThrowsException_WhenImplicitlyRoundingFloats_WhileExtractingIntegerArray)
    {
        m_sol.script(R"(
            notOnlyInts = {11, 12, 0.5}
        )");

        std::string errorMsg;

        try
        {
            LuaTypeConversions::ExtractArray<int32_t, 3>(m_sol["notOnlyInts"]);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        // TODO Violin error message is not clear - fix this
        EXPECT_EQ(errorMsg, std::string("lua: error: Unexpected value (type: 'number') at array element # 3!"));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, ThrowsException_WhenNegativeFloatFound_WhileExtractingIntegerArray)
    {
        m_sol.script(R"(
            notOnlyInts = {11, 12, -1.5}
        )");

        std::string errorMsg;

        try
        {
            LuaTypeConversions::ExtractArray<int32_t, 3>(m_sol["notOnlyInts"]);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        // TODO Violin error message is not clear - fix this
        EXPECT_EQ(errorMsg, std::string("lua: error: Unexpected value (type: 'number') at array element # 3!"));
    }

    TEST_F(TheLuaTypeConversions_CatchNumericErrors, ThrowsException_WhenNarrowing_WhileExtractingFloatArray)
    {
        constexpr double largerThanMaxFloatValue = std::numeric_limits<float>::max() * double(2.0);
        m_sol["largerThanMaxFloat"] = largerThanMaxFloatValue;
        m_sol.script(R"(
            tooLarge = {11, 12, largerThanMaxFloat}
        )");

        std::string errorMsg;

        try
        {
            LuaTypeConversions::ExtractArray<float, 3>(m_sol["tooLarge"]);
        }
        catch (sol::error const& err)
        {
            errorMsg = err.what();
        }

        // TODO Violin error message is not clear - fix this
        EXPECT_EQ(errorMsg, std::string("lua: error: Unexpected value (type: 'number') at array element # 3!"));
    }
}
