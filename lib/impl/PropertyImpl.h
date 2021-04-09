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

    enum class EPropertySemantics : uint8_t
    {
        ScriptInput,
        BindingInput,
        ScriptOutput
    };

    class PropertyImpl
    {
    public:
        PropertyImpl(std::string_view name, EPropertyType type, EPropertySemantics semantics);
        PropertyImpl(const rlogic_serialization::Property& prop, EPropertySemantics semantics);
        static std::unique_ptr<PropertyImpl> Create(const rlogic_serialization::Property& prop, EPropertySemantics semantics);

        // Move-able (noexcept); Not copy-able
        ~PropertyImpl() noexcept = default;
        PropertyImpl& operator=(PropertyImpl&& other) noexcept = default;
        PropertyImpl(PropertyImpl&& other) noexcept = default;
        PropertyImpl& operator=(const PropertyImpl& other) = delete;
        PropertyImpl(const PropertyImpl& other) = delete;

        // Creates a data copy of itself and its children on new memory
        [[nodiscard]] std::unique_ptr<PropertyImpl> deepCopy() const;

        [[nodiscard]] size_t getChildCount() const;
        [[nodiscard]] EPropertyType getType() const;
        [[nodiscard]] std::string_view getName() const;

        [[nodiscard]] bool bindingInputHasNewValue() const;
        [[nodiscard]] bool checkForBindingInputNewValueAndReset();

        [[nodiscard]] const Property* getChild(size_t index) const;

        // TODO Violin these 3 methods have redundancy, refactor
        [[nodiscard]] bool isInput() const;
        [[nodiscard]] bool isOutput() const;
        [[nodiscard]] EPropertySemantics getPropertySemantics() const;

        [[nodiscard]] Property* getChild(size_t index);
        [[nodiscard]] Property* getChild(std::string_view name);

        void addChild(std::unique_ptr<PropertyImpl> child);
        void clearChildren();

        // TODO Violin refactor the different setters below to reduce code duplication
        // and make better readable (have only one place where values are set, and a consistent
        // "dirtyness" check)

        template <typename T> [[nodiscard]] std::optional<T> get() const;

        template <typename T> bool setManually(T value);
        template <typename T> bool setOutputFromScript(T value);
        void setIsLinkedInput(bool isLinkedInput);

        // This version is used by links to avoid type expansion
        void setInternal(const PropertyImpl& other);
        // This version is used for non-default value initialization (to avoid having templated constructor)
        template <typename T> void setInternal(T value)
        {
            assert (PropertyTypeToEnum<T>::TYPE == m_type);
            assert (std::get<T>(m_value) != value && "Use this only to set non-default values!");
            m_value = value;
        }

        flatbuffers::Offset<rlogic_serialization::Property> serialize(flatbuffers::FlatBufferBuilder& builder);

        void setLogicNode(LogicNodeImpl& logicNode);
        [[nodiscard]] LogicNodeImpl& getLogicNode();

    private:
        std::string                                     m_name;
        EPropertyType                                   m_type;
        std::vector<std::unique_ptr<Property>>          m_children;
        std::variant<int32_t, float, bool, std::string, vec2f, vec3f, vec4f, vec2i, vec3i, vec4i> m_value;

        // TODO Violin/Sven consider solving this more elegantly
        LogicNodeImpl*                                  m_logicNode = nullptr;
        bool m_bindingInputHasNewValue = false;
        bool m_isLinkedInput = false;
        EPropertySemantics                              m_semantics;

        template <typename T> bool set(T value);
        flatbuffers::Offset<rlogic_serialization::Property> serialize_recursive(flatbuffers::FlatBufferBuilder& builder);
    };
}
