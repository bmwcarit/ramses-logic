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
#include "LoggerImpl.h"

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

    flatbuffers::Offset<rlogic_serialization::Property> PropertyImpl::Serialize(const PropertyImpl& prop, flatbuffers::FlatBufferBuilder& builder)
    {
        auto result = SerializeRecursive(prop, builder);
        builder.Finish(result);
        return result;
    }

    flatbuffers::Offset<rlogic_serialization::Property> PropertyImpl::SerializeRecursive(const PropertyImpl& prop, flatbuffers::FlatBufferBuilder& builder)
    {
        std::vector<flatbuffers::Offset<rlogic_serialization::Property>> child_vector;
        child_vector.reserve(prop.m_children.size());

        std::transform(prop.m_children.begin(), prop.m_children.end(), std::back_inserter(child_vector), [&builder](const std::vector<std::unique_ptr<Property>>::value_type& child) {
            return SerializeRecursive(*child->m_impl, builder);
            });

        // Assume primitive property, override only for struct/arrays based on m_type
        rlogic_serialization::EPropertyRootType propertyRootType = rlogic_serialization::EPropertyRootType::Primitive;
        rlogic_serialization::PropertyValue valueType = rlogic_serialization::PropertyValue::NONE;
        flatbuffers::Offset<void> valueOffset;

        switch (prop.m_type)
        {
        case EPropertyType::Bool:
        {
            const rlogic_serialization::bool_s bool_struct(std::get<bool>(prop.m_value));
            valueOffset = builder.CreateStruct(bool_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::bool_s>::enum_value;
        }
        break;
        case EPropertyType::Float:
        {
            const rlogic_serialization::float_s float_struct(std::get<float>(prop.m_value));
            valueOffset = builder.CreateStruct(float_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::float_s>::enum_value;
        }
        break;
        case EPropertyType::Vec2f:
        {
            const auto& valueVec2f = std::get<vec2f>(prop.m_value);
            const rlogic_serialization::vec2f_s vec2f_struct(valueVec2f[0], valueVec2f[1]);
            valueOffset = builder.CreateStruct(vec2f_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec2f_s>::enum_value;
        }
        break;
        case EPropertyType::Vec3f:
        {
            const auto& valueVec3f = std::get<vec3f>(prop.m_value);
            const rlogic_serialization::vec3f_s vec3f_struct(valueVec3f[0], valueVec3f[1], valueVec3f[2]);
            valueOffset = builder.CreateStruct(vec3f_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec3f_s>::enum_value;
        }
        break;
        case EPropertyType::Vec4f:
        {
            const auto& valueVec4f = std::get<vec4f>(prop.m_value);
            const rlogic_serialization::vec4f_s vec4f_struct(valueVec4f[0], valueVec4f[1], valueVec4f[2], valueVec4f[3]);
            valueOffset = builder.CreateStruct(vec4f_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec4f_s>::enum_value;
        }
        break;
        case EPropertyType::Int32:
        {
            const rlogic_serialization::int32_s int32_struct(std::get<int32_t>(prop.m_value));
            valueOffset = builder.CreateStruct(int32_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::int32_s>::enum_value;
        }
        break;
        case EPropertyType::Vec2i:
        {
            const auto& valueVec2i = std::get<vec2i>(prop.m_value);
            const rlogic_serialization::vec2i_s vec2i_struct(valueVec2i[0], valueVec2i[1]);
            valueOffset = builder.CreateStruct(vec2i_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec2i_s>::enum_value;
        }
        break;
        case EPropertyType::Vec3i:
        {
            const auto& valueVec3i = std::get<vec3i>(prop.m_value);
            const rlogic_serialization::vec3i_s vec3i_struct(valueVec3i[0], valueVec3i[1], valueVec3i[2]);
            valueOffset = builder.CreateStruct(vec3i_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec3i_s>::enum_value;
        }
        break;
        case EPropertyType::Vec4i:
        {
            const auto& valueVec4i = std::get<vec4i>(prop.m_value);
            const rlogic_serialization::vec4i_s vec4i_struct(valueVec4i[0], valueVec4i[1], valueVec4i[2], valueVec4i[3]);
            valueOffset = builder.CreateStruct(vec4i_struct).Union();
            valueType = rlogic_serialization::PropertyValueTraits<rlogic_serialization::vec4i_s>::enum_value;
        }
        break;
        case EPropertyType::String:
        {
            valueOffset = rlogic_serialization::Createstring_s(builder, builder.CreateString(std::get<std::string>(prop.m_value))).Union();
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
            builder.CreateString(prop.m_name),
            propertyRootType,
            builder.CreateVector(child_vector),
            valueType,
            valueOffset
        );
        return propertyFB;
    }

    std::unique_ptr<PropertyImpl> PropertyImpl::Deserialize(const rlogic_serialization::Property& prop, EPropertySemantics semantics)
    {
        std::unique_ptr<PropertyImpl> impl(new PropertyImpl(prop.name()->string_view(), ConvertSerializationTypeToEPropertyType(prop.rootType(), prop.value_type()), semantics));

        // If primitive: set value; otherwise load children
        if (prop.rootType() == rlogic_serialization::EPropertyRootType::Primitive)
        {
            switch (prop.value_type())
            {
            case rlogic_serialization::PropertyValue::float_s:
                impl->m_value = prop.value_as_float_s()->v();
                break;
            case rlogic_serialization::PropertyValue::vec2f_s: {
                auto vec2fValue = prop.value_as_vec2f_s();
                impl->m_value         = vec2f{vec2fValue->x(), vec2fValue->y()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec3f_s: {
                auto vec3fValue = prop.value_as_vec3f_s();
                impl->m_value         = vec3f{vec3fValue->x(), vec3fValue->y(), vec3fValue->z()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec4f_s: {
                auto vec4fValue = prop.value_as_vec4f_s();
                impl->m_value         = vec4f{vec4fValue->x(), vec4fValue->y(), vec4fValue->z(), vec4fValue->w()};
                break;
            }
            case rlogic_serialization::PropertyValue::int32_s:
                impl->m_value = prop.value_as_int32_s()->v();
                break;
            case rlogic_serialization::PropertyValue::vec2i_s: {
                auto vec2iValue = prop.value_as_vec2i_s();
                impl->m_value         = vec2i{vec2iValue->x(), vec2iValue->y()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec3i_s: {
                auto vec3iValue = prop.value_as_vec3i_s();
                impl->m_value         = vec3i{vec3iValue->x(), vec3iValue->y(), vec3iValue->z()};
                break;
            }
            case rlogic_serialization::PropertyValue::vec4i_s: {
                auto vec4iValue = prop.value_as_vec4i_s();
                impl->m_value         = vec4i{vec4iValue->x(), vec4iValue->y(), vec4iValue->z(), vec4iValue->w()};
                break;
            }
            case rlogic_serialization::PropertyValue::string_s:
                impl->m_value = prop.value_as_string_s()->v()->str();
                break;
            case rlogic_serialization::PropertyValue::bool_s:
                impl->m_value = prop.value_as_bool_s()->v();
                break;
            case rlogic_serialization::PropertyValue::NONE:
                // TODO Violin handle error here, and also default case (enum boundary problems are possible)
                assert(false && "Handling of corrupted/invalid data not supported yet");
                break;
            }
        }
        else
        {
            assert(prop.rootType() == rlogic_serialization::EPropertyRootType::Struct || prop.rootType() == rlogic_serialization::EPropertyRootType::Array);
            for (auto child : *prop.children())
            {
                impl->addChild(PropertyImpl::Deserialize(*child, semantics));
            }
        }

        return impl;
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

        LOG_ERROR("No child property with index '{}' found in '{}'", index, m_name);
        return nullptr;
    }

    const Property* PropertyImpl::getChild(size_t index) const
    {
        if (index < m_children.size())
        {
            return m_children[index].get();
        }

        LOG_ERROR("No child property with index '{}' found in '{}'", index, m_name);
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
        LOG_ERROR("No child property with name '{}' found in '{}'", name, m_name);
        return nullptr;
    }

    bool PropertyImpl::hasChild(std::string_view name) const
    {
        return m_children.end() != std::find_if(m_children.begin(), m_children.end(), [&name](const std::vector<std::unique_ptr<Property>>::value_type& property) {
            return property->getName() == name;
            });
    }

    void PropertyImpl::addChild(std::unique_ptr<PropertyImpl> child)
    {
        assert(nullptr != child);
        assert(m_semantics == child->m_semantics);
        assert(TypeUtils::CanHaveChildren(m_type));
        child->setLogicNode(*m_logicNode);
        m_children.push_back(std::make_unique<Property>(std::move(child)));
    }

    template <typename T> std::optional<T> PropertyImpl::get() const
    {
        if (PropertyTypeToEnum<T>::TYPE == m_type)
        {
            const T* value = std::get_if<T>(&m_value);
            assert(value);
            return {*value};
        }
        LOG_ERROR("Invalid type '{}' when accessing property '{}', correct type is '{}'",
            GetLuaPrimitiveTypeName(PropertyTypeToEnum<T>::TYPE), m_name, GetLuaPrimitiveTypeName(m_type));
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
        LOG_ERROR("Invalid type '{}' when setting property '{}', correct type is '{}'",
            GetLuaPrimitiveTypeName(PropertyTypeToEnum<T>::TYPE), m_name, GetLuaPrimitiveTypeName(m_type));
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

    template <typename T> bool  PropertyImpl::setManually(T value)
    {
        if (m_semantics == EPropertySemantics::ScriptOutput)
        {
            LOG_ERROR(fmt::format("Cannot set property '{}' which is an output.", m_name));
            return false;
        }
        if (m_isLinkedInput)
        {
            LOG_ERROR(fmt::format("Property '{}' is currently linked. Unlink it first before setting its value!", m_name));
            return false;
        }
        return set<T>(value);
    }

    template bool PropertyImpl::setManually<float>(float /*value*/);
    template bool PropertyImpl::setManually<vec2f>(vec2f /*value*/);
    template bool PropertyImpl::setManually<vec3f>(vec3f /*value*/);
    template bool PropertyImpl::setManually<vec4f>(vec4f /*value*/);
    template bool PropertyImpl::setManually<int32_t>(int32_t /*value*/);
    template bool PropertyImpl::setManually<vec2i>(vec2i /*value*/);
    template bool PropertyImpl::setManually<vec3i>(vec3i /*value*/);
    template bool PropertyImpl::setManually<vec4i>(vec4i /*value*/);
    template bool PropertyImpl::setManually<std::string>(std::string /*value*/);
    template bool PropertyImpl::setManually<bool>(bool /*value*/);

    template <typename T> bool  PropertyImpl::setOutputFromScript(T value)
    {
        assert(m_semantics == EPropertySemantics::ScriptOutput && "Property has to be a ScriptOutput");
        return set<T>(value);
    }

    template bool PropertyImpl::setOutputFromScript<float>(float /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec2f>(vec2f /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec3f>(vec3f /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec4f>(vec4f /*value*/);
    template bool PropertyImpl::setOutputFromScript<int32_t>(int32_t /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec2i>(vec2i /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec3i>(vec3i /*value*/);
    template bool PropertyImpl::setOutputFromScript<vec4i>(vec4i /*value*/);
    template bool PropertyImpl::setOutputFromScript<std::string>(std::string /*value*/);
    template bool PropertyImpl::setOutputFromScript<bool>(bool /*value*/);

    void PropertyImpl::setIsLinkedInput(bool isLinkedInput)
    {
        m_isLinkedInput = isLinkedInput;
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
