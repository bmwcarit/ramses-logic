//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/RamsesBindingImpl.h"
#include "ramses-client-api/ERotationConvention.h"

#include <memory>

namespace ramses
{
    class Scene;
    class Node;
}

namespace rlogic_serialization
{
    struct RamsesNodeBinding;
}

namespace flatbuffers
{
    class FlatBufferBuilder;
    template<typename T> struct Offset;
}

namespace rlogic::internal
{
    enum class ENodePropertyStaticIndex : size_t
    {
        Visibility = 0,
        Rotation = 1,
        Translation = 2,
        Scaling = 3,
    };

    class RamsesNodeBindingImpl : public RamsesBindingImpl
    {
    public:
        // Move-able (noexcept); Not copy-able
        explicit RamsesNodeBindingImpl(std::string_view name) noexcept;
        ~RamsesNodeBindingImpl() noexcept override = default;
        RamsesNodeBindingImpl(RamsesNodeBindingImpl&& other) noexcept = default;
        RamsesNodeBindingImpl& operator=(RamsesNodeBindingImpl&& other) noexcept = default;
        RamsesNodeBindingImpl(const RamsesNodeBindingImpl& other)                = delete;
        RamsesNodeBindingImpl& operator=(const RamsesNodeBindingImpl& other) = delete;

        // TODO Violin we can remove this, Bindings don't need to deserialize their values after recent rework!
        RamsesNodeBindingImpl(std::string_view name, ramses::ERotationConvention rotationConvention, std::unique_ptr<PropertyImpl> inputs, ramses::Node* ramsesNode) noexcept;
        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding> Serialize(const RamsesNodeBindingImpl& nodeBinding, flatbuffers::FlatBufferBuilder& builder);
        [[nodiscard]] static std::unique_ptr<RamsesNodeBindingImpl> Deserialize(const rlogic_serialization::RamsesNodeBinding& nodeBinding, ramses::Node* ramsesNode);

        bool setRamsesNode(ramses::Node* node);
        [[nodiscard]] ramses::Node* getRamsesNode() const;

        bool setRotationConvention(ramses::ERotationConvention rotationConvention);
        [[nodiscard]] ramses::ERotationConvention getRotationConvention() const;

        std::optional<LogicNodeRuntimeError> update() override;


    private:
        ramses::Node* m_ramsesNode = nullptr;
        ramses::ERotationConvention m_rotationConvention = ramses::ERotationConvention::XYZ;

        [[nodiscard]] static std::unique_ptr<PropertyImpl> CreateNodeProperties();
    };
}
