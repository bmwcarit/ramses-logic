//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

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

    // Remembers flatbuffers pointers to deserialized objects temporarily during deserialization
    class DeserializationMap
    {
    public:
        void storePropertyImpl(const rlogic_serialization::Property& flatbufferObject, PropertyImpl& impl)
        {
            assert(m_properties.find(&flatbufferObject) == m_properties.end() && "never try to store the same object twice");
            m_properties.emplace(std::make_pair(&flatbufferObject, &impl));
        }

        PropertyImpl& resolvePropertyImpl(const rlogic_serialization::Property& flatbufferObject) const
        {
            auto iter = m_properties.find(&flatbufferObject);
            assert(iter != m_properties.end() && iter->second != nullptr);
            return *iter->second;
        }

        void storeDataArray(const rlogic_serialization::DataArray& flatbufferObject, const DataArray& dataArray)
        {
            assert(m_dataArrays.count(&flatbufferObject) == 0 && "one time store only");
            m_dataArrays.insert({ &flatbufferObject, &dataArray });
        }

        const DataArray& resolveDataArray(const rlogic_serialization::DataArray& flatbufferObject) const
        {
            const auto it = m_dataArrays.find(&flatbufferObject);
            assert(it != m_dataArrays.cend());
            return *it->second;
        }

        void storeLuaModule(const rlogic_serialization::LuaModule& flatbufferObject, const LuaModule& luaModule)
        {
            assert(m_luaModules.count(&flatbufferObject) == 0 && "one time store only");
            m_luaModules.insert({ &flatbufferObject, &luaModule });
        }

        const LuaModule& resolveLuaModule(const rlogic_serialization::LuaModule& flatbufferObject) const
        {
            const auto it = m_luaModules.find(&flatbufferObject);
            assert(it != m_luaModules.cend());
            return *it->second;
        }

    private:
        std::unordered_map<const rlogic_serialization::Property*, PropertyImpl*> m_properties;
        std::unordered_map<const rlogic_serialization::DataArray*, const DataArray*> m_dataArrays;
        std::unordered_map<const rlogic_serialization::LuaModule*, const LuaModule*> m_luaModules;
    };

}
