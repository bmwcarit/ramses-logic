//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesAppearanceBindingImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/LoggerImpl.h"

#include "ramses-client-api/UniformInput.h"

#include "internals/RamsesHelper.h"
#include "internals/ErrorReporting.h"
#include "internals/TypeUtils.h"

#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "generated/ramsesappearancebinding_gen.h"

#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Effect.h"

namespace rlogic::internal
{
    RamsesAppearanceBindingImpl::RamsesAppearanceBindingImpl(std::string_view name)
        : RamsesBindingImpl(name, std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::BindingInput), nullptr)
        , m_ramsesAppearance(nullptr)
    {
    }

    RamsesAppearanceBindingImpl::RamsesAppearanceBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Appearance& appearance)
        : RamsesBindingImpl(name, std::move(inputs), nullptr)
        , m_ramsesAppearance(&appearance)
    {
    }

    flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding> RamsesAppearanceBindingImpl::Serialize(const RamsesAppearanceBindingImpl& binding, flatbuffers::FlatBufferBuilder& builder)
    {
        ramses::sceneObjectId_t appearanceId;
        if (binding.m_ramsesAppearance != nullptr)
        {
            appearanceId = binding.m_ramsesAppearance->getSceneObjectId();
        }
        auto ramsesAppearanceBinding = rlogic_serialization::CreateRamsesAppearanceBinding(
            builder,
            LogicNodeImpl::Serialize(binding, builder), // Serialize base class
            appearanceId.getValue());
        builder.Finish(ramsesAppearanceBinding);

        return ramsesAppearanceBinding;
    }

    std::unique_ptr<RamsesAppearanceBindingImpl> RamsesAppearanceBindingImpl::Deserialize(
        const rlogic_serialization::RamsesAppearanceBinding& appearanceBinding,
        ramses::Appearance* appearance,
        ErrorReporting& errorReporting)
    {
        assert(nullptr != appearanceBinding.logicnode());
        assert(nullptr != appearanceBinding.logicnode()->name());
        assert(nullptr != appearanceBinding.logicnode()->inputs());

        std::unique_ptr<PropertyImpl> inputsImpl = PropertyImpl::Deserialize(*appearanceBinding.logicnode()->inputs(), EPropertySemantics::BindingInput);
        if (nullptr != appearance)
        {
            if (!AppearanceCompatibleWithDeserializedInputs(*inputsImpl, *appearance, errorReporting))
            {
                return nullptr;
            }
            // TODO Violin redesign this (currently not possible because of impl restrictions)
            std::unique_ptr<RamsesAppearanceBindingImpl> implWhichNeedsUpdating(new RamsesAppearanceBindingImpl(
                appearanceBinding.logicnode()->name()->string_view(),
                std::move(inputsImpl),
                *appearance));
            implWhichNeedsUpdating->populatePropertyMappingCache(*appearance);
            return implWhichNeedsUpdating;
        }

        if (inputsImpl->getChildCount() != 0u)
        {
            errorReporting.add(fmt::format("Fatal error while loading from file: appearance binding (name: {}) has stored inputs, but a ramses appearance (id: {}) could not be resolved",
                appearanceBinding.logicnode()->name()->string_view(),
                appearanceBinding.ramsesAppearance()));
            return nullptr;
        }
        return std::make_unique<RamsesAppearanceBindingImpl>(appearanceBinding.logicnode()->name()->string_view());
    }

    bool RamsesAppearanceBindingImpl::UniformTypeMatchesBindingInputType(const ramses::UniformInput& uniformInput, const PropertyImpl& bindingInput, ErrorReporting& errorReporting)
    {
        EPropertyType bindingInputType = bindingInput.getType();
        if (!TypeUtils::IsPrimitiveType(bindingInputType))
        {
            // Only arrays supported currently
            assert(bindingInputType == EPropertyType::Array);

            const uint32_t uniformArraySize = uniformInput.getElementCount();
            if (uniformArraySize != bindingInput.getChildCount())
            {
                errorReporting.add(fmt::format("Fatal error while loading from file: ramses appearance binding input (Name: {}) is expected to be an array of size {}, but instead it has size {}!)",
                    bindingInput.getName(), uniformArraySize, bindingInput.getChildCount()));
                return false;
            }

            // Take type of first element to compare types with the ramses uniform
            bindingInputType = bindingInput.getChild(0)->getType();
        }

        const std::optional<EPropertyType> possiblySupportedUniformType = ConvertRamsesUniformTypeToPropertyType(uniformInput.getDataType());
        if (!possiblySupportedUniformType || bindingInputType != *possiblySupportedUniformType)
        {
            const std::string typeMismatchLog = possiblySupportedUniformType ? GetLuaPrimitiveTypeName(*possiblySupportedUniformType) : "unsupported type";
            errorReporting.add(fmt::format("Fatal error while loading from file: ramses appearance binding input (Name: {}) is expected to be of type {}, but instead it is {}!)",
                bindingInput.getName(), GetLuaPrimitiveTypeName(bindingInputType), typeMismatchLog));
            return false;
        }

        return true;
    }

    bool RamsesAppearanceBindingImpl::AppearanceCompatibleWithDeserializedInputs(PropertyImpl& deserializedInputs, ramses::Appearance& appearance, ErrorReporting& errorReporting)
    {
        const ramses::Effect& effect = appearance.getEffect();

        for (size_t i = 0; i < deserializedInputs.getChildCount(); ++i)
        {
            const PropertyImpl& input = *deserializedInputs.getChild(i)->m_impl;
            ramses::UniformInput uniformInput;
            if (ramses::StatusOK != effect.findUniformInput(input.getName().data(), uniformInput))
            {
                errorReporting.add(fmt::format("Fatal error while loading from file: ramses appearance binding input (Name: {}) was not found in appearance '{}'!)",
                    input.getName(), appearance.getName()));
                return false;
            }

            if (!UniformTypeMatchesBindingInputType(uniformInput, input, errorReporting))
            {
                return false;
            }
        }

        return true;
    }

    std::optional<LogicNodeRuntimeError> RamsesAppearanceBindingImpl::update()
    {
        // Early exit if appearance is not set
        if (m_ramsesAppearance)
        {
            const size_t childCount = getInputs()->getChildCount();
            for (size_t i = 0; i < childCount; ++i)
            {
                setInputValueToUniform(i);
            }
        }

        return std::nullopt;
    }

    void RamsesAppearanceBindingImpl::setInputValueToUniform(size_t inputIndex)
    {
        PropertyImpl& inputProperty = *getInputs()->getChild(inputIndex)->m_impl;
        const EPropertyType propertyType = inputProperty.getType();

        if (TypeUtils::IsPrimitiveType(propertyType))
        {
            if (inputProperty.checkForBindingInputNewValueAndReset())
            {
                ramses::UniformInput uniform;
                m_ramsesAppearance->getEffect().getUniformInput(m_uniformIndices[inputIndex], uniform);

                switch (propertyType)
                {
                case EPropertyType::Float:
                    m_ramsesAppearance->setInputValueFloat(uniform, *inputProperty.get<float>());
                    break;
                case EPropertyType::Int32:
                    m_ramsesAppearance->setInputValueInt32(uniform, *inputProperty.get<int32_t>());
                    break;
                case EPropertyType::Vec2f:
                {
                    const auto vec = *inputProperty.get<vec2f>();
                    m_ramsesAppearance->setInputValueVector2f(uniform, vec[0], vec[1]);
                    break;
                }
                case EPropertyType::Vec2i:
                {
                    const auto vec = *inputProperty.get<vec2i>();
                    m_ramsesAppearance->setInputValueVector2i(uniform, vec[0], vec[1]);
                    break;
                }
                case EPropertyType::Vec3f:
                {
                    const auto vec = *inputProperty.get<vec3f>();
                    m_ramsesAppearance->setInputValueVector3f(uniform, vec[0], vec[1], vec[2]);
                    break;
                }
                case EPropertyType::Vec3i:
                {
                    const auto vec = *inputProperty.get<vec3i>();
                    m_ramsesAppearance->setInputValueVector3i(uniform, vec[0], vec[1], vec[2]);
                    break;
                }
                case EPropertyType::Vec4f:
                {
                    const auto vec = *inputProperty.get<vec4f>();
                    m_ramsesAppearance->setInputValueVector4f(uniform, vec[0], vec[1], vec[2], vec[3]);
                    break;
                }
                case EPropertyType::Vec4i:
                {
                    const auto vec = *inputProperty.get<vec4i>();
                    m_ramsesAppearance->setInputValueVector4i(uniform, vec[0], vec[1], vec[2], vec[3]);
                    break;
                }
                case EPropertyType::String:
                case EPropertyType::Array:
                case EPropertyType::Struct:
                case EPropertyType::Bool:
                    assert(false && "This should never happen");
                    break;
                }
            }
        }
        else
        {
            assert(propertyType == EPropertyType::Array);

            // A new value on any of the array element causes the whole array to be updated
            // Ramses does not allow partial updates so this is the only option here
            bool anyArrayElementWasSet = false;
            const size_t arraySize = inputProperty.getChildCount();
            for (size_t i = 0; i < arraySize; ++i)
            {
                if (inputProperty.getChild(i)->m_impl->checkForBindingInputNewValueAndReset())
                {
                    anyArrayElementWasSet = true;
                }
            }

            if (anyArrayElementWasSet)
            {
                ramses::UniformInput uniform;
                m_ramsesAppearance->getEffect().getUniformInput(m_uniformIndices[inputIndex], uniform);

                const EPropertyType arrayElementType = inputProperty.getChild(0)->getType();
                switch (arrayElementType)
                {
                case EPropertyType::Float:
                    m_ramsesAppearance->setInputValueFloat(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<float, float>(inputProperty).data());
                    break;
                case EPropertyType::Int32:
                    m_ramsesAppearance->setInputValueInt32(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<int32_t, int32_t>(inputProperty).data());
                    break;
                case EPropertyType::Vec2f:
                    m_ramsesAppearance->setInputValueVector2f(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<float, vec2f>(inputProperty).data());
                    break;
                case EPropertyType::Vec2i:
                    m_ramsesAppearance->setInputValueVector2i(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<int32_t, vec2i>(inputProperty).data());
                    break;
                case EPropertyType::Vec3f:
                    m_ramsesAppearance->setInputValueVector3f(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<float, vec3f>(inputProperty).data());
                    break;
                case EPropertyType::Vec3i:
                    m_ramsesAppearance->setInputValueVector3i(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<int32_t, vec3i>(inputProperty).data());
                    break;
                case EPropertyType::Vec4f:
                    m_ramsesAppearance->setInputValueVector4f(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<float, vec4f>(inputProperty).data());
                    break;
                case EPropertyType::Vec4i:
                    m_ramsesAppearance->setInputValueVector4i(uniform, static_cast<uint32_t>(arraySize), TypeUtils::FlattenArrayData<int32_t, vec4i>(inputProperty).data());
                    break;
                case EPropertyType::String:
                case EPropertyType::Array:
                case EPropertyType::Struct:
                case EPropertyType::Bool:
                    assert(false && "This should never happen");
                    break;
                }
            }
        }
    }

    // TODO Violin There is code-duplication between this method and populatePropertyMappingCache further down which is very error-prone
    void RamsesAppearanceBindingImpl::createInputProperties(ramses::Appearance& appearance)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        const auto& effect = appearance.getEffect();
        const uint32_t uniformCount = effect.getUniformInputCount();

        for (uint32_t i = 0; i < uniformCount; ++i)
        {
            ramses::UniformInput uniformInput;
            // TODO Violin maybe need error handling here, to be super safe
            effect.getUniformInput(i, uniformInput);

            const auto convertedType = ConvertRamsesUniformTypeToPropertyType(uniformInput.getDataType());

            // TODO Violin consider making it an error (or warning?) when using appearance with unsupported input types
            if (convertedType)
            {
                // Can't bind semantic uniforms
                if (uniformInput.getSemantics() == ramses::EEffectUniformSemantic::Invalid)
                {
                    // Non-array case
                    if (uniformInput.getElementCount() == 1)
                    {
                        inputsImpl.addChild(std::make_unique<PropertyImpl>(uniformInput.getName(), *convertedType, EPropertySemantics::BindingInput));
                    }
                    // Array case
                    else
                    {
                        inputsImpl.addChild(std::make_unique<PropertyImpl>(uniformInput.getName(), EPropertyType::Array, EPropertySemantics::BindingInput));
                        PropertyImpl& arrayInputImpl = *inputsImpl.getChild(inputsImpl.getChildCount() - 1)->m_impl;
                        const size_t arraySize = uniformInput.getElementCount();

                        for (size_t arrayElem = 0; arrayElem < arraySize; ++arrayElem)
                        {
                            arrayInputImpl.addChild(std::make_unique<PropertyImpl>("", *convertedType, EPropertySemantics::BindingInput));
                        }
                    }
                }
            }
        }

        populatePropertyMappingCache(appearance);
    }

    // TODO Violin Having to manage another mapping is dangerous and bug-prone. Investigate other options
    // The name of this method is a bit ugly, but reflects exactly what it does
    void RamsesAppearanceBindingImpl::populatePropertyMappingCache(ramses::Appearance& appearance)
    {
        m_uniformIndices.clear();

        auto& effect = appearance.getEffect();
        const uint32_t uniformCount = effect.getUniformInputCount();
        m_uniformIndices.reserve(getInputs()->getChildCount());

        for (uint32_t i = 0; i < uniformCount; ++i)
        {
            ramses::UniformInput uniformInput;
            effect.getUniformInput(i, uniformInput);

            const auto convertedType = ConvertRamsesUniformTypeToPropertyType(uniformInput.getDataType());
            if (convertedType && uniformInput.getSemantics() == ramses::EEffectUniformSemantic::Invalid)
            {
                m_uniformIndices.push_back(i);
            }
        }
    }

    void RamsesAppearanceBindingImpl::setRamsesAppearance(ramses::Appearance* appearance)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        inputsImpl.clearChildren();

        m_ramsesAppearance = appearance;

        if (nullptr != m_ramsesAppearance)
        {
            createInputProperties(*appearance);
        }
    }

    ramses::Appearance* RamsesAppearanceBindingImpl::getRamsesAppearance() const
    {
        return m_ramsesAppearance;
    }
}
