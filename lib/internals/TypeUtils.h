//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/Property.h"
#include "impl/PropertyImpl.h"
#include <cassert>
#include <vector>

namespace rlogic::internal
{
    class TypeUtils
    {
    public:
        static bool IsPrimitiveType(EPropertyType type)
        {
            return (type != EPropertyType::Array && type != EPropertyType::Struct);
        }

        // This method is for better readability in code
        static bool CanHaveChildren(EPropertyType type)
        {
            return !IsPrimitiveType(type);
        }

        // Makes {x, y, z, w...} out of {{x, y}, {z, w}, ...}
        // This is required so that array data can be passed to ramses arrays
        // RAMSESTYPE: float/uint32_t (base type used by all arrays in ramses)
        // LOGICTYPE: float, vec2f, vec3i, ...
        template <typename RAMSESTYPE, typename LOGICTYPE>
        static std::vector<RAMSESTYPE> FlattenArrayData(const PropertyImpl& arrayProperty)
        {
            std::vector<RAMSESTYPE> arrayData;
            // Reserve space for array size times size of logic array element type
            arrayData.reserve(arrayProperty.getChildCount() * ComponentsSizeForPropertyType(arrayProperty.getChild(0)->getType()));

            const size_t arraySize = arrayProperty.getChildCount();
            for (size_t i = 0; i < arraySize; ++i)
            {
                FlattenElementIntoArray(arrayData, *arrayProperty.getChild(i)->get<LOGICTYPE>());
            }

            return arrayData;
        }

    private:
        // LOGICTYPE == float, int32, ... (arithmetic types)
        template <typename RAMSESTYPE, typename LOGICTYPE>
        inline typename std::enable_if<std::is_arithmetic<LOGICTYPE>::value, void>::type
        static FlattenElementIntoArray(std::vector<RAMSESTYPE>& ramsesArray, LOGICTYPE logicElement)
        {
            ramsesArray.emplace_back(logicElement);
        }

        // LOGICTYPE == vec2f, vec3i, ... (array types)
        template <typename RAMSESTYPE, typename LOGICTYPE>
        inline typename std::enable_if<!std::is_arithmetic<LOGICTYPE>::value, void>::type
        static FlattenElementIntoArray(std::vector<RAMSESTYPE>& ramsesArray, LOGICTYPE logicElement)
        {
            for (size_t i = 0; i < logicElement.size(); ++i)
            {
                ramsesArray.emplace_back(logicElement[i]);
            }
        }

        static size_t ComponentsSizeForPropertyType(EPropertyType propertyType)
        {
            switch (propertyType)
            {
            case EPropertyType::Float:
            case EPropertyType::Int32:
                return 1u;
            case EPropertyType::Vec2f:
            case EPropertyType::Vec2i:
                return 2u;
            case EPropertyType::Vec3f:
            case EPropertyType::Vec3i:
                return 3u;
            case EPropertyType::Vec4f:
            case EPropertyType::Vec4i:
                return 4u;
            case EPropertyType::String:
            case EPropertyType::Array:
            case EPropertyType::Struct:
            case EPropertyType::Bool:
                assert(false && "This should never happen");
            }
            return 0u;
        }
    };
}
