//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/EPropertyType.h"

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

namespace rlogic
{
    class Property;
}
namespace rlogic::serialization
{
    struct Property;
}

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic::internal
{

    class PropertyImpl
    {
    public:
        PropertyImpl(std::string_view name, EPropertyType type);
        static std::unique_ptr<PropertyImpl> Create(const serialization::Property* prop);

        // Move-able (noexcept); Not copy-able
        ~PropertyImpl() noexcept = default;
        PropertyImpl& operator=(PropertyImpl&& other) noexcept = default;
        PropertyImpl(PropertyImpl&& other) noexcept = default;
        PropertyImpl& operator=(const PropertyImpl& other) = delete;
        PropertyImpl(const PropertyImpl& other) = delete;

        size_t getChildCount() const;
        EPropertyType getType() const;
        std::string_view getName() const;
        bool wasSet() const;

        const Property* getChild(size_t index) const;
        const Property* getChild(std::string_view name) const;

        Property* getChild(size_t index);
        Property* getChild(std::string_view name);

        void addChild(std::unique_ptr<PropertyImpl> child);

        template <typename T> std::optional<T> get() const;
        template <typename T> bool set(T value);

        flatbuffers::Offset<rlogic::serialization::Property> serialize(flatbuffers::FlatBufferBuilder& builder);

    private:
        std::string                                     m_name;
        EPropertyType                                   m_type;
        std::vector<std::unique_ptr<Property>>          m_children;
        std::variant<int32_t, float, bool, std::string, vec2f, vec3f, vec4f, vec2i, vec3i, vec4i> m_value;
        // TODO Violin/Sven/Tobias re-consider if we want to track it this way after we have implemented dirty handling and more binding types
        bool m_wasSet = false;

        flatbuffers::Offset<rlogic::serialization::Property> serialize_recursive(flatbuffers::FlatBufferBuilder& builder);
    };
}
