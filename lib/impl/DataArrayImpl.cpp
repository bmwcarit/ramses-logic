//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/DataArrayImpl.h"
#include "generated/DataArrayGen.h"
#include "flatbuffers/flatbuffers.h"
#include "internals/ErrorReporting.h"
#include "LoggerImpl.h"
#include <cassert>

namespace rlogic::internal
{
    template <typename T>
    DataArrayImpl::DataArrayImpl(std::vector<T>&& data, std::string_view name, uint64_t id)
        : LogicObjectImpl(name, id)
        , m_dataType{ PropertyTypeToEnum<T>::TYPE }
        , m_data{ std::move(data) }
    {
    }

    template <typename T>
    const std::vector<T>* rlogic::internal::DataArrayImpl::getData() const
    {
        if (PropertyTypeToEnum<T>::TYPE != m_dataType)
        {
            LOG_ERROR("DataArray::getData failed, correct template that matches stored data type must be used.");
            return nullptr;
        }

        return &std::get<std::vector<T>>(m_data);
    }

    EPropertyType DataArrayImpl::getDataType() const
    {
        return m_dataType;
    }

    template <typename T>
    constexpr size_t getNumElements()
    {
        if constexpr (std::is_arithmetic_v<T>)
        {
            return 1u;
        }
        else
        {
            return std::tuple_size_v<T>;
        }
    }

    template <typename T, typename fbT>
    std::vector<fbT> flattenArrayOfVec(const DataArrayImpl::DataArrayVariant& data)
    {
        const auto& dataVec = std::get<std::vector<T>>(data);
        std::vector<fbT> dataFlattened;
        dataFlattened.reserve(dataVec.size() * getNumElements<T>());
        for (const auto& v : dataVec)
            dataFlattened.insert(dataFlattened.end(), v.cbegin(), v.cend());
        return dataFlattened;
    }

    flatbuffers::Offset<rlogic_serialization::DataArray> DataArrayImpl::Serialize(const DataArrayImpl& data, flatbuffers::FlatBufferBuilder& builder)
    {
        rlogic_serialization::ArrayUnion unionType = rlogic_serialization::ArrayUnion::NONE;
        rlogic_serialization::EDataArrayType arrayType = rlogic_serialization::EDataArrayType::Float;
        flatbuffers::Offset<void> dataOffset;

        switch (data.m_dataType)
        {
        case EPropertyType::Float:
            unionType = rlogic_serialization::ArrayUnion::floatArr;
            arrayType = rlogic_serialization::EDataArrayType::Float;
            dataOffset = rlogic_serialization::CreatefloatArr(builder, builder.CreateVector(std::get<std::vector<float>>(data.m_data))).Union();
            break;
        case EPropertyType::Vec2f:
            unionType = rlogic_serialization::ArrayUnion::floatArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec2f;
            dataOffset = rlogic_serialization::CreatefloatArr(builder, builder.CreateVector(flattenArrayOfVec<vec2f, float>(data.m_data))).Union();
            break;
        case EPropertyType::Vec3f:
            unionType = rlogic_serialization::ArrayUnion::floatArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec3f;
            dataOffset = rlogic_serialization::CreatefloatArr(builder, builder.CreateVector(flattenArrayOfVec<vec3f, float>(data.m_data))).Union();
            break;
        case EPropertyType::Vec4f:
            unionType = rlogic_serialization::ArrayUnion::floatArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec4f;
            dataOffset = rlogic_serialization::CreatefloatArr(builder, builder.CreateVector(flattenArrayOfVec<vec4f, float>(data.m_data))).Union();
            break;
        case EPropertyType::Int32:
            unionType = rlogic_serialization::ArrayUnion::intArr;
            arrayType = rlogic_serialization::EDataArrayType::Int32;
            dataOffset = rlogic_serialization::CreateintArr(builder, builder.CreateVector(std::get<std::vector<int32_t>>(data.m_data))).Union();
            break;
        case EPropertyType::Vec2i:
            unionType = rlogic_serialization::ArrayUnion::intArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec2i;
            dataOffset = rlogic_serialization::CreateintArr(builder, builder.CreateVector(flattenArrayOfVec<vec2i, int32_t>(data.m_data))).Union();
            break;
        case EPropertyType::Vec3i:
            unionType = rlogic_serialization::ArrayUnion::intArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec3i;
            dataOffset = rlogic_serialization::CreateintArr(builder, builder.CreateVector(flattenArrayOfVec<vec3i, int32_t>(data.m_data))).Union();
            break;
        case EPropertyType::Vec4i:
            unionType = rlogic_serialization::ArrayUnion::intArr;
            arrayType = rlogic_serialization::EDataArrayType::Vec4i;
            dataOffset = rlogic_serialization::CreateintArr(builder, builder.CreateVector(flattenArrayOfVec<vec4i, int32_t>(data.m_data))).Union();
            break;
        case EPropertyType::Bool:
        default:
            assert(!"missing implementation");
            break;
        }

        auto animDataFB = rlogic_serialization::CreateDataArray(
            builder,
            builder.CreateString(data.getName()),
            data.getId(),
            arrayType,
            unionType,
            dataOffset
        );

        return animDataFB;
    }

    template <typename T, typename fbT, typename fbArrayT>
    bool checkFlatbufferVectorValidity(const rlogic_serialization::DataArray& data, ErrorReporting& errorReporting)
    {
        if (!data.data_as<fbArrayT>() || !data.data_as<fbArrayT>()->data())
        {
            errorReporting.add("Fatal error during loading of DataArray from serialized data: unexpected data type!", nullptr);
            return false;
        }
        const auto fbVec = data.data_as<fbArrayT>()->data();
        static_assert(std::is_arithmetic_v<fbT>, "wrong base type used");
        if (fbVec->size() % getNumElements<T>() != 0)
        {
            errorReporting.add("Fatal error during loading of DataArray from serialized data: unexpected data size!", nullptr);
            return false;
        }

        return true;
    }

    template <typename T, typename fbT>
    std::vector<T> unflattenIntoArrayOfVec(const flatbuffers::Vector<fbT>& fbDataFlattened)
    {
        constexpr size_t numElementsInT = getNumElements<T>();
        std::vector<T> dataVec;
        dataVec.resize(fbDataFlattened.size() / numElementsInT);
        auto destIt = dataVec.begin();
        for (auto it = fbDataFlattened.cbegin(); it != fbDataFlattened.cend(); it += numElementsInT, ++destIt)
            std::copy(it, it + numElementsInT, destIt->data());
        return dataVec;
    }

    std::unique_ptr<DataArrayImpl> DataArrayImpl::Deserialize(const rlogic_serialization::DataArray& data, ErrorReporting& errorReporting)
    {
        if (data.id() == 0u)
        {
            errorReporting.add("Fatal error during loading of DataArray from serialized data: missing id!", nullptr);
            return nullptr;
        }

        if (!data.name())
        {
            errorReporting.add("Fatal error during loading of DataArray from serialized data: missing name!", nullptr);
            return nullptr;
        }

        const auto name = data.name()->string_view();

        switch (data.type())
        {
        case rlogic_serialization::EDataArrayType::Float:
        {
            if (!checkFlatbufferVectorValidity<float, float, rlogic_serialization::floatArr>(data, errorReporting))
                return nullptr;
            const auto& fbData = *data.data_as<rlogic_serialization::floatArr>()->data();
            auto dataVec = std::vector<float>{ fbData.cbegin(), fbData.cend() };
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec2f:
        {
            if (!checkFlatbufferVectorValidity<vec2f, float, rlogic_serialization::floatArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec2f, float>(*data.data_as<rlogic_serialization::floatArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec3f:
        {
            if (!checkFlatbufferVectorValidity<vec3f, float, rlogic_serialization::floatArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec3f, float>(*data.data_as<rlogic_serialization::floatArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec4f:
        {
            if (!checkFlatbufferVectorValidity<vec4f, float, rlogic_serialization::floatArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec4f, float>(*data.data_as<rlogic_serialization::floatArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Int32:
        {
            if (!checkFlatbufferVectorValidity<int32_t, int32_t, rlogic_serialization::intArr>(data, errorReporting))
                return nullptr;
            const auto& fbData = *data.data_as<rlogic_serialization::intArr>()->data();
            auto dataVec = std::vector<int32_t>{ fbData.cbegin(), fbData.cend() };
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec2i:
        {
            if (!checkFlatbufferVectorValidity<vec2i, int32_t, rlogic_serialization::intArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec2i, int32_t>(*data.data_as<rlogic_serialization::intArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec3i:
        {
            if (!checkFlatbufferVectorValidity<vec3i, int32_t, rlogic_serialization::intArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec3i, int32_t>(*data.data_as<rlogic_serialization::intArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        case rlogic_serialization::EDataArrayType::Vec4i:
        {
            if (!checkFlatbufferVectorValidity<vec4i, int32_t, rlogic_serialization::intArr>(data, errorReporting))
                return nullptr;
            auto dataVec = unflattenIntoArrayOfVec<vec4i, int32_t>(*data.data_as<rlogic_serialization::intArr>()->data());
            return std::make_unique<DataArrayImpl>(std::move(dataVec), name, data.id());
        }
        default:
            errorReporting.add(fmt::format("Fatal error during loading of DataArray from serialized data: unsupported or corrupt data type '{}'!", data.type()), nullptr);
            return nullptr;
        }
    }

    size_t DataArrayImpl::getNumElements() const
    {
        size_t numElements = 0u;
        std::visit([&numElements](const auto& v) { numElements = v.size(); }, m_data);
        return numElements;
    }

    const DataArrayImpl::DataArrayVariant& DataArrayImpl::getDataVariant() const
    {
        return m_data;
    }

    template DataArrayImpl::DataArrayImpl(std::vector<float>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec2f>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec3f>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec4f>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<int32_t>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec2i>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec3i>&& data, std::string_view name, uint64_t id);
    template DataArrayImpl::DataArrayImpl(std::vector<vec4i>&& data, std::string_view name, uint64_t id);

    template const std::vector<float>* DataArrayImpl::getData<float>() const;
    template const std::vector<vec2f>* DataArrayImpl::getData<vec2f>() const;
    template const std::vector<vec3f>* DataArrayImpl::getData<vec3f>() const;
    template const std::vector<vec4f>* DataArrayImpl::getData<vec4f>() const;
    template const std::vector<int32_t>* DataArrayImpl::getData<int32_t>() const;
    template const std::vector<vec2i>* DataArrayImpl::getData<vec2i>() const;
    template const std::vector<vec3i>* DataArrayImpl::getData<vec3i>() const;
    template const std::vector<vec4i>* DataArrayImpl::getData<vec4i>() const;
}
