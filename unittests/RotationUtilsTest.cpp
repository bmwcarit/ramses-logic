//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"

#include "ramses-client-api/Node.h"
#include "internals/RotationUtils.h"

namespace rlogic::internal
{
    class TheRotationUtils : public ::testing::Test
    {
    protected:
    };

    TEST_F(TheRotationUtils, ConvertRotationTypeEnumsInBothDirections)
    {
        const std::vector<std::pair<ERotationType, ramses::ERotationConvention>> enumPairs =
        {
            {ERotationType::Euler_ZYX, ramses::ERotationConvention::XYZ},
            {ERotationType::Euler_YZX, ramses::ERotationConvention::XZY},
            {ERotationType::Euler_ZXY, ramses::ERotationConvention::YXZ},
            {ERotationType::Euler_XZY, ramses::ERotationConvention::YZX},
            {ERotationType::Euler_YXZ, ramses::ERotationConvention::ZXY},
            {ERotationType::Euler_XYZ, ramses::ERotationConvention::ZYX},
        };

        for (const auto& [logicEnum, ramsesEnum] : enumPairs)
        {
            std::optional<ramses::ERotationConvention> convertedRamsesEnum = RotationUtils::RotationTypeToRamsesRotationConvention(logicEnum);
            EXPECT_TRUE(convertedRamsesEnum);
            EXPECT_EQ(ramsesEnum, *convertedRamsesEnum);

            std::optional<ERotationType> convertedLogicEnum = RotationUtils::RamsesRotationConventionToRotationType(ramsesEnum);
            EXPECT_TRUE(convertedLogicEnum);
            EXPECT_EQ(logicEnum, *convertedLogicEnum);
        }

        EXPECT_FALSE(RotationUtils::RotationTypeToRamsesRotationConvention(ERotationType::Quaternion));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::XYX));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::XZX));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::YXY));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::YZY));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::ZXZ));
        EXPECT_FALSE(RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention::ZYZ));
    }

    class TheRotationUtils_Quaternions : public TheRotationUtils
    {
    protected:
    };

    TEST_F(TheRotationUtils_Quaternions, ConvertsToEuler_SingleAxis)
    {
        std::vector<std::pair<vec4f, vec3f>> testAngles = {
            // 45 degrees around single axis (X, Y, Z)
            {{0.3826834f, 0, 0, 0.9238795f }, { 45.f, 0.f, 0.f }},
            {{0, 0.3826834f, 0, 0.9238795f}, {0.f, 45.f, 0.f}},
            {{0, 0, 0.3826834f, 0.9238795f}, { 0.f, 0.f, 45.f }},

            // 90 degrees around single axis (X, Y, Z)
            {{0.7071068f, 0, 0, 0.7071068f }, { 90.f, 0.f, 0.f }},
            {{0, 0.7071068f, 0, 0.7071068f}, { 0.f, 90.f, 0.f }},
            {{0, 0, 0.7071068f, 0.7071068f}, { 0.f, 0.f, 90.f }},
        };

        for (const auto& [quat, euler] : testAngles)
        {
            const vec3f convertedEuler = RotationUtils::QuaternionToEulerXYZDegrees(quat);

            EXPECT_FLOAT_EQ(convertedEuler[0], euler[0]);
            EXPECT_FLOAT_EQ(convertedEuler[1], euler[1]);
            EXPECT_FLOAT_EQ(convertedEuler[2], euler[2]);
        }
    }

    TEST_F(TheRotationUtils_Quaternions, ConvertsToEulerXYZ_MultipleAxesCombinations)
    {
        std::vector<std::pair<vec4f, vec3f>> testAngles = {
            // 135 degrees around single axis (X, Y, Z)
            {{0.9238795f, 0, 0, 0.3826834f}, { 135.f, 0.f, 0.f }},
            {{0, 0.9238795f, 0, 0.3826834f}, { -180.f, 45.f, -180.f }}, // Gimbal lock, is equivalent to (0, +135, 0) rotation
            {{0, 0, 0.9238795f, 0.3826834f}, { 0.f, 0.f, 135.f }},
            // 90 degrees, 2 axes
            {{0.5f, -0.5f, 0.5f, 0.5f }, { 90.f, 0.f, 90.f }},
            {{0.5f, 0.5f, 0.5f, 0.5f }, { 90.f, 90.f, 0.f }},
            // More exotic combinations
            {{0.2317316f, 0.5668337f, 0.2478199f, 0.7507232f}, { 15.f, 75.f, 25.f }},
            {{0.2705981f, -0.2705981f, 0.6532815f, 0.6532815f }, { 45.f, 0.f, 90.f }},
            {{0.25f, 0.0669873f, 0.9330127f, 0.25f }, { 0.f, 30.f, 150.f }},
        };

        for (const auto& [quat, euler] : testAngles)
        {
            const vec3f convertedEuler = RotationUtils::QuaternionToEulerXYZDegrees(quat);

            EXPECT_LE(std::abs(convertedEuler[0] - euler[0]), RotationUtils::ConversionPrecision);
            EXPECT_LE(std::abs(convertedEuler[1] - euler[1]), RotationUtils::ConversionPrecision);
            EXPECT_LE(std::abs(convertedEuler[2] - euler[2]), RotationUtils::ConversionPrecision);
        }
    }

    // Conversion from Euler to Quats is not reverse-mappable because of Gimbal lock. This is a fundamental property of Euler angles
    // This test ensures that the angles are correct, despite being different than the original Euler angles.
    TEST_F(TheRotationUtils_Quaternions, TestGimbalLockConversionCases)
    {
        std::vector<std::pair<vec4f, vec3f>> anglePairs = {
            {{ 0, 0.7660444f, 0, 0.6427876f },                  {-180.f, 80.f, -180.f}},    // 0, 100, 0
            {{ 0, -0.7660444f, 0, 0.6427876f },                 {180.f, -80.f, 180.f}},     // 0, -100, 0
            {{ 0, 0.7071068f, 0, -0.7071068f },                 {0.f, -90.f, 0.f}},         // 0, 270, 0
            {{ 0.4545195f, 0.5416752f, 0.5416752f, 0.4545195f },{-90.f, 80.f, -180.f}},     // 90, 100, 0
            {{ 0.5868241f, 0.4924039f, 0.4924039f, 0.4131759f },{-180.f, 80.f, -80.f}},     // 0, 100, 100
        };

        for (const auto& [quat, expectedEuler] : anglePairs)
        {
            vec3f convertedEuler = RotationUtils::QuaternionToEulerXYZDegrees(quat);

            EXPECT_LE(std::abs(convertedEuler[0] - expectedEuler[0]), RotationUtils::ConversionPrecision);
            EXPECT_LE(std::abs(convertedEuler[1] - expectedEuler[1]), RotationUtils::ConversionPrecision);
            EXPECT_LE(std::abs(convertedEuler[2] - expectedEuler[2]), RotationUtils::ConversionPrecision);
        }
    }
}
