//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-utils.h"
#include "ramses-client-api/Camera.h"
#include "ramses-client-api/PerspectiveCamera.h"

#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "impl/PropertyImpl.h"
#include "impl/LoggerImpl.h"

#include "internals/RamsesHelper.h"
#include "internals/ErrorReporting.h"
#include "internals/IRamsesObjectResolver.h"

#include "generated/RamsesCameraBindingGen.h"

namespace rlogic::internal
{
    RamsesCameraBindingImpl::RamsesCameraBindingImpl(ramses::Camera& ramsesCamera, std::string_view name, uint64_t id)
        : RamsesBindingImpl(name, id)
        , m_ramsesCamera(ramsesCamera)
    {
        std::vector<TypeData> frustumPlanes = {
            TypeData{ "nearPlane", EPropertyType::Float },
            TypeData{ "farPlane", EPropertyType::Float },
        };
        switch (m_ramsesCamera.get().getType())
        {
        case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
            // Attention! This order is important - it has to match the indices in EPerspectiveCameraFrustumPropertyStaticIndex
            frustumPlanes.emplace_back("fieldOfView", EPropertyType::Float);
            frustumPlanes.emplace_back("aspectRatio", EPropertyType::Float);
            break;
        case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
            // Attention! This order is important - it has to match the indices in EOrthographicCameraFrustumPropertyStaticIndex
            frustumPlanes.emplace_back("leftPlane", EPropertyType::Float);
            frustumPlanes.emplace_back("rightPlane", EPropertyType::Float);
            frustumPlanes.emplace_back("bottomPlane", EPropertyType::Float);
            frustumPlanes.emplace_back("topPlane", EPropertyType::Float);
            break;
        default:
            assert(false && "This should never happen");
            break;
        }

        HierarchicalTypeData cameraBindingInputs(
            TypeData{"IN", EPropertyType::Struct},
            {
                MakeStruct("viewport",
                    {
                        // Attention! This order is important - it has to match the indices in ECameraViewportPropertyStaticIndex
                        TypeData{"offsetX", EPropertyType::Int32},
                        TypeData{"offsetY", EPropertyType::Int32},
                        TypeData{"width", EPropertyType::Int32},
                        TypeData{"height", EPropertyType::Int32}
                    }
                ),
                MakeStruct("frustum", frustumPlanes),
            }
        );

        setRootProperties(
            std::make_unique<Property>(std::make_unique<PropertyImpl>(cameraBindingInputs, EPropertySemantics::BindingInput)),
            {} // No outputs
        );

        ApplyRamsesValuesToInputProperties(*this, ramsesCamera);
    }

    flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding> RamsesCameraBindingImpl::Serialize(
        const RamsesCameraBindingImpl& cameraBinding,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        auto ramsesReference = RamsesBindingImpl::SerializeRamsesReference(cameraBinding.m_ramsesCamera, builder);

        auto ramsesBinding = rlogic_serialization::CreateRamsesBinding(builder,
            builder.CreateString(cameraBinding.getName()),
            cameraBinding.getId(),
            ramsesReference,
            // TODO Violin don't serialize these - they carry no useful information and are redundant
            PropertyImpl::Serialize(*cameraBinding.getInputs()->m_impl, builder, serializationMap));
        builder.Finish(ramsesBinding);

        auto ramsesCameraBinding = rlogic_serialization::CreateRamsesCameraBinding(builder, ramsesBinding);
        builder.Finish(ramsesCameraBinding);

        return ramsesCameraBinding;
    }

    std::unique_ptr<RamsesCameraBindingImpl> RamsesCameraBindingImpl::Deserialize(
        const rlogic_serialization::RamsesCameraBinding& cameraBinding,
        const IRamsesObjectResolver& ramsesResolver,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!cameraBinding.base())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing base class info!", nullptr);
            return nullptr;
        }

        if (cameraBinding.base()->id() == 0u)
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing id!", nullptr);
            return nullptr;
        }

        // TODO Violin make optional - no need to always serialize string if not used
        if (!cameraBinding.base()->name())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing name!", nullptr);
            return nullptr;
        }

        const std::string_view name = cameraBinding.base()->name()->string_view();

        if (!cameraBinding.base()->rootInput())
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: missing root input!", nullptr);
            return nullptr;
        }

        std::unique_ptr<PropertyImpl> deserializedRootInput = PropertyImpl::Deserialize(*cameraBinding.base()->rootInput(), EPropertySemantics::BindingInput, errorReporting, deserializationMap);

        if (!deserializedRootInput)
        {
            return nullptr;
        }

        // TODO Violin don't serialize these inputs -> get rid of the check
        if (deserializedRootInput->getName() != "IN" || deserializedRootInput->getType() != EPropertyType::Struct)
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: root input has unexpected name or type!", nullptr);
            return nullptr;
        }

        const auto* boundObject = cameraBinding.base()->boundRamsesObject();
        if (!boundObject)
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: no reference to ramses camera!", nullptr);
            return nullptr;
        }

        const ramses::sceneObjectId_t objectId(boundObject->objectId());

        ramses::Camera* resolvedCamera = ramsesResolver.findRamsesCameraInScene(name, objectId);
        if (!resolvedCamera)
        {
            // TODO Violin improve error reporting for this particular error (it's reported in ramsesResolver currently): provide better message and scene/camera ids
            return nullptr;
        }

        if (resolvedCamera->getType() != static_cast<int>(boundObject->objectType()))
        {
            errorReporting.add("Fatal error during loading of RamsesCameraBinding from serialized data: loaded type does not match referenced camera type!", nullptr);
            return nullptr;
        }

        auto binding = std::make_unique<RamsesCameraBindingImpl>(*resolvedCamera, name, cameraBinding.base()->id());
        binding->setRootProperties(std::make_unique<Property>(std::move(deserializedRootInput)), {});

        ApplyRamsesValuesToInputProperties(*binding, *resolvedCamera);

        return binding;
    }

    std::optional<LogicNodeRuntimeError> RamsesCameraBindingImpl::update()
    {
        ramses::status_t status = ramses::StatusOK;
        PropertyImpl& vpProperties = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Viewport))->m_impl;

        PropertyImpl& vpOffsetX = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX))->m_impl;
        PropertyImpl& vpOffsetY = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetY))->m_impl;
        PropertyImpl& vpWidth = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth))->m_impl;
        PropertyImpl& vpHeight = *vpProperties.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight))->m_impl;
        if (vpOffsetX.checkForBindingInputNewValueAndReset()
            || vpOffsetY.checkForBindingInputNewValueAndReset()
            || vpWidth.checkForBindingInputNewValueAndReset()
            || vpHeight.checkForBindingInputNewValueAndReset())
        {
            const int32_t vpX = vpOffsetX.getValueAs<int32_t>();
            const int32_t vpY = vpOffsetY.getValueAs<int32_t>();
            const int32_t vpW = vpWidth.getValueAs<int32_t>();
            const int32_t vpH = vpHeight.getValueAs<int32_t>();

            if (vpW <= 0 || vpH <= 0)
            {
                return LogicNodeRuntimeError{ fmt::format("Camera viewport size must be positive! (width: {}; height: {})", vpW, vpH) };
            }

            status = m_ramsesCamera.get().setViewport(vpX, vpY, vpW, vpH);

            if (status != ramses::StatusOK)
            {
                return LogicNodeRuntimeError{m_ramsesCamera.get().getStatusMessage(status)};
            }
        }

        PropertyImpl& frustum = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum))->m_impl;

        // Index of Perspective Frustum Properties is used, but wouldn't matter as Ortho Camera indeces are the same for these two properties
        PropertyImpl& nearPlane = *frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl;
        PropertyImpl& farPlane = *frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl;

        if (m_ramsesCamera.get().getType() == ramses::ERamsesObjectType_PerspectiveCamera)
        {
            PropertyImpl& fov = *frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl;
            PropertyImpl& aR = *frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl;

            if (nearPlane.checkForBindingInputNewValueAndReset()
                || farPlane.checkForBindingInputNewValueAndReset()
                || fov.checkForBindingInputNewValueAndReset()
                || aR.checkForBindingInputNewValueAndReset())
            {
                auto* perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(m_ramsesCamera.get());
                status = perspectiveCam->setFrustum(fov.getValueAs<float>(), aR.getValueAs<float>(), nearPlane.getValueAs<float>(), farPlane.getValueAs<float>());

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesCamera.get().getStatusMessage(status) };
                }
            }
        }
        else if (m_ramsesCamera.get().getType() == ramses::ERamsesObjectType_OrthographicCamera)
        {
            PropertyImpl& leftPlane = *frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane))->m_impl;
            PropertyImpl& rightPlane = *frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane))->m_impl;
            PropertyImpl& bottomPlane = *frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane))->m_impl;
            PropertyImpl& topPlane = *frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane))->m_impl;

            if (nearPlane.checkForBindingInputNewValueAndReset()
                || farPlane.checkForBindingInputNewValueAndReset()
                || leftPlane.checkForBindingInputNewValueAndReset()
                || rightPlane.checkForBindingInputNewValueAndReset()
                || bottomPlane.checkForBindingInputNewValueAndReset()
                || topPlane.checkForBindingInputNewValueAndReset())
            {
                status = m_ramsesCamera.get().setFrustum(
                    leftPlane.getValueAs<float>(),
                    rightPlane.getValueAs<float>(),
                    bottomPlane.getValueAs<float>(),
                    topPlane.getValueAs<float>(),
                    nearPlane.getValueAs<float>(),
                    farPlane.getValueAs<float>());

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{ m_ramsesCamera.get().getStatusMessage(status) };
                }
            }
        }
        else
        {
            assert(false && "this should never happen");
        }

        return std::nullopt;
    }

    ramses::ERamsesObjectType RamsesCameraBindingImpl::getCameraType() const
    {
        return m_ramsesCamera.get().getType();
    }

    ramses::Camera& RamsesCameraBindingImpl::getRamsesCamera() const
    {
        return m_ramsesCamera;
    }

    void RamsesCameraBindingImpl::ApplyRamsesValuesToInputProperties(RamsesCameraBindingImpl& binding, ramses::Camera& ramsesCamera)
    {
        // Initializes input values with values from ramses camera silently (no dirty mechanism triggered)
        PropertyImpl& viewport = *binding.getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Viewport))->m_impl;
        viewport.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getViewportX() });
        viewport.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetY))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getViewportY() });
        viewport.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth))->m_impl->initializeBindingInputValue(PropertyValue{ static_cast<int32_t>(ramsesCamera.getViewportWidth()) });
        viewport.getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight))->m_impl->initializeBindingInputValue(PropertyValue{ static_cast<int32_t>(ramsesCamera.getViewportHeight()) });

        PropertyImpl& frustum = *binding.getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum))->m_impl;
        const float nearPlane = ramsesCamera.getNearPlane();
        const float farPlane = ramsesCamera.getFarPlane();
        switch (ramsesCamera.getType())
        {
        case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
        {
            const auto* perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(ramsesCamera);
            frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl->initializeBindingInputValue(PropertyValue{ nearPlane });
            frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl->initializeBindingInputValue(PropertyValue{ farPlane });
            frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl->initializeBindingInputValue(PropertyValue{ perspectiveCam->getVerticalFieldOfView() });
            frustum.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl->initializeBindingInputValue(PropertyValue{ perspectiveCam->getAspectRatio() });
            break;
        }
        case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::NearPlane))->m_impl->initializeBindingInputValue(PropertyValue{ nearPlane });
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::FarPlane))->m_impl->initializeBindingInputValue(PropertyValue{ farPlane });
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getLeftPlane() });
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getRightPlane() });
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getBottomPlane() });
            frustum.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane))->m_impl->initializeBindingInputValue(PropertyValue{ ramsesCamera.getTopPlane() });
            break;
        default:
            assert(false && "This should never happen");
            break;
        }
    }

}
