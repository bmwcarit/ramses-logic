//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"

namespace rlogic
{
    /**
    * #ERotationType lists the types of rotation conventions used to tell #rlogic::RamsesNodeBinding how to
    * compute rotation matrices. It is similar to ramses::ERotationConvention except that it also allows quaternions.
    * The rotation types are to be interpreted like this:
    * - All rotations are right-handed, Ramses supports only right-handed rotation math which the Logic Engine only wraps
    * - All values are expected as degrees, _not_ radians (except Quaternions)
    * - Euler_ABC enum value corresponds to ramses::ERotationConvention::CBA
    *       - note the inverted order of the axes! This is because Ramses has a different notation of the enum
    * - Euler_ABC will rotate around axis A, B, then C in this exact order
    * - Quaternion will apply standard quaternion math. Quaternions are expected to be normalized
    */
    enum class ERotationType : int
    {
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::XYZ
        Euler_ZYX,
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::XZY
        Euler_YZX,
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::YXZ
        Euler_ZXY,
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::YZX
        Euler_XZY,
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::ZXY
        Euler_YXZ,
        /// Passes its vec3f values to Ramses nodes as ramses::ERotationConvention::ZYX
        Euler_XYZ,
        /// Converts its vec4f (Quaternion) to a Euler rotation with X-Y-Z ordering and passes to ramses as ramses::ERotationConvention::ZYX
        Quaternion,
    };
}
