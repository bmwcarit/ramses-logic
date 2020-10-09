//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/impl/RamsesBindingImpl.h"

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
        ~RamsesNodeBindingImpl() noexcept override = default;
        RamsesNodeBindingImpl(RamsesNodeBindingImpl&& other) noexcept = default;
        RamsesNodeBindingImpl& operator=(RamsesNodeBindingImpl&& other) noexcept = default;
        RamsesNodeBindingImpl(const RamsesNodeBindingImpl& other)                = delete;
        RamsesNodeBindingImpl& operator=(const RamsesNodeBindingImpl& other) = delete;
        static std::unique_ptr<RamsesNodeBindingImpl> Create(std::string_view name);
        static std::unique_ptr<RamsesNodeBindingImpl> Create(const rlogic_serialization::RamsesNodeBinding& nodeBinding, ramses::Node* ramsesNode);

        bool setRamsesNode(ramses::Node* node);
        ramses::Node* getRamsesNode() const;
        bool update() override;

        flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding> serialize(flatbuffers::FlatBufferBuilder& builder) const;

    private:

        explicit RamsesNodeBindingImpl(std::string_view name) noexcept;
        RamsesNodeBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Node* ramsesNode) noexcept;
        ramses::Node* m_ramsesNode = nullptr;

        static std::unique_ptr<PropertyImpl> CreateNodeProperties();
    };
}
