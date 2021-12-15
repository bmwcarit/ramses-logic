//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesNodeBindingImpl.h"

#include "ramses-client-api/Node.h"

#include "ramses-logic/Property.h"

#include "impl/PropertyImpl.h"
#include "impl/LoggerImpl.h"

#include "internals/ErrorReporting.h"
#include "internals/IRamsesObjectResolver.h"
#include "internals/RotationUtils.h"

#include "generated/RamsesNodeBindingGen.h"

namespace rlogic::internal
{
    RamsesNodeBindingImpl::RamsesNodeBindingImpl(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name, uint64_t id)
        : RamsesBindingImpl(name, id)
        , m_ramsesNode(ramsesNode)
        , m_rotationType(rotationType)
    {
        // Attention! This order is important - it has to match the indices in ENodePropertyStaticIndex!
        auto inputsType = MakeStruct("IN", {
                TypeData{"visibility", EPropertyType::Bool},
                TypeData{"rotation", rotationType == ERotationType::Quaternion ? EPropertyType::Vec4f : EPropertyType::Vec3f},
                TypeData{"translation", EPropertyType::Vec3f},
                TypeData{"scaling", EPropertyType::Vec3f},
            });
        auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>(inputsType, EPropertySemantics::BindingInput));

        setRootProperties(std::move(inputs), {});

        ApplyRamsesValuesToInputProperties(*this, ramsesNode);
    }

    flatbuffers::Offset<rlogic_serialization::RamsesNodeBinding> RamsesNodeBindingImpl::Serialize(
        const RamsesNodeBindingImpl& nodeBinding,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        auto ramsesReference = RamsesBindingImpl::SerializeRamsesReference(nodeBinding.m_ramsesNode, builder);

        auto ramsesBinding = rlogic_serialization::CreateRamsesBinding(builder,
            builder.CreateString(nodeBinding.getName()),
            nodeBinding.getId(),
            ramsesReference,
            // TODO Violin don't serialize inputs - it's better to re-create them on the fly, they are uniquely defined and don't need serialization
            PropertyImpl::Serialize(*nodeBinding.getInputs()->m_impl, builder, serializationMap));
        builder.Finish(ramsesBinding);

        auto ramsesNodeBinding = rlogic_serialization::CreateRamsesNodeBinding(builder,
            ramsesBinding,
            static_cast<uint8_t>(nodeBinding.m_rotationType)
        );
        builder.Finish(ramsesNodeBinding);

        return ramsesNodeBinding;
    }

    std::unique_ptr<RamsesNodeBindingImpl> RamsesNodeBindingImpl::Deserialize(
        const rlogic_serialization::RamsesNodeBinding& nodeBinding,
        const IRamsesObjectResolver& ramsesResolver,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!nodeBinding.base())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing base class info!", nullptr);
            return nullptr;
        }

        if (nodeBinding.base()->id() == 0u)
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing id!", nullptr);
            return nullptr;
        }

        // TODO Violin make optional - no need to always serialize string if not used
        if (!nodeBinding.base()->name())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing name!", nullptr);
            return nullptr;
        }

        const std::string_view name = nodeBinding.base()->name()->string_view();

        if (!nodeBinding.base()->rootInput())
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing root input!", nullptr);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> deserializedRootInput = PropertyImpl::Deserialize(*nodeBinding.base()->rootInput(), EPropertySemantics::BindingInput, errorReporting, deserializationMap);

        if (!deserializedRootInput)
        {
            return nullptr;
        }

        // TODO Violin don't serialize these inputs -> get rid of the check
        if (deserializedRootInput->getName() != "IN" || deserializedRootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: root input has unexpected name or type!", nullptr);
            return nullptr;
        }

        const auto* boundObject = nodeBinding.base()->boundRamsesObject();
        if (!boundObject)
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: missing ramses object reference!", nullptr);
            return nullptr;
        }

        const ramses::sceneObjectId_t objectId(boundObject->objectId());

        ramses::Node* ramsesNode = ramsesResolver.findRamsesNodeInScene(name, objectId);
        if (!ramsesNode)
        {
            // TODO Violin improve error reporting for this particular error (it's reported in ramsesResolver currently): provide better message and scene/node ids
            return nullptr;
        }

        if (ramsesNode->getType() != static_cast<int>(boundObject->objectType()))
        {
            errorReporting.add("Fatal error during loading of RamsesNodeBinding from serialized data: loaded node type does not match referenced node type!", nullptr);
            return nullptr;
        }

        const auto rotationType (static_cast<ERotationType>(nodeBinding.rotationType()));

        auto binding = std::make_unique<RamsesNodeBindingImpl>(*ramsesNode, rotationType, name, nodeBinding.base()->id());
        binding->setRootProperties(std::make_unique<Property>(std::move(deserializedRootInput)), {});

        ApplyRamsesValuesToInputProperties(*binding, *ramsesNode);

        return binding;
    }

    std::optional<LogicNodeRuntimeError> RamsesNodeBindingImpl::update()
    {
        ramses::status_t status = ramses::StatusOK;
        PropertyImpl& visibility = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl;
        if (visibility.checkForBindingInputNewValueAndReset())
        {
            // TODO Violin what about 'Off' state? Worth discussing!
            if (visibility.getValueAs<bool>())
            {
                status = m_ramsesNode.get().setVisibility(ramses::EVisibilityMode::Visible);
            }
            else
            {
                status = m_ramsesNode.get().setVisibility(ramses::EVisibilityMode::Invisible);
            }

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesNode.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& rotation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl;
        if (rotation.checkForBindingInputNewValueAndReset())
        {
            if (m_rotationType == ERotationType::Quaternion)
            {
                const auto& valuesQuat = rotation.getValueAs<vec4f>();
                const vec3f eulerXYZ = RotationUtils::QuaternionToEulerXYZDegrees(valuesQuat);
                status = m_ramsesNode.get().setRotation(eulerXYZ[0], eulerXYZ[1], eulerXYZ[2], ramses::ERotationConvention::ZYX);
            }
            else
            {
                const auto& valuesEuler = rotation.getValueAs<vec3f>();
                status = m_ramsesNode.get().setRotation(valuesEuler[0], valuesEuler[1], valuesEuler[2], *RotationUtils::RotationTypeToRamsesRotationConvention(m_rotationType));
            }

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesNode.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& translation = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation))->m_impl;
        if (translation.checkForBindingInputNewValueAndReset())
        {
            const auto& value = translation.getValueAs<vec3f>();
            status = m_ramsesNode.get().setTranslation(value[0], value[1], value[2]);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{ m_ramsesNode.get().getStatusMessage(status) };
            }
        }

        PropertyImpl& scaling = *getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl;
        if (scaling.checkForBindingInputNewValueAndReset())
        {
            const auto& value = scaling.getValueAs<vec3f>();
            status = m_ramsesNode.get().setScaling(value[0], value[1], value[2]);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{ m_ramsesNode.get().getStatusMessage(status) };
            }
        }

        return std::nullopt;
    }

    ramses::Node& RamsesNodeBindingImpl::getRamsesNode() const
    {
        return m_ramsesNode;
    }

    ERotationType RamsesNodeBindingImpl::getRotationType() const
    {
        return m_rotationType;
    }

    // Overwrites binding value cache silently (without triggering dirty check) - this code is only executed at initialization,
    // should not overwrite values unless set() or link explicitly called
    void RamsesNodeBindingImpl::ApplyRamsesValuesToInputProperties(RamsesNodeBindingImpl& binding, ramses::Node& ramsesNode)
    {
        const bool visible = (ramsesNode.getVisibility() == ramses::EVisibilityMode::Visible);
        binding.getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Visibility))->m_impl->initializeBindingInputValue(PropertyValue{ visible });

        vec3f translationValue;
        ramsesNode.getTranslation(translationValue[0], translationValue[1], translationValue[2]);
        binding.getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Translation))->m_impl->initializeBindingInputValue(PropertyValue{ translationValue });

        vec3f scalingValue;
        ramsesNode.getScaling(scalingValue[0], scalingValue[1], scalingValue[2]);
        binding.getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Scaling))->m_impl->initializeBindingInputValue(PropertyValue{ scalingValue });

        if (binding.m_rotationType == ERotationType::Quaternion)
        {
            binding.getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl->initializeBindingInputValue(vec4f{0.f, 0.f, 0.f, 1.f});
        }
        else
        {
            vec3f rotationValue;
            ramses::ERotationConvention rotationConvention;
            ramsesNode.getRotation(rotationValue[0], rotationValue[1], rotationValue[2], rotationConvention);

            std::optional<ERotationType> convertedType = RotationUtils::RamsesRotationConventionToRotationType(rotationConvention);
            if (!convertedType || binding.m_rotationType != *convertedType)
            {
                LOG_WARN("Initial rotation values for RamsesNodeBinding '{}' will not be imported from bound Ramses node due to mismatching rotation type.", binding.getName());
            }
            else
            {
                binding.getInputs()->getChild(static_cast<size_t>(ENodePropertyStaticIndex::Rotation))->m_impl->initializeBindingInputValue(PropertyValue{ rotationValue });
            }
        }
    }
}
