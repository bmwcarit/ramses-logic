//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/RamsesAppearanceBindingImpl.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/impl/LoggerImpl.h"

#include "ramses-client-api/UniformInput.h"

#include "internals/RamsesHelper.h"

#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "generated/ramsesappearancebinding_gen.h"

#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Effect.h"

namespace rlogic::internal
{
    RamsesAppearanceBindingImpl::RamsesAppearanceBindingImpl(std::string_view name)
        : RamsesBindingImpl(name, std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EInputOutputProperty::Input), nullptr)
        , m_appearance(nullptr)
    {
    }

    RamsesAppearanceBindingImpl::RamsesAppearanceBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Appearance& appearance)
        : RamsesBindingImpl(name, std::move(inputs), nullptr)
        , m_appearance(&appearance)
    {
    }

    std::unique_ptr<RamsesAppearanceBindingImpl> RamsesAppearanceBindingImpl::Create(std::string_view name)
    {
        // We can't use std::make_unique here, because the constructor is private
        return std::unique_ptr<RamsesAppearanceBindingImpl>(new RamsesAppearanceBindingImpl(name));
    }

    bool RamsesAppearanceBindingImpl::AppearanceCompatibleWithDeserializedInputs(PropertyImpl& deserializedInputs, ramses::Appearance& appearance, std::vector<std::string>& errorsOut)
    {
        const ramses::Effect& effect = appearance.getEffect();

        for (size_t i = 0; i < deserializedInputs.getChildCount(); ++i)
        {
            const auto input = deserializedInputs.getChild(i);
            ramses::UniformInput uniformInput;
            if (ramses::StatusOK != effect.findUniformInput(input->getName().data(), uniformInput))
            {
                errorsOut.emplace_back(fmt::format("Fatal error while loading from file: ramses appearance binding input (Name: {}) was not found in appearance '{}'!)",
                    input->getName(), appearance.getName()));
                return false;
            }
            const auto maybeSupportedUniformType = ConvertRamsesUniformTypeToPropertyType(uniformInput.getDataType());
            if (!maybeSupportedUniformType || input->getType() != *maybeSupportedUniformType)
            {
                const std::string typeMismatchLog = maybeSupportedUniformType ? GetLuaPrimitiveTypeName(*maybeSupportedUniformType) : "unsupported type";
                errorsOut.emplace_back(fmt::format("Fatal error while loading from file: ramses appearance binding input (Name: {}) is expected to be of type {}, but instead it is {}!)",
                    input->getName(), GetLuaPrimitiveTypeName(input->getType()), typeMismatchLog));
                return false;
            }
        }

        return true;
    }

    std::unique_ptr<RamsesAppearanceBindingImpl> RamsesAppearanceBindingImpl::Create(const rlogic_serialization::RamsesAppearanceBinding& appearanceBinding, ramses::Appearance* appearance, std::vector<std::string>& errorsOut)
    {
        // TODO Test with large scene how much overhead it is to store lots of bindings with empty names
        assert(nullptr != appearanceBinding.logicnode());
        assert(nullptr != appearanceBinding.logicnode()->name());
        assert(nullptr != appearanceBinding.logicnode()->inputs());

        std::unique_ptr<PropertyImpl> inputsImpl = PropertyImpl::Create(appearanceBinding.logicnode()->inputs(), EInputOutputProperty::Input);
        if (nullptr != appearance)
        {
            if (!AppearanceCompatibleWithDeserializedInputs(*inputsImpl, *appearance, errorsOut))
            {
                return nullptr;
            }
            // TODO Violin code needs more redesign here
            auto implWhichNeedsUpdating = std::unique_ptr<RamsesAppearanceBindingImpl>(new RamsesAppearanceBindingImpl(appearanceBinding.logicnode()->name()->string_view(), std::move(inputsImpl), *appearance));
            implWhichNeedsUpdating->populatePropertyMappingCache(*appearance);
            return implWhichNeedsUpdating;
        }

        if (inputsImpl->getChildCount() != 0u)
        {
            errorsOut.emplace_back(fmt::format("Fatal error while loading from file: appearance binding (name: {}) has stored inputs, but a ramses appearance (id: {}) could not be resolved",
                appearanceBinding.logicnode()->name()->string_view(),
                appearanceBinding.ramsesAppearance()));
            return nullptr;
        }
        return std::unique_ptr<RamsesAppearanceBindingImpl>(new RamsesAppearanceBindingImpl(appearanceBinding.logicnode()->name()->string_view()));
    }

    bool RamsesAppearanceBindingImpl::update()
    {
        const auto inputs = getInputs();
        if (nullptr != inputs)
        {
            const auto childCount = inputs->getChildCount();
            for (uint32_t i = 0; i < childCount; ++i)
            {
                auto child = inputs->getChild(i);
                if (child->m_impl->wasSet())
                {
                    setInputValueToUniform(child->m_impl.get());
                }
            }
        }

        return true;
    }

    void RamsesAppearanceBindingImpl::setInputValueToUniform(PropertyImpl* property)
    {
        auto it = m_propertyToUniformInput.find(property);
        assert(it != m_propertyToUniformInput.end() && "Searched for a Uniform by Property in RamsesAppearanceBindingImpl. This should never happen");

        switch (property->getType())
        {
        case EPropertyType::Float:
            m_appearance->setInputValueFloat(*it->second, *property->get<float>());
            break;
        case EPropertyType::Int32:
            m_appearance->setInputValueInt32(*it->second, *property->get<int32_t>());
            break;
        case EPropertyType::Vec2f:
        {
            const auto vec = *property->get<vec2f>();
            m_appearance->setInputValueVector2f(*it->second, vec[0], vec[1]);
            break;
        }
        case EPropertyType::Vec2i:
        {
            const auto vec = *property->get<vec2i>();
            m_appearance->setInputValueVector2i(*it->second, vec[0], vec[1]);
            break;
        }
        case EPropertyType::Vec3f:
        {
            const auto vec = *property->get<vec3f>();
            m_appearance->setInputValueVector3f(*it->second, vec[0], vec[1], vec[2]);
            break;
        }
        case EPropertyType::Vec3i:
        {
            const auto vec = *property->get<vec3i>();
            m_appearance->setInputValueVector3i(*it->second, vec[0], vec[1], vec[2]);
            break;
        }
        case EPropertyType::Vec4f:
        {
            const auto vec = *property->get<vec4f>();
            m_appearance->setInputValueVector4f(*it->second, vec[0], vec[1], vec[2], vec[3]);
            break;
        }
        case EPropertyType::Vec4i:
        {
            const auto vec = *property->get<vec4i>();
            m_appearance->setInputValueVector4i(*it->second, vec[0], vec[1], vec[2], vec[3]);
            break;
        }
        case EPropertyType::String:
        case EPropertyType::Struct:
        case EPropertyType::Bool:
            assert(false && "This should never happen");
            break;
        }
    }

    // TODO Violin This can be refactored... But first need to make it work bug-free
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
            if (convertedType)
            {
                inputsImpl.addChild(std::make_unique<PropertyImpl>(uniformInput.getName(), *convertedType, EInputOutputProperty::Input));
            }
        }

        populatePropertyMappingCache(appearance);
    }

    // TODO Violin Having to manage another mapping is dangerous and bug-prone. Investigate other options
    // The name of this method is a bit ugly, but reflects exactly what it does
    void RamsesAppearanceBindingImpl::populatePropertyMappingCache(ramses::Appearance& appearance)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        auto& effect = appearance.getEffect();
        const uint32_t uniformCount = effect.getUniformInputCount();

        for (uint32_t i = 0; i < uniformCount; ++i)
        {
            // TODO Violin this is an allocation which can be easily optimized. Investigate why UniformInput can't be moved in ramses
            auto uniformInput = std::make_unique<ramses::UniformInput>();
            effect.getUniformInput(i, *uniformInput);

            const auto convertedType = ConvertRamsesUniformTypeToPropertyType(uniformInput->getDataType());
            if (convertedType)
            {
                Property* maybeProperty = inputsImpl.getChild(uniformInput->getName());
                assert(nullptr != maybeProperty && "This should never happen");
                m_propertyToUniformInput.insert(std::make_pair(maybeProperty->m_impl.get(), std::move(uniformInput)));
            }
        }
    }

    void RamsesAppearanceBindingImpl::setRamsesAppearance(ramses::Appearance* appearance)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        inputsImpl.clearChildren();
        m_propertyToUniformInput.clear();

        m_appearance = appearance;

        if (nullptr != m_appearance)
        {
            createInputProperties(*appearance);
        }
    }

    ramses::Appearance* RamsesAppearanceBindingImpl::getRamsesAppearance() const
    {
        return m_appearance;
    }

    flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding> RamsesAppearanceBindingImpl::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        ramses::sceneObjectId_t appearanceId;
        if (m_appearance != nullptr)
        {
            appearanceId = m_appearance->getSceneObjectId();
        }
        auto ramsesAppearanceBinding = rlogic_serialization::CreateRamsesAppearanceBinding(
            builder,
            LogicNodeImpl::serialize(builder),
            appearanceId.getValue());
        builder.Finish(ramsesAppearanceBinding);

        return ramsesAppearanceBinding;
    }
}
