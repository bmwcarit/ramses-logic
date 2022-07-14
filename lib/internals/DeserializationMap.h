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
}

namespace rlogic
{
    class DataArray;
    class LuaModule;
}

namespace rlogic::internal
{
    class PropertyImpl;
    class RamsesNodeBindingImpl;
    class RamsesCameraBindingImpl;

    // Remembers flatbuffers pointers to deserialized objects temporarily during deserialization
    class DeserializationMap
    {
    public:
        void storePropertyImpl(const rlogic_serialization::Property& flatbufferObject, PropertyImpl& impl)
        {
            Store(&flatbufferObject, &impl, m_properties);
        }

        PropertyImpl& resolvePropertyImpl(const rlogic_serialization::Property& flatbufferObject) const
        {
            return *Get(&flatbufferObject, m_properties);
        }

        void storeDataArray(const rlogic_serialization::DataArray& flatbufferObject, const DataArray& dataArray)
        {
            Store(&flatbufferObject, &dataArray, m_dataArrays);
        }

        const DataArray& resolveDataArray(const rlogic_serialization::DataArray& flatbufferObject) const
        {
            return *Get(&flatbufferObject, m_dataArrays);
        }

        void storeLuaModule(uint64_t luaModuleId, const LuaModule& luaModule)
        {
            Store(luaModuleId, &luaModule, m_luaModules);
        }

        const LuaModule* resolveLuaModule(uint64_t luaModuleId) const
        {
            return Get(luaModuleId, m_luaModules);
        }

        void storeNodeBinding(uint64_t id, RamsesNodeBindingImpl& nodeBinding)
        {
            Store(id, &nodeBinding, m_nodeBindings);
        }

        RamsesNodeBindingImpl* resolveNodeBinding(uint64_t id) const
        {
            return Get(id, m_nodeBindings);
        }

        void storeCameraBinding(uint64_t id, RamsesCameraBindingImpl& cameraBinding)
        {
            Store(id, &cameraBinding, m_cameraBindings);
        }

        RamsesCameraBindingImpl* resolveCameraBinding(uint64_t id) const
        {
            return Get(id, m_cameraBindings);
        }

    private:
        template <typename Key, typename Value>
        static void Store(Key key, Value value, std::unordered_map<Key, Value>& container)
        {
            static_assert(std::is_trivially_copyable_v<Key> && std::is_trivially_copyable_v<Value>, "Performance warning");
            assert(container.count(key) == 0 && "one time store only");
            container.emplace(std::move(key), std::move(value));
        }

        template <typename Key, typename Value>
        [[nodiscard]] static Value Get(Key key, const std::unordered_map<Key, Value>& container)
        {
            const auto it = container.find(key);
            assert(it != container.cend());
            return it->second;
        }

        // fail queries using IDs gracefully if given ID not found
        // file can be OK on flatbuffer schema level but might still contain corrupted value
        template <typename Value>
        [[nodiscard]] static Value Get(uint64_t key, const std::unordered_map<uint64_t, Value>& container)
        {
            const auto it = container.find(key);
            return (it == container.cend()) ? nullptr : it->second;
        }

        std::unordered_map<const rlogic_serialization::Property*, PropertyImpl*> m_properties;
        std::unordered_map<const rlogic_serialization::DataArray*, const DataArray*> m_dataArrays;
        std::unordered_map<uint64_t, const LuaModule*> m_luaModules;
        std::unordered_map<uint64_t, RamsesNodeBindingImpl*> m_nodeBindings;
        std::unordered_map<uint64_t, RamsesCameraBindingImpl*> m_cameraBindings;
    };

}
