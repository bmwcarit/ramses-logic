//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/EPropertyType.h"

#include "generated/property_gen.h"

namespace rlogic::internal
{
    static EPropertyType ConvertSerializationTypeToEPropertyType(serialization::PropertyValue type)
    {
        switch (type)
        {
        case serialization::PropertyValue::bool_s:
            return EPropertyType::Bool;
        case serialization::PropertyValue::float_s:
            return EPropertyType::Float;
        case serialization::PropertyValue::vec2f_s:
            return EPropertyType::Vec2f;
        case serialization::PropertyValue::vec3f_s:
            return EPropertyType::Vec3f;
        case serialization::PropertyValue::vec4f_s:
            return EPropertyType::Vec4f;
        case serialization::PropertyValue::int32_s:
            return EPropertyType::Int32;
        case serialization::PropertyValue::vec2i_s:
            return EPropertyType::Vec2i;
        case serialization::PropertyValue::vec3i_s:
            return EPropertyType::Vec3i;
        case serialization::PropertyValue::vec4i_s:
            return EPropertyType::Vec4i;
        case serialization::PropertyValue::string_s:
            return EPropertyType::String;
        case serialization::PropertyValue::NONE:
            return EPropertyType::Struct;
        }
        return EPropertyType::Struct;
    }
}
