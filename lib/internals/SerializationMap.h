//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "generated/PropertyGen.h"
#include "generated/DataArrayGen.h"

#include <unordered_map>

namespace rlogic_serialization
{
    struct Property;
    struct DataArray;
    struct LuaModule;
}

namespace rlogic
{
    class DataArray;
    class LuaModule;
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

        void storeDataArray(const DataArray& dataArray, flatbuffers::Offset<rlogic_serialization::DataArray> offset)
        {
            assert(m_dataArrays.count(&dataArray) == 0 && "one time store only");
            m_dataArrays.insert({ &dataArray, offset });
        }

        flatbuffers::Offset<rlogic_serialization::DataArray> resolveDataArrayOffset(const DataArray& dataArray) const
        {
            const auto it = m_dataArrays.find(&dataArray);
            assert(it != m_dataArrays.cend());
            return it->second;
        }

        void storeLuaModule(uint64_t luaModuleId, flatbuffers::Offset<rlogic_serialization::LuaModule> offset)
        {
            assert(luaModuleId != 0 && "Module must have valid id!");
            assert(m_luaModules.count(luaModuleId) == 0 && "one time store only");
            m_luaModules.insert({ luaModuleId, offset });
        }

        flatbuffers::Offset<rlogic_serialization::LuaModule> resolveLuaModuleOffset(uint64_t luaModuleId) const
        {
            assert(luaModuleId != 0 && "Module must have valid id!");
            const auto it = m_luaModules.find(luaModuleId);
            assert(it != m_luaModules.cend());
            return it->second;
        }

    private:
        std::unordered_map<const PropertyImpl*, flatbuffers::Offset<rlogic_serialization::Property>> m_properties;
        std::unordered_map<const DataArray*, flatbuffers::Offset<rlogic_serialization::DataArray>> m_dataArrays;
        std::unordered_map<uint64_t, flatbuffers::Offset<rlogic_serialization::LuaModule>> m_luaModules;
    };

}
