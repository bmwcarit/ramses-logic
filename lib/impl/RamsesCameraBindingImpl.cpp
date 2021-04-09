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

    RamsesCameraBindingImpl::RamsesCameraBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Camera& camera)
        : RamsesBindingImpl(name, std::move(inputs), nullptr)
        , m_camera(&camera)
    {
    }

    std::unique_ptr<RamsesCameraBindingImpl> RamsesCameraBindingImpl::Create(std::string_view name)
    {
        // We can't use std::make_unique here, because the constructor is private
        return std::unique_ptr<RamsesCameraBindingImpl>(new RamsesCameraBindingImpl(name));
    }

    std::unique_ptr<RamsesCameraBindingImpl> RamsesCameraBindingImpl::Create(const rlogic_serialization::RamsesCameraBinding& cameraBinding, ramses::Camera* camera)
    {
        assert(nullptr != cameraBinding.logicnode());
        assert(nullptr != cameraBinding.logicnode()->name());
        assert(nullptr != cameraBinding.logicnode()->inputs());

        auto inputs = PropertyImpl::Create(*cameraBinding.logicnode()->inputs(), EPropertySemantics::BindingInput);

        assert(nullptr != inputs);

        const auto name = cameraBinding.logicnode()->name()->string_view();

        return std::unique_ptr<RamsesCameraBindingImpl>(new RamsesCameraBindingImpl(name, std::move(inputs), *camera));
    }


    void RamsesCameraBindingImpl::CreateCameraProperties(ramses::ERamsesObjectType cameraType)
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

    bool RamsesCameraBindingImpl::update()
    {
        ramses::PerspectiveCamera* perspectiveCam = nullptr;
        if (m_camera != nullptr)
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
                status = m_camera->setViewport(
                    *vpOffsetX.get<int32_t>(),
                    *vpOffsetY.get<int32_t>(),
                    *vpWidth.get<int32_t>(),
                    *vpHeight.get<int32_t>());

                if (status != ramses::StatusOK)
                {
                    addError(m_camera->getStatusMessage(status));
                    return false;
                }
            }

            PropertyImpl& frustumProperties = *getInputs()->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum))->m_impl;

            // Index of Perspective Frustum Properties is used, but wouldn't matter as Ortho Camera indeces are the same for these two properties
            PropertyImpl& nearPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane))->m_impl;
            PropertyImpl& farPlane = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane))->m_impl;

            if (m_camera->getType() == ramses::ERamsesObjectType_PerspectiveCamera)
            {
                PropertyImpl& fov = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView))->m_impl;
                PropertyImpl& aR = *frustumProperties.getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio))->m_impl;

                if (nearPlane.checkForBindingInputNewValueAndReset()
                    || farPlane.checkForBindingInputNewValueAndReset()
                    || fov.checkForBindingInputNewValueAndReset()
                    || aR.checkForBindingInputNewValueAndReset())
                {
                    perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(*m_camera);
                    status = perspectiveCam->setFrustum(*fov.get<float>(), *aR.get<float>(), *nearPlane.get<float>(), *farPlane.get<float>());

                    if (status != ramses::StatusOK)
                    {
                        addError(m_camera->getStatusMessage(status));
                        return false;
                    }
                }
            }
            else if (m_camera->getType() == ramses::ERamsesObjectType_OrthographicCamera)
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
                    status = m_camera->setFrustum(*leftPlane.get<float>(), *rightPlane.get<float>(), *bottomPlane.get<float>(), *topPlane.get<float>(), *nearPlane.get<float>(), *farPlane.get<float>());

                    if (status != ramses::StatusOK)
                    {
                        addError(m_camera->getStatusMessage(status));
                        return false;
                    }
                }
            }
            else
            {
                assert(false && "this should never happen");
            }

        }

        return true;
    }

    ramses::ERamsesObjectType RamsesCameraBindingImpl::getCameraType() const
    {
        if (m_camera)
        {
            return m_camera->getType();
        }
        return ramses::ERamsesObjectType::ERamsesObjectType_Invalid;
    }

    void RamsesCameraBindingImpl::setRamsesCamera(ramses::Camera* camera)
    {
        PropertyImpl& inputsImpl = *getInputs()->m_impl;
        inputsImpl.clearChildren();

        m_camera = camera;

        if (nullptr != m_camera)
        {
            CreateCameraProperties(camera->getType());
        }
    }

    ramses::Camera* RamsesCameraBindingImpl::getRamsesCamera() const
    {
        return m_camera;
    }

    flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding> RamsesCameraBindingImpl::serialize(flatbuffers::FlatBufferBuilder& builder) const
    {
        ramses::sceneObjectId_t cameraId;
        if (m_camera != nullptr)
        {
            cameraId = m_camera->getSceneObjectId();
        }
        auto ramsesCameraBinding = rlogic_serialization::CreateRamsesCameraBinding(
            builder,
            LogicNodeImpl::serialize(builder),
            cameraId.getValue());
        builder.Finish(ramsesCameraBinding);

        return ramsesCameraBinding;
    }
}
