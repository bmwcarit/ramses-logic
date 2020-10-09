//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/impl/RamsesBindingImpl.h"

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

    class RamsesAppearanceBindingImpl : public RamsesBindingImpl
    {
    public:
        static std::unique_ptr<RamsesAppearanceBindingImpl> Create(std::string_view name);
        static std::unique_ptr<RamsesAppearanceBindingImpl> Create(const rlogic_serialization::RamsesAppearanceBinding& appearanceBinding, ramses::Appearance* appearance, std::vector<std::string>& errorsOut);

        void setRamsesAppearance(ramses::Appearance* appearance);
        ramses::Appearance* getRamsesAppearance() const;

        bool update() override;

        flatbuffers::Offset<rlogic_serialization::RamsesAppearanceBinding> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    private:
        explicit RamsesAppearanceBindingImpl(std::string_view name);
        RamsesAppearanceBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Appearance& appearance);

        ramses::Appearance* m_appearance;
        // TODO find a solution to directly store ramses::UniformInput inside containers
        // If this is done, think about using a standard vector for storing ramses::UniformInputs
        std::unordered_map<PropertyImpl*, std::unique_ptr<ramses::UniformInput>> m_propertyToUniformInput;

        void setInputValueToUniform(PropertyImpl* property);
        static bool AppearanceCompatibleWithDeserializedInputs(PropertyImpl& deserializedInputs, ramses::Appearance& appearance, std::vector<std::string>& errorsOut);
        void populatePropertyMappingCache(ramses::Appearance& appearance);
        void createInputProperties(ramses::Appearance& appearance);
    };
}
