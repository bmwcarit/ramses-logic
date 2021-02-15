//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/Property.h"

#include "impl/PropertyImpl.h"
#include "impl/LogicNodeImpl.h"

#include "internals/SerializationHelper.h"
#include "internals/TypeUtils.h"

#include "generated/property_gen.h"

#include <cassert>
#include <algorithm>

namespace rlogic::internal
{
    PropertyImpl::PropertyImpl(std::string_view name, EPropertyType type, EPropertySemantics semantics)
        : m_name(name)
        , m_type(type)
        , m_semantics(semantics)
    {
        switch (type)
        {
        case EPropertyType::Float:
            m_value = 0.0f;
            break;
        case EPropertyType::Vec2f:
            m_value = vec2f{0.0f, 0.0f};
            break;
        case EPropertyType::Vec3f:
            m_value = vec3f{0.0f, 0.0f, 0.0f};
            break;
        case EPropertyType::Vec4f:
            m_value = vec4f{0.0f, 0.0f, 0.0f, 0.0f};
            break;
        case EPropertyType::Int32:
            m_value = 0;
            break;
        case EPropertyType::Vec2i:
            m_value = vec2i{0, 0};
            break;
        case EPropertyType::Vec3i:
            m_value = vec3i{0, 0, 0};
            break;
        case EPropertyType::Vec4i:
            m_value = vec4i{0, 0, 0, 0};
            break;
        case EPropertyType::String:
            m_value = std::string("");
            break;
        case EPropertyType::Bool:
            m_value = false;
            break;
        case EPropertyType::Array:
        case EPropertyType::Struct:
            break;
        }
    }

    PropertyImpl::PropertyImpl(const rlogic_serialization::Property& prop, EPropertySemantics semantics)
        : m_name(prop.name()->string_view())
        , m_type(ConvertSerializationTypeToEPropertyType(prop.rootType(), prop.value_type()))
        , m_semantics(semantics)
    {
        // If primitive: set value; otherwise load children
        if (prop.rootType() == rlogic_serialization::EPropertyRootType::Primitive)
        {
            switch (prop.value_type())
            {
            case rlogic_serialization::PropertyValue::float_s:
                m_value = prop.value_as_float_s()->v();
                break;
            case rlogic_serialization::PropertyValue::vec2f_s: {
                auto vec2fValue = prop.value_as_vec2f_s();
                m_value         = vec2f{vec2fValue->x(), vec2fValue->y()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec3f_s: {
                auto vec3fValue = prop.value_as_vec3f_s();
                m_value         = vec3f{vec3fValue->x(), vec3fValue->y(), vec3fValue->z()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec4f_s: {
                auto vec4fValue = prop.value_as_vec4f_s();
                m_value         = vec4f{vec4fValue->x(), vec4fValue->y(), vec4fValue->z(), vec4fValue->w()};
                break;
            }
            case rlogic_serialization::PropertyValue::int32_s:
                m_value = prop.value_as_int32_s()->v();
                break;
            case rlogic_serialization::PropertyValue::vec2i_s: {
                auto vec2iValue = prop.value_as_vec2i_s();
                m_value         = vec2i{vec2iValue->x(), vec2iValue->y()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec3i_s: {
                auto vec3iValue = prop.value_as_vec3i_s();
                m_value         = vec3i{vec3iValue->x(), vec3iValue->y(), vec3iValue->z()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec4i_s: {
                auto vec4iValue = prop.value_as_vec4i_s();
                m_value         = vec4i{vec4iValue->x(), vec4iValue->y(), vec4iValue->z(), vec4iValue->w()};
                break;
            }
            case rlogic_serialization::PropertyValue::string_s:
                m_value = prop.value_as_string_s()->v()->str();
                break;
            case rlogic_serialization::PropertyValue::bool_s:
                m_value = prop.value_as_bool_s()->v();
                break;
            case rlogic_serialization::PropertyValue::NONE:
                break;
            }
        }
        else
        {
            for (auto child : *prop.children())
            {
                addChild(Create(*child, semantics));
            }
        }
    }

    size_t PropertyImpl::getChildCount() const
    {
        return m_children.size();
    }

    EPropertyType PropertyImpl::getType() const
    {
        return m_type;
    }

    std::string_view PropertyImpl::getName() const
    {
        return m_name;
    }

    Property* PropertyImpl::getChild(size_t index)
    {
        if (index < m_children.size())
        {
            return m_children[index].get();
        }

        return nullptr;
    }

    const Property* PropertyImpl::getChild(size_t index) const
    {
        if (index < m_children.size())
        {
            return m_children[index].get();
        }

        return nullptr;
    }

    Property* PropertyImpl::getChild(std::string_view name)
    {
        auto it = std::find_if(m_children.begin(), m_children.end(), [&name](const std::vector<std::unique_ptr<Property>>::value_type& property) {
            return property->getName() == name;
        });
        if (it != m_children.end())
        {
            return it->get();
        }
        return nullptr;
    }

    void PropertyImpl::addChild(std::unique_ptr<PropertyImpl> child)
    {
        assert(nullptr != child);
        assert(m_semantics == child->m_semantics);
        assert(TypeUtils::CanHaveChildren(m_type));
        child->setLogicNode(*m_logicNode);
        m_children.push_back(std::make_unique<Property>(std::move(child)));
    }

    flatbuffers::Offset<rlogic_serialization::Property> PropertyImpl::serialize(flatbuffers::FlatBufferBuilder& builder)
    {
        auto result = serialize_recursive(builder);
        builder.Finish(result);
        return result;
    }

    flatbuffers::Offset<rlogic_serialization::Property> PropertyImpl::serialize_recursive(flatbuffers::FlatBufferBuilder& builder)
    {
        std::vector<flatbuffers::Offset<rlogic_serialization::Property>> child_vector;
        child_vector.reserve(m_children.size());

        std::transform(m_children.begin(), m_children.end(), std::back_inserter(child_vector), [&builder](const std::vector<std::unique_ptr<Property>>::value_type& child) {
            return child->m_impl->serialize_recursive(builder);
        });

        // Assume primitive property, override only for struct/arrays based on m_type
        rlogic_serialization::EPropertyRootType propertyRootType = rlogic_serialization::EPropertyRootType::Primitive;
        rlogic_serialization::PropertyValue valueType = rlogic_serialization::PropertyValue::NONE;
        flatbuffers::Offset<void> valueOffset;

        switch (m_type)
        {
        case EPropertyType::Bool:
            {
                const rlogic_serialization::bool_s bool_struct(std::get<bool>(m_value));
                valueOffset = builder.CreateStruct(bool_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::bool_s>::enum_value;
            }
            break;
        case EPropertyType::Float:
            {
                const rlogic_serialization::float_s float_struct(std::get<float>(m_value));
                valueOffset = builder.CreateStruct(float_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::float_s>::enum_value;
            }
            break;
        case EPropertyType::Vec2f:
            {
                const auto& valueVec2f = std::get<vec2f>(m_value);
                const rlogic_serialization::vec2f_s vec2f_struct(valueVec2f[0], valueVec2f[1]);
                valueOffset = builder.CreateStruct(vec2f_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec2f_s>::enum_value;
            }
            break;
        case EPropertyType::Vec3f:
            {
                const auto& valueVec3f = std::get<vec3f>(m_value);
                const rlogic_serialization::vec3f_s vec3f_struct(valueVec3f[0], valueVec3f[1], valueVec3f[2]);
                valueOffset = builder.CreateStruct(vec3f_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec3f_s>::enum_value;
            }
            break;
        case EPropertyType::Vec4f:
            {
                const auto& valueVec4f = std::get<vec4f>(m_value);
                const rlogic_serialization::vec4f_s vec4f_struct(valueVec4f[0], valueVec4f[1], valueVec4f[2], valueVec4f[3]);
                valueOffset = builder.CreateStruct(vec4f_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec4f_s>::enum_value;
            }
            break;
        case EPropertyType::Int32:
            {
                const rlogic_serialization::int32_s int32_struct(std::get<int32_t>(m_value));
                valueOffset = builder.CreateStruct(int32_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::int32_s>::enum_value;
            }
            break;
        case EPropertyType::Vec2i:
            {
                const auto& valueVec2i = std::get<vec2i>(m_value);
                const rlogic_serialization::vec2i_s vec2i_struct(valueVec2i[0], valueVec2i[1]);
                valueOffset = builder.CreateStruct(vec2i_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec2i_s>::enum_value;
            }
            break;
        case EPropertyType::Vec3i:
            {
                const auto& valueVec3i = std::get<vec3i>(m_value);
                const rlogic_serialization::vec3i_s vec3i_struct(valueVec3i[0], valueVec3i[1], valueVec3i[2]);
                valueOffset = builder.CreateStruct(vec3i_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec3i_s>::enum_value;
            }
            break;
        case EPropertyType::Vec4i:
            {
                const auto& valueVec4i = std::get<vec4i>(m_value);
                const rlogic_serialization::vec4i_s vec4i_struct(valueVec4i[0], valueVec4i[1], valueVec4i[2], valueVec4i[3]);
                valueOffset = builder.CreateStruct(vec4i_struct).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec4i_s>::enum_value;
            }
            break;
        case EPropertyType::String:
            {
                valueOffset = rlogic_serialization::Createstring_s(builder, builder.CreateString(std::get<std::string>(m_value))).Union();
                valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::string_s>::enum_value;
            }
            break;
        case EPropertyType::Array:
            propertyRootType = rlogic_serialization::EPropertyRootType::Array;
            break;
        case EPropertyType::Struct:
            propertyRootType = rlogic_serialization::EPropertyRootType::Struct;
            break;
        }

        auto propertyFB = rlogic_serialization::CreateProperty(builder,
            builder.CreateString(m_name),
            propertyRootType,
            builder.CreateVector(child_vector),
            valueType,
            valueOffset
        );
        return propertyFB;
    }

    std::unique_ptr<PropertyImpl> PropertyImpl::Create(const rlogic_serialization::Property& prop, EPropertySemantics semantics)
    {
        return std::make_unique<PropertyImpl>(prop, semantics);
    }

    template <typename T> std::optional<T> PropertyImpl::get() const
    {
        if (PropertyTypeToEnum<T>::TYPE == m_type)
        {
            if (auto value = std::get_if<T>(&m_value))
            {
                return {*value};
            }
        }
        return std::nullopt;
    }

    template std::optional<float>       PropertyImpl::get<float>() const;
    template std::optional<vec2f>       PropertyImpl::get<vec2f>() const;
    template std::optional<vec3f>       PropertyImpl::get<vec3f>() const;
    template std::optional<vec4f>       PropertyImpl::get<vec4f>() const;
    template std::optional<int32_t>     PropertyImpl::get<int32_t>() const;
    template std::optional<vec2i>       PropertyImpl::get<vec2i>() const;
    template std::optional<vec3i>       PropertyImpl::get<vec3i>() const;
    template std::optional<vec4i>       PropertyImpl::get<vec4i>() const;
    template std::optional<std::string> PropertyImpl::get<std::string>() const;
    template std::optional<bool> PropertyImpl::get<bool>() const;

    template <typename T> bool PropertyImpl::set(T value)
    {
        if (PropertyTypeToEnum<T>::TYPE == m_type)
        {
            // TODO Violin Treat setting output values as an error here - it makes no sense semantics-wise to set the value of an
            // output manually

            // Check if value changed before doing something
            if (std::get<T>(m_value) != value)
            {
                m_value = value;
                m_logicNode->setDirty(true);
            }
            // Binding inputs behave differently than other inputs
            if (m_semantics == EPropertySemantics::BindingInput)
            {
                m_bindingInputHasNewValue = true;
                m_logicNode->setDirty(true);
            }
            return true;
        }
        return false;
    }

    bool PropertyImpl::bindingInputHasNewValue() const
    {
        // TODO Violin can we make this assert the bindings semantics?
        return m_bindingInputHasNewValue;
    }

    bool PropertyImpl::checkForBindingInputNewValueAndReset()
    {
        // TODO Violin can we make this assert the bindings semantics?
        const bool newValue = m_bindingInputHasNewValue;
        m_bindingInputHasNewValue = false;
        return newValue;
    }

    template bool PropertyImpl::set<float>(float /*value*/);
    template bool PropertyImpl::set<vec2f>(vec2f /*value*/);
    template bool PropertyImpl::set<vec3f>(vec3f /*value*/);
    template bool PropertyImpl::set<vec4f>(vec4f /*value*/);
    template bool PropertyImpl::set<int32_t>(int32_t /*value*/);
    template bool PropertyImpl::set<vec2i>(vec2i /*value*/);
    template bool PropertyImpl::set<vec3i>(vec3i /*value*/);
    template bool PropertyImpl::set<vec4i>(vec4i /*value*/);
    template bool PropertyImpl::set<std::string>(std::string /*value*/);
    template bool PropertyImpl::set<bool>(bool /*value*/);

    // TODO Violin remove code duplication here and in other setters, e.g. by splitting the logic
    // for value setting, semantics check, and dirty handling and calling separately
    void PropertyImpl::setInternal(const PropertyImpl& other)
    {
        assert(m_type == other.m_type);
        assert(TypeUtils::IsPrimitiveType(m_type));
        assert(m_semantics == EPropertySemantics::BindingInput || m_semantics == EPropertySemantics::ScriptInput);

        // Check if value changed before doing something
        if (m_value != other.m_value)
        {
            m_value = other.m_value;
            m_logicNode->setDirty(true);
        }

        // Binding inputs behave differently than other inputs
        if (m_semantics == EPropertySemantics::BindingInput)
        {
            m_bindingInputHasNewValue = true;
            m_logicNode->setDirty(true);
        }
    }

    void PropertyImpl::clearChildren()
    {
        m_children.clear();
    }

    void PropertyImpl::setLogicNode(LogicNodeImpl& logicNode)
    {
        assert(m_logicNode == nullptr && "Properties are not transferrable across logic nodes!");
        m_logicNode = &logicNode;
        for (auto& child : m_children)
        {
            child->m_impl->setLogicNode(logicNode);
        }
    }

    LogicNodeImpl& PropertyImpl::getLogicNode()
    {
        assert(m_logicNode != nullptr);
        return *m_logicNode;
    }

    bool PropertyImpl::isInput() const
    {
        return m_semantics == EPropertySemantics::ScriptInput || m_semantics == EPropertySemantics::BindingInput;
    }

    bool PropertyImpl::isOutput() const
    {
        return m_semantics == EPropertySemantics::ScriptOutput;
    }

    EPropertySemantics PropertyImpl::getPropertySemantics() const
    {
        return m_semantics;
    }

    std::unique_ptr<PropertyImpl> PropertyImpl::deepCopy() const
    {
        assert(!m_bindingInputHasNewValue && !m_logicNode && "Deep copy supported only before setting values and attaching to property tree, as means to supplement type expansion only");
        auto deepCopy = std::make_unique<PropertyImpl>(m_name, m_type, m_semantics);

        for (const auto& child : m_children)
        {
            deepCopy->addChild(child->m_impl->deepCopy());
        }
        return deepCopy;
    }
}
