//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "generated/PropertyGen.h"

#include <unordered_map>

namespace rlogic_serialization
{
    struct Property;
}

namespace rlogic::internal
{
    class PropertyImpl;

    // Remembers flatbuffer offsets for select objects during serialization
    class SerializationMap
    {
    public:
        void storePropertyOffset(const PropertyImpl& impl, flatbuffers::Offset<rlogic_serialization::Property> offset)
        {
            assert(m_properties.find(&impl) == m_properties.end() && "never try to store the same impl twice");
            m_properties.emplace(std::make_pair(&impl, offset));
        }

        flatbuffers::Offset<rlogic_serialization::Property> resolvePropertyOffset(const PropertyImpl& impl) const
        {
            auto iter = m_properties.find(&impl);
            assert(iter != m_properties.end() && !iter->second.IsNull());
            return iter->second;
        }

    private:
        std::unordered_map<const PropertyImpl*, flatbuffers::Offset<rlogic_serialization::Property>> m_properties;
    };

}
