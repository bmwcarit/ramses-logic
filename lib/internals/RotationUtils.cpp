//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/RotationUtils.h"

#include <cassert>
#include <cmath>
#include <algorithm>

namespace rlogic::internal
{
    vec3f RotationUtils::QuaternionToEulerXYZDegrees(vec4f quaternion)
    {
        const float x = quaternion[0];
        const float y = quaternion[1];
        const float z = quaternion[2];
        const float w = quaternion[3];

        // Compute 3x3 rotation matrix as intermediate representation
        const float x2 = x + x;
        const float y2 = y + y;
        const float z2 = z + z;
        const float xx = x * x2;
        const float xy = x * y2;
        const float xz = x * z2;
        const float yy = y * y2;
        const float yz = y * z2;
        const float zz = z * z2;
        const float wx = w * x2;
        const float wy = w * y2;
        const float wz = w * z2;

        const float m11 = (1 - (yy + zz));
        const float m12 = (xy - wz);
        const float m22 = (1 - (xx + zz));
        const float m32 = (yz + wx);
        const float m13 = (xz + wy);
        const float m23 = (yz - wx);
        const float m33 = (1 - (xx + yy));

        // Compute euler XYZ angles from matrix values
        float eulerX = 0.f;
        const float eulerY = std::asin(std::clamp(m13, -1.f, 1.f));
        float eulerZ = 0.f;
        if (std::abs(m13) < 1.f)
        {
            eulerX = std::atan2(-m23, m33);
            eulerZ = std::atan2(-m12, m11);
        }
        else
        {
            eulerX = std::atan2(m32, m22);
        }

        return {
            Rad2Deg(eulerX),
            Rad2Deg(eulerY),
            Rad2Deg(eulerZ)
        };
    }

    std::optional<ERotationType> RotationUtils::RamsesRotationConventionToRotationType(ramses::ERotationConvention convention)
    {
        switch (convention)
        {
        case ramses::ERotationConvention::XYZ:
            return ERotationType::Euler_ZYX;
        case ramses::ERotationConvention::XZY:
            return ERotationType::Euler_YZX;
        case ramses::ERotationConvention::YXZ:
            return ERotationType::Euler_ZXY;
        case ramses::ERotationConvention::YZX:
            return ERotationType::Euler_XZY;
        case ramses::ERotationConvention::ZXY:
            return ERotationType::Euler_YXZ;
        case ramses::ERotationConvention::ZYX:
            return ERotationType::Euler_XYZ;
            //
        case ramses::ERotationConvention::XYX:
        case ramses::ERotationConvention::XZX:
        case ramses::ERotationConvention::YXY:
        case ramses::ERotationConvention::YZY:
        case ramses::ERotationConvention::ZXZ:
        case ramses::ERotationConvention::ZYZ:
            return std::nullopt;
        }

        return std::nullopt;
    }

    std::optional<ramses::ERotationConvention> RotationUtils::RotationTypeToRamsesRotationConvention(ERotationType rotationType)
    {
        switch (rotationType)
        {
        case ERotationType::Euler_ZYX:
            return ramses::ERotationConvention::XYZ;
        case ERotationType::Euler_YZX:
            return ramses::ERotationConvention::XZY;
        case ERotationType::Euler_ZXY:
            return ramses::ERotationConvention::YXZ;
        case ERotationType::Euler_XZY:
            return ramses::ERotationConvention::YZX;
        case ERotationType::Euler_YXZ:
            return ramses::ERotationConvention::ZXY;
        case ERotationType::Euler_XYZ:
            return ramses::ERotationConvention::ZYX;
        case ERotationType::Quaternion:
            // Ramses doesn't support native quaternions yet
            return std::nullopt;
        }

        return std::nullopt;
    }
}
