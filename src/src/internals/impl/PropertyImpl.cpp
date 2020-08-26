//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/Property.h"

#include "internals/impl/PropertyImpl.h"

#include "internals/SerializationHelper.h"

#include "generated/property_gen.h"

#include <cassert>
#include <algorithm>

namespace rlogic::internal
{
    PropertyImpl::PropertyImpl(std::string_view name, EPropertyType type)
        : m_name(name)
        , m_type(type)
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
        case EPropertyType::Struct:
            break;
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

    const Property* PropertyImpl::getChild(std::string_view name) const
    {
        auto it = std::find_if(
            m_children.begin(), m_children.end(), [&name](const std::vector<std::unique_ptr<Property>>::value_type& property) { return property->getName() == name; });
        if (it != m_children.end())
        {
            return it->get();
        }
        return nullptr;
    }

    void PropertyImpl::addChild(std::unique_ptr<PropertyImpl> child)
    {
        assert(nullptr != child);
        if (m_type == EPropertyType::Struct)
        {
            m_children.push_back(std::make_unique<Property>(std::move(child)));
        }
    }

    flatbuffers::Offset<rlogic::serialization::Property> PropertyImpl::serialize(flatbuffers::FlatBufferBuilder& builder)
    {
        auto result = serialize_recursive(builder);
        builder.Finish(result);
        return result;
    }

    flatbuffers::Offset<rlogic::serialization::Property> PropertyImpl::serialize_recursive(flatbuffers::FlatBufferBuilder& builder)
    {
        std::vector<flatbuffers::Offset<rlogic::serialization::Property>> child_vector;
        child_vector.reserve(m_children.size());

        std::transform(m_children.begin(), m_children.end(), std::back_inserter(child_vector), [&builder](const std::vector<std::unique_ptr<Property>>::value_type& child) {
            return child->m_impl->serialize_recursive(builder);
        });

        serialization::PropertyValue value_type = serialization::PropertyValue::NONE;
        flatbuffers::Offset<void> value_offset;

        switch (m_type)
        {
        case EPropertyType::Bool:
            {
                const serialization::bool_s bool_struct(std::get<bool>(m_value));
                value_offset = builder.CreateStruct(bool_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::bool_s>::enum_value;
            }
            break;
        case EPropertyType::Float:
            {
                const serialization::float_s float_struct(std::get<float>(m_value));
                value_offset = builder.CreateStruct(float_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::float_s>::enum_value;
            }
            break;
        case EPropertyType::Vec2f:
            {
                const auto& valueVec2f = std::get<vec2f>(m_value);
                const serialization::vec2f_s vec2f_struct(valueVec2f[0], valueVec2f[1]);
                value_offset = builder.CreateStruct(vec2f_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec2f_s>::enum_value;
            }
            break;
        case EPropertyType::Vec3f:
            {
                const auto& valueVec3f = std::get<vec3f>(m_value);
                const serialization::vec3f_s vec3f_struct(valueVec3f[0], valueVec3f[1], valueVec3f[2]);
                value_offset = builder.CreateStruct(vec3f_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec3f_s>::enum_value;
            }
            break;
        case EPropertyType::Vec4f:
            {
                const auto& valueVec4f = std::get<vec4f>(m_value);
                const serialization::vec4f_s vec4f_struct(valueVec4f[0], valueVec4f[1], valueVec4f[2], valueVec4f[3]);
                value_offset = builder.CreateStruct(vec4f_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec4f_s>::enum_value;
            }
            break;
        case EPropertyType::Int32:
            {
                const serialization::int32_s int32_struct(std::get<int32_t>(m_value));
                value_offset = builder.CreateStruct(int32_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::int32_s>::enum_value;
            }
            break;
        case EPropertyType::Vec2i:
            {
                const auto& valueVec2i = std::get<vec2i>(m_value);
                const serialization::vec2i_s vec2i_struct(valueVec2i[0], valueVec2i[1]);
                value_offset = builder.CreateStruct(vec2i_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec2i_s>::enum_value;
            }
            break;
        case EPropertyType::Vec3i:
            {
                const auto& valueVec3i = std::get<vec3i>(m_value);
                const serialization::vec3i_s vec3i_struct(valueVec3i[0], valueVec3i[1], valueVec3i[2]);
                value_offset = builder.CreateStruct(vec3i_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec3i_s>::enum_value;
            }
            break;
        case EPropertyType::Vec4i:
            {
                const auto& valueVec4i = std::get<vec4i>(m_value);
                const serialization::vec4i_s vec4i_struct(valueVec4i[0], valueVec4i[1], valueVec4i[2], valueVec4i[3]);
                value_offset = builder.CreateStruct(vec4i_struct).Union();
                value_type = serialization::PropertyValueTraits<serialization::vec4i_s>::enum_value;
            }
            break;
        case EPropertyType::String:
            {
                value_offset = serialization::Createstring_s(builder, builder.CreateString(std::get<std::string>(m_value))).Union();
                value_type = serialization::PropertyValueTraits<serialization::string_s>::enum_value;
            }
            break;
        case EPropertyType::Struct:
            break;
        }

        auto propertyFB = serialization::CreateProperty(builder,
            builder.CreateString(m_name),
            builder.CreateVector(child_vector),
            value_type,
            value_offset,
            m_wasSet
        );
        return propertyFB;
    }

    std::unique_ptr<PropertyImpl> PropertyImpl::Create(const serialization::Property* prop)
    {
        // TODO Violin this should be an assert
        if (nullptr == prop)
        {
            return nullptr;
        }

        const auto type = prop->children()->size() > 0 ? EPropertyType::Struct : ConvertSerializationTypeToEPropertyType(prop->value_type());
        auto property = std::make_unique<PropertyImpl>(prop->name()->string_view(), type);

        switch (prop->value_type())
        {
        case serialization::PropertyValue::bool_s:
            property->set(prop->value_as_bool_s()->v());
            break;
        case serialization::PropertyValue::float_s:
            property->set(prop->value_as_float_s()->v());
            break;
        case serialization::PropertyValue::vec2f_s:
        {
            auto vec2fValue = prop->value_as_vec2f_s();
            property->set<vec2f>({vec2fValue->x(), vec2fValue->y()});
            break;
        }
        case serialization::PropertyValue::vec3f_s: {
            auto vec3fValue = prop->value_as_vec3f_s();
            property->set<vec3f>({vec3fValue->x(), vec3fValue->y(), vec3fValue->z()});
            break;
        }
        case serialization::PropertyValue::vec4f_s: {
            auto vec4fValue = prop->value_as_vec4f_s();
            property->set<vec4f>({vec4fValue->x(), vec4fValue->y(), vec4fValue->z(), vec4fValue->w()});
            break;
        }
        case serialization::PropertyValue::int32_s:
            property->set(prop->value_as_int32_s()->v());
            break;
        case serialization::PropertyValue::vec2i_s: {
            auto vec2iValue = prop->value_as_vec2i_s();
            property->set<vec2i>({vec2iValue->x(), vec2iValue->y()});
            break;
        }
        case serialization::PropertyValue::vec3i_s: {
            auto vec3iValue = prop->value_as_vec3i_s();
            property->set<vec3i>({vec3iValue->x(), vec3iValue->y(), vec3iValue->z()});
            break;
        }
        case serialization::PropertyValue::vec4i_s: {
            auto vec4iValue = prop->value_as_vec4i_s();
            property->set<vec4i>({vec4iValue->x(), vec4iValue->y(), vec4iValue->z(), vec4iValue->w()});
            break;
        }
        case serialization::PropertyValue::string_s:
            property->set(prop->value_as_string_s()->v()->str());
            break;
        case serialization::PropertyValue::NONE:
            break;
        }
        // TODO Violin/Sven we can probably save some time by having default values not being deserialized
        // If we do this, we could also not serialize the m_hasDefaultValue maybe
        property->m_wasSet = prop->wasSet();

        for (auto child : *prop->children())
        {
            property->addChild(Create(child));
        }

        return property;
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
            m_value = value;
            m_wasSet = true;
            return true;
        }
        return false;
    }

    bool PropertyImpl::wasSet() const
    {
        return m_wasSet;
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
}
