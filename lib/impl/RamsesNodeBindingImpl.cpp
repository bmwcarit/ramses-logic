//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesNodeBindingImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

#include "ramses-client-api/Node.h"

#include "generated/ramsesnodebinding_gen.h"

namespace rlogic::internal
{
    RamsesNodeBindingImpl::RamsesNodeBindingImpl(std::string_view name) noexcept
        : RamsesBindingImpl(name, CreateNodeProperties(), nullptr)
    {
        // TODO Violin this still needs some thought (the impl lifecycle with and without deserialization + base classes)
    }

    RamsesNodeBindingImpl::RamsesNodeBindingImpl(std::string_view name, ramses::ERotationConvention rotationConvention, std::unique_ptr<PropertyImpl> inputs, ramses::Node* ramsesNode) noexcept
        : RamsesBindingImpl(name, std::move(inputs), nullptr)
        , m_ramsesNode(ramsesNode)
        , m_rotationConvention(rotationConvention)
    {
    }

    flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding> RamsesNodeBindingImpl::Serialize(const RamsesNodeBindingImpl& nodeBinding, flatbuffers::FlatBufferBuilder& builder)
    {
        ramses::sceneObjectId_t ramsesNodeId;
        if (nodeBinding.m_ramsesNode != nullptr)
        {
            ramsesNodeId = nodeBinding.m_ramsesNode->getSceneObjectId();
        }

        auto ramsesNodeBinding = rlogic_serialization::CreateRamsesNodeBinding(builder,
            LogicNodeImpl::Serialize(nodeBinding, builder), // Serialize base class
            ramsesNodeId.getValue(),
            static_cast<uint8_t>(nodeBinding.m_rotationConvention)
        );
        builder.Finish(ramsesNodeBinding);

        return ramsesNodeBinding;
    }

    std::unique_ptr<RamsesNodeBindingImpl> RamsesNodeBindingImpl::Deserialize(const rlogic_serialization::RamsesNodeBinding& nodeBinding, ramses::Node* ramsesNode)
    {
        assert(nullptr != nodeBinding.logicnode());
        assert(nullptr != nodeBinding.logicnode()->name());
        assert(nullptr != nodeBinding.logicnode()->inputs());

        std::unique_ptr<PropertyImpl> inputs = PropertyImpl::Deserialize(*nodeBinding.logicnode()->inputs(), EPropertySemantics::BindingInput);
        assert(nullptr != inputs);

        return std::make_unique<RamsesNodeBindingImpl>(
            nodeBinding.logicnode()->name()->string_view(),
            static_cast<ramses::ERotationConvention>(nodeBinding.rotationConvention()),
            std::move(inputs),
            ramsesNode);
    }

    std::unique_ptr<PropertyImpl> RamsesNodeBindingImpl::CreateNodeProperties()
    {
        auto inputsImpl = std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::BindingInput);
        // Attention! This order is important - it has to match the indices in ENodePropertyStaticIndex!

        inputsImpl->addChild(std::make_unique<PropertyImpl>("visibility", EPropertyType::Bool, EPropertySemantics::BindingInput));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("rotation", EPropertyType::Vec3f, EPropertySemantics::BindingInput));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("translation", EPropertyType::Vec3f, EPropertySemantics::BindingInput));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("scaling", EPropertyType::Vec3f, EPropertySemantics::BindingInput));

        // Set default values equivalent to those of Ramses
        inputsImpl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl->setInternal<bool>(true);
        inputsImpl->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl->setInternal<vec3f>({ 1.f, 1.f, 1.f });

        return inputsImpl;
    }

    std::optional<LogicNodeRuntimeError> RamsesNodeBindingImpl::update()
    {
        if (m_ramsesNode != nullptr)
        {
            ramses::status_t status = ramses::StatusOK;
            PropertyImpl& visibility = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl;
            if (visibility.checkForBindingInputNewValueAndReset())
            {
                // TODO Violin what about 'Off' state? Worth discussing!
                if (*visibility.get<bool>())
                {
                    status = m_ramsesNode->setVisibility(ramses::EVisibilityMode::Visible);
                }
                else
                {
                    status = m_ramsesNode->setVisibility(ramses::EVisibilityMode::Invisible);
                }

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{m_ramsesNode->getStatusMessage(status)};
                }
            }

            PropertyImpl& rotation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl;
            if (rotation.checkForBindingInputNewValueAndReset())
            {
                vec3f value = *rotation.get<vec3f>();
                status = m_ramsesNode->setRotation(value[0], value[1], value[2], m_rotationConvention);

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{m_ramsesNode->getStatusMessage(status)};
                }
            }

            PropertyImpl& translation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation))->m_impl;
            if (translation.checkForBindingInputNewValueAndReset())
            {
                vec3f value = *translation.get<vec3f>();
                status = m_ramsesNode->setTranslation(value[0], value[1], value[2]);

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesNode->getStatusMessage(status) };
                }
            }

            PropertyImpl& scaling = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl;
            if (scaling.checkForBindingInputNewValueAndReset())
            {
                vec3f value = *scaling.get<vec3f>();
                status = m_ramsesNode->setScaling(value[0], value[1], value[2]);

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesNode->getStatusMessage(status) };
                }
            }
        }

        return std::nullopt;
    }

    bool RamsesNodeBindingImpl::setRamsesNode(ramses::Node* node)
    {
        m_ramsesNode = node;
        return true;
    }

    ramses::Node* RamsesNodeBindingImpl::getRamsesNode() const
    {
        return m_ramsesNode;
    }

    bool RamsesNodeBindingImpl::setRotationConvention(ramses::ERotationConvention rotationConvention)
    {
        m_rotationConvention = rotationConvention;
        return true;
    }

    ramses::ERotationConvention RamsesNodeBindingImpl::getRotationConvention() const
    {
        return m_rotationConvention;
    }

}
