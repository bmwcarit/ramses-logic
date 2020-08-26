//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/RamsesNodeBindingImpl.h"
#include "internals/impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

#include "ramses-client-api/Node.h"

#include "generated/ramsesnodebinding_gen.h"

namespace rlogic::internal
{
    std::unique_ptr<RamsesNodeBindingImpl> RamsesNodeBindingImpl::Create(std::string_view name)
    {
        // We can't use std::make_unique here, because the constructor is private
        return std::unique_ptr<RamsesNodeBindingImpl>(new RamsesNodeBindingImpl(name));
    }

    std::unique_ptr<RamsesNodeBindingImpl> RamsesNodeBindingImpl::Create(const serialization::RamsesNodeBinding& nodeBinding)
    {
        // TODO Test with large scene how much overhead it is to store lots of bindings with empty names
        assert(nullptr != nodeBinding.logicnode());
        assert(nullptr != nodeBinding.logicnode()->name());
        assert(nullptr != nodeBinding.logicnode()->inputs());

        auto inputs = PropertyImpl::Create(nodeBinding.logicnode()->inputs());

        assert (nullptr != inputs);

        const auto name = nodeBinding.logicnode()->name()->string_view();

        return std::unique_ptr<RamsesNodeBindingImpl>(new RamsesNodeBindingImpl(name , std::move(inputs)));
    }

    RamsesNodeBindingImpl::RamsesNodeBindingImpl(std::string_view name) noexcept
        : LogicNodeImpl(name)
    {
        auto inputsImpl = std::make_unique<PropertyImpl>("IN", EPropertyType::Struct);
        // Attention! This order is important - it has to match the indices in ENodePropertyStaticIndex!
        inputsImpl->addChild(std::make_unique<PropertyImpl>("visibility", EPropertyType::Bool));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("rotation", EPropertyType::Vec3f));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("translation", EPropertyType::Vec3f));
        inputsImpl->addChild(std::make_unique<PropertyImpl>("scaling", EPropertyType::Vec3f));

        setInputs(std::move(inputsImpl));
    }

    RamsesNodeBindingImpl::RamsesNodeBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs) noexcept
        : LogicNodeImpl(name, std::move(inputs), nullptr)
    {
    }

    bool RamsesNodeBindingImpl::update()
    {
        if (m_ramsesNode != nullptr)
        {
            ramses::status_t status = ramses::StatusOK;
            const PropertyImpl* visibility = getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl.get();
            if (visibility->wasSet())
            {
                // TODO Violin what about 'Off' state? Worth discussing!
                if (*visibility->get<bool>())
                {
                    status = m_ramsesNode->setVisibility(ramses::EVisibilityMode::Visible);
                }
                else
                {
                    status = m_ramsesNode->setVisibility(ramses::EVisibilityMode::Invisible);
                }

                if (status != ramses::StatusOK)
                {
                    addError(m_ramsesNode->getStatusMessage(status));
                    return false;
                }
            }

            const PropertyImpl* rotation = getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl.get();
            if (rotation->wasSet())
            {
                vec3f value = *rotation->get<vec3f>();
                status = m_ramsesNode->setRotation(value[0], value[1], value[2]);

                if (status != ramses::StatusOK)
                {
                    addError(m_ramsesNode->getStatusMessage(status));
                    return false;
                }
            }

            const PropertyImpl* translation = getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation))->m_impl.get();
            if (translation->wasSet())
            {
                vec3f value = *translation->get<vec3f>();
                status = m_ramsesNode->setTranslation(value[0], value[1], value[2]);

                if (status != ramses::StatusOK)
                {
                    addError(m_ramsesNode->getStatusMessage(status));
                    return false;
                }
            }

            const PropertyImpl* scaling = getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl.get();
            if (scaling->wasSet())
            {
                vec3f value = *scaling->get<vec3f>();
                status = m_ramsesNode->setScaling(value[0], value[1], value[2]);

                if (status != ramses::StatusOK)
                {
                    addError(m_ramsesNode->getStatusMessage(status));
                    return false;
                }
            }
        }
        return true;
    }

    flatbuffers::Offset<serialization::RamsesNodeBinding> RamsesNodeBindingImpl::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        auto ramsesNodeBinding = serialization::CreateRamsesNodeBinding(builder,
            LogicNodeImpl::serialize(builder)
        );
        builder.Finish(ramsesNodeBinding);

        return ramsesNodeBinding;
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

}
