//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/RamsesBindingImpl.h"

#include "ramses-client-api/UniformInput.h"

#include <memory>
#include <unordered_map>

namespace ramses
{
    class Appearance;
    class UniformInput;
}

namespace rlogic_serialization
{
    struct RamsesAppearanceBinding;
}

namespace flatbuffers
{
    class FlatBufferBuilder;
    template <typename T> struct Offset;
}

namespace rlogic::internal
{
    class PropertyImpl;
    class ErrorReporting;

    class RamsesAppearanceBindingImpl : public RamsesBindingImpl
    {
    public:
        explicit RamsesAppearanceBindingImpl(std::string_view name);
        // Special constructor used when deserialized with appearance reference and inputs have pre-populated values
        // TODO Violin we can remove this, Bindings don't need to deserialize their values after recent rework!
        RamsesAppearanceBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Appearance& appearance);
        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding> Serialize(const RamsesAppearanceBindingImpl& binding, flatbuffers::FlatBufferBuilder& builder);
        [[nodiscard]] static std::unique_ptr<RamsesAppearanceBindingImpl> Deserialize(const rlogic_serialization::RamsesAppearanceBinding& appearanceBinding, ramses::Appearance* appearance, ErrorReporting& errorReporting);

        void setRamsesAppearance(ramses::Appearance* appearance);
        [[nodiscard]] ramses::Appearance* getRamsesAppearance() const;

        std::optional<LogicNodeRuntimeError> update() override;

    private:

        ramses::Appearance* m_appearance;
        // TODO Violin find a solution to directly store ramses::UniformInput inside containers
        // If this is done, think about using a standard vector for storing ramses::UniformInputs
        std::unordered_map<const PropertyImpl*, std::unique_ptr<ramses::UniformInput>> m_propertyToUniformInput;

        void setInputValueToUniform(PropertyImpl& property);
        [[nodiscard]] static bool UniformTypeMatchesBindingInputType(const ramses::UniformInput& uniformInput, const PropertyImpl& bindingInput, ErrorReporting& errorReporting);
        [[nodiscard]] static bool AppearanceCompatibleWithDeserializedInputs(PropertyImpl& deserializedInputs, ramses::Appearance& appearance, ErrorReporting& errorReporting);
        void populatePropertyMappingCache(ramses::Appearance& appearance);
        void createInputProperties(ramses::Appearance& appearance);
    };
}
