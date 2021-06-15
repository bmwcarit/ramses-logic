//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesCameraBindingImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/LoggerImpl.h"

#include "internals/RamsesHelper.h"
#include "internals/ErrorReporting.h"

#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"

#include "generated/ramsescamerabinding_gen.h"

#include "ramses-utils.h"
#include "ramses-client-api/Camera.h"
#include "ramses-client-api/PerspectiveCamera.h"

namespace rlogic::internal
{
    RamsesCameraBindingImpl::RamsesCameraBindingImpl(std::string_view name)
        : RamsesBindingImpl(name, std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::BindingInput), nullptr)
    {
    }

    RamsesCameraBindingImpl::RamsesCameraBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> deserializedInputs, ramses::Camera* camera)
        : RamsesBindingImpl(name, std::move(deserializedInputs), nullptr)
        , m_ramsesCamera(camera)
    {
    }

    flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding> RamsesCameraBindingImpl::Serialize(const RamsesCameraBindingImpl& cameraBinding, flatbuffers::FlatBufferBuilder& builder)
    {
        ramses::sceneObjectId_t cameraId;
        if (cameraBinding.m_ramsesCamera != nullptr)
        {
            cameraId = cameraBinding.m_ramsesCamera->getSceneObjectId();
        }
        auto ramsesCameraBinding = rlogic_serialization::CreateRamsesCameraBinding(
            builder,
            LogicNodeImpl::Serialize(cameraBinding, builder), // Call base class serialization method
            cameraId.getValue());
        builder.Finish(ramsesCameraBinding);

        return ramsesCameraBinding;
    }

    std::unique_ptr<RamsesCameraBindingImpl> RamsesCameraBindingImpl::Deserialize(const rlogic_serialization::RamsesCameraBinding& cameraBinding, ramses::Camera* camera)
    {
        assert(nullptr != cameraBinding.logicnode());
        assert(nullptr != cameraBinding.logicnode()->name());
        assert(nullptr != cameraBinding.logicnode()->inputs());

        return std::make_unique<RamsesCameraBindingImpl>(
            cameraBinding.logicnode()->name()->string_view(),
            PropertyImpl::Deserialize(*cameraBinding.logicnode()->inputs(), EPropertySemantics::BindingInput),
            camera);
    }

    void RamsesCameraBindingImpl::createCameraProperties(ramses::ERamsesObjectType cameraType)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        // Attention! This order is important - it has to match the indices in ECameraViewportPropertyStaticIndex, EPerspectiveCameraFrustumPropertyStaticIndex
        // and EOrthographicCameraFrustumPropertyStaticIndex

        // Add viewport struct
        std::unique_ptr<PropertyImpl> viewPortProperty = std::make_unique<PropertyImpl>("viewPortProperties", EPropertyType::Struct, EPropertySemantics::BindingInput);
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortOffsetX", EPropertyType::Int32, EPropertySemantics::BindingInput));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortOffsetY", EPropertyType::Int32, EPropertySemantics::BindingInput));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortWidth", EPropertyType::Int32, EPropertySemantics::BindingInput));
        viewPortProperty->addChild(std::make_unique<PropertyImpl>("viewPortHeight", EPropertyType::Int32, EPropertySemantics::BindingInput));

        // Add frustum struct
        std::unique_ptr<PropertyImpl> frustumProperty = std::make_unique<PropertyImpl>("frustumProperties", EPropertyType::Struct, EPropertySemantics::BindingInput);
        frustumProperty->addChild(std::make_unique<PropertyImpl>("nearPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
        frustumProperty->addChild(std::make_unique<PropertyImpl>("farPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
        switch (cameraType)
        {
        case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
            frustumProperty->addChild(std::make_unique<PropertyImpl>("fieldOfView", EPropertyType::Float, EPropertySemantics::BindingInput));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("aspectRatio", EPropertyType::Float, EPropertySemantics::BindingInput));
            break;
        case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
            frustumProperty->addChild(std::make_unique<PropertyImpl>("leftPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("rightPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("bottomPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
            frustumProperty->addChild(std::make_unique<PropertyImpl>("topPlane", EPropertyType::Float, EPropertySemantics::BindingInput));
            break;
        default:
            assert(false && "This should never happen");
            break;
        }

        // Set default non zero values of the common fields between perspective and orthographic camera equivalent to those of Ramses
        viewPortProperty->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth))->m_impl->setInternal<int32_t>(16);
        viewPortProperty->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight))->m_impl->setInternal<int32_t>(16);

        //Set the default non zero values of the uncommon fields between perspective and orthographic camera equivalent to those of Ramses
        switch (cameraType)
        {
            case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
                frustumProperty->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl->setInternal<float>(0.1f);
                frustumProperty->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl->setInternal<float>(1.0f);
                frustumProperty->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl->setInternal<float>(168.579f);
                frustumProperty->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl->setInternal<float>(1.0f);
                break;
            case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::NearPlane))->m_impl->setInternal<float>(0.1f);
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::FarPlane))->m_impl->setInternal<float>(1.0f);
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane))->m_impl->setInternal<float>(-1.0f);
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane))->m_impl->setInternal<float>(1.0f);
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane))->m_impl->setInternal<float>(-1.0f);
                frustumProperty->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane))->m_impl->setInternal<float>(1.0f);
                break;
            default:
                assert(false && "This should never happen");
                break;
        }

        inputsImpl.addChild(std::move(viewPortProperty));
        inputsImpl.addChild(std::move(frustumProperty));
    }

    std::optional<LogicNodeRuntimeError> RamsesCameraBindingImpl::update()
    {
        ramses::PerspectiveCamera* perspectiveCam = nullptr;
        if (m_ramsesCamera != nullptr)
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
                const int32_t vpX = *vpOffsetX.get<int32_t>();
                const int32_t vpY = *vpOffsetY.get<int32_t>();
                const int32_t vpW = *vpWidth.get<int32_t>();
                const int32_t vpH = *vpHeight.get<int32_t>();

                if (vpW <= 0 || vpH <= 0)
                {
                    return LogicNodeRuntimeError{ fmt::format("Camera viewport size must be positive! (width: {}; height: {})", vpW, vpH) };
                }

                status = m_ramsesCamera->setViewport(vpX, vpY, vpW, vpH);

                if (status != ramses::StatusOK)
                {
                    return LogicNodeRuntimeError{m_ramsesCamera->getStatusMessage(status)};
                }
            }

            PropertyImpl& frustumProperties = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum))->m_impl;

            // Index of Perspective Frustum Properties is used, but wouldn't matter as Ortho Camera indeces are the same for these two properties
            PropertyImpl& nearPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl;
            PropertyImpl& farPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl;

            if (m_ramsesCamera->getType() == ramses::ERamsesObjectType_PerspectiveCamera)
            {
                PropertyImpl& fov = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl;
                PropertyImpl& aR = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl;

                if (nearPlane.checkForBindingInputNewValueAndReset()
                    || farPlane.checkForBindingInputNewValueAndReset()
                    || fov.checkForBindingInputNewValueAndReset()
                    || aR.checkForBindingInputNewValueAndReset())
                {
                    perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(*m_ramsesCamera);
                    status = perspectiveCam->setFrustum(*fov.get<float>(), *aR.get<float>(), *nearPlane.get<float>(), *farPlane.get<float>());

                    if (status != ramses::StatusOK)
                    {
                        return LogicNodeRuntimeError{ m_ramsesCamera->getStatusMessage(status) };
                    }
                }
            }
            else if (m_ramsesCamera->getType() == ramses::ERamsesObjectType_OrthographicCamera)
            {
                PropertyImpl& leftPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane))->m_impl;
                PropertyImpl& rightPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane))->m_impl;
                PropertyImpl& bottomPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane))->m_impl;
                PropertyImpl& topPlane = *frustumProperties.getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane))->m_impl;

                if (nearPlane.checkForBindingInputNewValueAndReset()
                    || farPlane.checkForBindingInputNewValueAndReset()
                    || leftPlane.checkForBindingInputNewValueAndReset()
                    || rightPlane.checkForBindingInputNewValueAndReset()
                    || bottomPlane.checkForBindingInputNewValueAndReset()
                    || topPlane.checkForBindingInputNewValueAndReset())
                {
                    status = m_ramsesCamera->setFrustum(*leftPlane.get<float>(), *rightPlane.get<float>(), *bottomPlane.get<float>(), *topPlane.get<float>(), *nearPlane.get<float>(), *farPlane.get<float>());

                    if (status != ramses::StatusOK)
                    {
                        return LogicNodeRuntimeError{ m_ramsesCamera->getStatusMessage(status) };
                    }
                }
            }
            else
            {
                assert(false && "this should never happen");
            }

        }

        return std::nullopt;
    }

    ramses::ERamsesObjectType RamsesCameraBindingImpl::getCameraType() const
    {
        if (m_ramsesCamera)
        {
            return m_ramsesCamera->getType();
        }
        return ramses::ERamsesObjectType::ERamsesObjectType_Invalid;
    }

    void RamsesCameraBindingImpl::setRamsesCamera(ramses::Camera* camera)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        inputsImpl.clearChildren();

        m_ramsesCamera = camera;

        if (nullptr != m_ramsesCamera)
        {
            createCameraProperties(camera->getType());
        }
    }

    ramses::Camera* RamsesCameraBindingImpl::getRamsesCamera() const
    {
        return m_ramsesCamera;
    }
}
