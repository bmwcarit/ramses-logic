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
namespace rlogic_serialization
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
    class LogicNodeImpl;

    enum class EInputOutputProperty : uint8_t
    {
        Input,
        Output
    };

    class PropertyImpl
    {
    public:
        PropertyImpl(std::string_view name, EPropertyType type, EInputOutputProperty inputOutput);
        PropertyImpl(const rlogic_serialization::Property* prop, EInputOutputProperty inputOutput);
        static std::unique_ptr<PropertyImpl> Create(const rlogic_serialization::Property* prop, EInputOutputProperty inputOutput);

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

        bool isInput() const;
        bool isOutput() const;
        EInputOutputProperty getInputOutputProperty() const;

        Property* getChild(size_t index);
        Property* getChild(std::string_view name);

        void addChild(std::unique_ptr<PropertyImpl> child);
        void clearChildren();

        template <typename T> std::optional<T> get() const;
        template <typename T> bool set(T value);
        void set(const PropertyImpl& other);

        flatbuffers::Offset<rlogic_serialization::Property> serialize(flatbuffers::FlatBufferBuilder& builder);

        void setLogicNode(LogicNodeImpl& logicNode);
        LogicNodeImpl& getLogicNode();
        const LogicNodeImpl& getLogicNode() const;

    private:
        std::string                                     m_name;
        EPropertyType                                   m_type;
        std::vector<std::unique_ptr<Property>>          m_children;
        std::variant<int32_t, float, bool, std::string, vec2f, vec3f, vec4f, vec2i, vec3i, vec4i> m_value;

        // TODO Violin/Sven consider solving this more elegantly
        LogicNodeImpl*                                  m_logicNode = nullptr;
        // TODO Violin/Sven/Tobias re-consider if we want to track it this way after we have implemented dirty handling and more binding types
        bool m_wasSet = false;
        EInputOutputProperty                            m_inputOutputProperty;

        flatbuffers::Offset<rlogic_serialization::Property> serialize_recursive(flatbuffers::FlatBufferBuilder& builder);
    };
}
