//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LogicNodeImpl.h"

namespace ramses
{
    class SceneObject;
}

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic_serialization
{
    struct RamsesReference;
}

namespace rlogic::internal
{
    class RamsesBindingImpl : public LogicNodeImpl
    {
    public:
        // This is a base class, only deleted functions are public
        RamsesBindingImpl(const RamsesBindingImpl& other) = delete;
        RamsesBindingImpl& operator=(const RamsesBindingImpl& other) = delete;
    protected:
        // Move-able (noexcept); Not copy-able
        explicit RamsesBindingImpl(std::string_view name) noexcept;
        ~RamsesBindingImpl() noexcept override                           = default;
        RamsesBindingImpl(RamsesBindingImpl&& other) noexcept            = default;
        RamsesBindingImpl& operator=(RamsesBindingImpl&& other) noexcept = default;

        // Used by subclasses to handle serialization
        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::RamsesReference> SerializeRamsesReference(const ramses::SceneObject& object, flatbuffers::FlatBufferBuilder& builder);

        // TODO Violin consider moving pointer(s) to ramses objects here and add templated getters for subclasses
    };
}
