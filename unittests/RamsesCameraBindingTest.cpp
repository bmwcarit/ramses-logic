//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "RamsesTestUtils.h"
#include "WithTempDirectory.h"

#include "impl/LogicEngineImpl.h"
#include "impl/RamsesCameraBindingImpl.h"
#include "impl/PropertyImpl.h"
#include "internals/RamsesHelper.h"
#include "generated/ramsescamerabinding_gen.h"

#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses-utils.h"
#include "ramses-client-api/PerspectiveCamera.h"
#include "ramses-client-api/OrthographicCamera.h"

namespace rlogic
{
    using rlogic::internal::EPerspectiveCameraFrustumPropertyStaticIndex;
    using rlogic::internal::EOrthographicCameraFrustumPropertyStaticIndex;
    using rlogic::internal::ECameraPropertyStructStaticIndex;
    using rlogic::internal::ECameraViewportPropertyStaticIndex;

    constexpr int32_t DefaultViewportOffsetX = 0;
    constexpr int32_t DefaultViewportOffsetY = 0;
    constexpr uint32_t DefaultViewportWidth = 16u;
    constexpr uint32_t DefaultViewportHeight = 16u;

    constexpr float NearPlaneDefault = 0.1f;
    constexpr float FarPlaneDefault = 1.0f;

    constexpr float PerspectiveFrustumFOVdefault = 168.579f;
    constexpr float PerspectiveFrustumARdefault = 1.f;

    constexpr float OrthoFrustumLPdefault = -1.f;
    constexpr float OrthoFrustumRPdefault = 1.f;
    constexpr float OrthoFrustumBPdefault = -1.f;
    constexpr float OrthoFrustumTPdefault = 1.0f;

    class ARamsesCameraBinding : public ::testing::Test
    {
    protected:
        ARamsesCameraBinding()
            : m_testScene(*m_ramsesTestSetup.createScene(ramses::sceneId_t(1)))
        {
        }

        RamsesCameraBinding& createCameraBindingForTest(std::string_view name, ramses::Camera* ramsesCamera = nullptr)
        {
            auto cameraBinding = m_logicEngine.createRamsesCameraBinding(name);
            if (ramsesCamera)
            {
                cameraBinding->setRamsesCamera(ramsesCamera);
            }
            return *cameraBinding;
        }

        ramses::PerspectiveCamera& createPerspectiveCameraForTest()
        {
            return *m_testScene.createPerspectiveCamera();
        }

        ramses::OrthographicCamera& createOrthoCameraForTest()
        {
            return *m_testScene.createOrthographicCamera();
        }

        static void ExpectPropertyTypeAndChildCount(Property* prop, EPropertyType type, uint32_t childCount)
        {
            ASSERT_NE(nullptr, prop);
            EXPECT_EQ(type, prop->getType());
            EXPECT_EQ(childCount, prop->getChildCount());
        }

        static void ExpectDefaultViewportValues(ramses::Camera& camera)
        {
            EXPECT_EQ(camera.getViewportX(), DefaultViewportOffsetX);
            EXPECT_EQ(camera.getViewportY(), DefaultViewportOffsetY);
            EXPECT_EQ(camera.getViewportWidth(), DefaultViewportWidth);
            EXPECT_EQ(camera.getViewportHeight(), DefaultViewportHeight);
        }

        static void ExpectDefaultPerspectiveCameraFrustumValues(ramses::PerspectiveCamera& camera)
        {
            EXPECT_NEAR(camera.getVerticalFieldOfView(), PerspectiveFrustumFOVdefault, 0.001f);
            EXPECT_EQ(camera.getAspectRatio(), PerspectiveFrustumARdefault);
            EXPECT_EQ(camera.getNearPlane(), NearPlaneDefault);
            EXPECT_EQ(camera.getFarPlane(), FarPlaneDefault);
        }

        static void ExpectDefaultOrthoCameraFrustumValues(ramses::Camera& camera)
        {
            EXPECT_EQ(camera.getLeftPlane(), OrthoFrustumLPdefault);
            EXPECT_EQ(camera.getRightPlane(), OrthoFrustumRPdefault);
            EXPECT_EQ(camera.getBottomPlane(), OrthoFrustumBPdefault);
            EXPECT_EQ(camera.getTopPlane(), OrthoFrustumTPdefault);
            EXPECT_EQ(camera.getNearPlane(), NearPlaneDefault);
            EXPECT_EQ(camera.getFarPlane(), FarPlaneDefault);
        }

        static void ExpectDefaultValues(ramses::Camera& camera)
        {
            ramses::PerspectiveCamera* perspectiveCam = nullptr;
            switch (camera.getType())
            {
            case ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera:
                perspectiveCam = ramses::RamsesUtils::TryConvert<ramses::PerspectiveCamera>(camera);
                ExpectDefaultViewportValues(camera);
                ExpectDefaultPerspectiveCameraFrustumValues(*perspectiveCam);
                break;
            case ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera:
                ExpectDefaultViewportValues(camera);
                ExpectDefaultOrthoCameraFrustumValues(camera);
                break;
            default:
                ASSERT_TRUE(false);
                break;
            }
        }

        RamsesTestSetup m_ramsesTestSetup;
        ramses::Scene& m_testScene;
        LogicEngine m_logicEngine;
    };

    TEST_F(ARamsesCameraBinding, HasANameAfterCreation)
    {
        auto& cameraBinding = createCameraBindingForTest("CameraBinding");
        EXPECT_EQ("CameraBinding", cameraBinding.getName());
    }

    TEST_F(ARamsesCameraBinding, HasInvalidCameraTypeAfterCreation)
    {
        auto& cameraBinding = createCameraBindingForTest("");
        EXPECT_EQ(ramses::ERamsesObjectType::ERamsesObjectType_Invalid, cameraBinding.m_cameraBinding->getCameraType());
    }

    TEST_F(ARamsesCameraBinding, HasEmptyInputsAfterCreation)
    {
        auto& appearanceBinding = createCameraBindingForTest("");
        EXPECT_EQ(appearanceBinding.getInputs()->getChildCount(), 0u);
        EXPECT_EQ(appearanceBinding.getInputs()->getType(), EPropertyType::Struct);
        EXPECT_EQ(appearanceBinding.getInputs()->getName(), "IN");
    }

    TEST_F(ARamsesCameraBinding, HasNoOutputsAfterCreation)
    {
        auto& cameraBinding = createCameraBindingForTest("");
        EXPECT_EQ(nullptr, cameraBinding.getOutputs());
    }

    TEST_F(ARamsesCameraBinding, ProducesNoErrorsDuringUpdate_IfNoRamsesCameraIsAssigned)
    {
        auto& cameraBinding = createCameraBindingForTest("");
        EXPECT_EQ(std::nullopt, cameraBinding.m_impl.get().update());
    }

    TEST_F(ARamsesCameraBinding, ReturnsPointerToRamsesCamera)
    {
        RamsesCameraBinding& cameraBinding = createCameraBindingForTest("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        EXPECT_EQ(ramses::ERamsesObjectType::ERamsesObjectType_Invalid, cameraBinding.m_cameraBinding->getCameraType());

        EXPECT_EQ(nullptr, cameraBinding.getRamsesCamera());
        cameraBinding.setRamsesCamera(&perspectiveCam);
        EXPECT_EQ(&perspectiveCam, cameraBinding.getRamsesCamera());
        EXPECT_EQ(ramses::ERamsesObjectType::ERamsesObjectType_PerspectiveCamera, cameraBinding.m_cameraBinding->getCameraType());

        cameraBinding.setRamsesCamera(nullptr);
        EXPECT_EQ(nullptr, cameraBinding.getRamsesCamera());
        EXPECT_EQ(ramses::ERamsesObjectType::ERamsesObjectType_Invalid, cameraBinding.m_cameraBinding->getCameraType());

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&orthoCam);
        EXPECT_EQ(&orthoCam, cameraBinding.getRamsesCamera());
        EXPECT_EQ(ramses::ERamsesObjectType::ERamsesObjectType_OrthographicCamera, cameraBinding.m_cameraBinding->getCameraType());
    }

    TEST_F(ARamsesCameraBinding, ClearsInputsAfterSettingCameraToNullptr)
    {
        RamsesCameraBinding& cameraBinding = createCameraBindingForTest("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding.setRamsesCamera(&perspectiveCam);

        const auto inputs = cameraBinding.getInputs();
        ASSERT_EQ(2u, inputs->getChildCount());

        const auto vpProperties = inputs->getChild("viewPortProperties");
        const auto frustumProperties = inputs->getChild("frustumProperties");
        ASSERT_EQ(4u, vpProperties->getChildCount());
        ASSERT_EQ(4u, frustumProperties->getChildCount());

        cameraBinding.setRamsesCamera(nullptr);
        ASSERT_EQ(0u, cameraBinding.getInputs()->getChildCount());
    }

    TEST_F(ARamsesCameraBinding, SwitchingBetweenCameraTypesRecreatesRespectiveSetOfInputsWithDefaultValues)
    {
        RamsesCameraBinding& cameraBinding = createCameraBindingForTest("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();

        ASSERT_EQ(0u, cameraBinding.getInputs()->getChildCount());
        cameraBinding.setRamsesCamera(&perspectiveCam);

        const auto perspectiveInputs = cameraBinding.getInputs();
        const auto vpPropertiesPerspective = perspectiveInputs->getChild("viewPortProperties");
        const auto frustumPropertiesPerspective = perspectiveInputs->getChild("frustumProperties");
        ASSERT_EQ(2u, perspectiveInputs->getChildCount());
        ASSERT_EQ(4u, vpPropertiesPerspective->getChildCount());
        ASSERT_EQ(4u, frustumPropertiesPerspective->getChildCount());

        EXPECT_EQ(*vpPropertiesPerspective->getChild("viewPortOffsetX")->get<int32_t>(), DefaultViewportOffsetX);
        EXPECT_EQ(*vpPropertiesPerspective->getChild("viewPortOffsetY")->get<int32_t>(), DefaultViewportOffsetY);
        EXPECT_EQ(*vpPropertiesPerspective->getChild("viewPortWidth")->get<int32_t>(), static_cast<int32_t>(DefaultViewportWidth));
        EXPECT_EQ(*vpPropertiesPerspective->getChild("viewPortHeight")->get<int32_t>(), static_cast<int32_t>(DefaultViewportHeight));
        EXPECT_NEAR(*frustumPropertiesPerspective->getChild("fieldOfView")->get<float>(), PerspectiveFrustumFOVdefault, 0.001f);
        EXPECT_EQ(*frustumPropertiesPerspective->getChild("aspectRatio")->get<float>(), PerspectiveFrustumARdefault);
        EXPECT_EQ(*frustumPropertiesPerspective->getChild("nearPlane")->get<float>(), NearPlaneDefault);
        EXPECT_EQ(*frustumPropertiesPerspective->getChild("farPlane")->get<float>(), FarPlaneDefault);

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&orthoCam);

        const auto orthoInputs = cameraBinding.getInputs();
        const auto vpPropertiesOrtho = orthoInputs->getChild("viewPortProperties");
        const auto frustumPropertiesOrtho = orthoInputs->getChild("frustumProperties");
        ASSERT_EQ(2u, orthoInputs->getChildCount());
        ASSERT_EQ(4u, vpPropertiesOrtho->getChildCount());
        ASSERT_EQ(6u, frustumPropertiesOrtho->getChildCount());

        EXPECT_EQ(*vpPropertiesOrtho->getChild("viewPortOffsetX")->get<int32_t>(), DefaultViewportOffsetX);
        EXPECT_EQ(*vpPropertiesOrtho->getChild("viewPortOffsetY")->get<int32_t>(), DefaultViewportOffsetY);
        EXPECT_EQ(*vpPropertiesOrtho->getChild("viewPortWidth")->get<int32_t>(), static_cast<int32_t>(DefaultViewportWidth));
        EXPECT_EQ(*vpPropertiesOrtho->getChild("viewPortHeight")->get<int32_t>(), static_cast<int32_t>(DefaultViewportHeight));
        EXPECT_NEAR(*frustumPropertiesOrtho->getChild("leftPlane")->get<float>(), OrthoFrustumLPdefault, 0.001f);
        EXPECT_EQ(*frustumPropertiesOrtho->getChild("rightPlane")->get<float>(), OrthoFrustumRPdefault);
        EXPECT_EQ(*frustumPropertiesOrtho->getChild("bottomPlane")->get<float>(), OrthoFrustumBPdefault);
        EXPECT_EQ(*frustumPropertiesOrtho->getChild("topPlane")->get<float>(), OrthoFrustumTPdefault);
        EXPECT_EQ(*frustumPropertiesOrtho->getChild("nearPlane")->get<float>(), NearPlaneDefault);
        EXPECT_EQ(*frustumPropertiesOrtho->getChild("farPlane")->get<float>(), FarPlaneDefault);
    }

    TEST_F(ARamsesCameraBinding, HasInputsAfterSettingPerspectiveCamera)
    {
        RamsesCameraBinding& cameraBinding = createCameraBindingForTest("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();

        cameraBinding.setRamsesCamera(&perspectiveCam);
        const auto inputs = cameraBinding.getInputs();
        ASSERT_EQ(2u, inputs->getChildCount());

        const auto vpProperties = inputs->getChild("viewPortProperties");
        const auto frustumProperties = inputs->getChild("frustumProperties");
        ASSERT_EQ(4u, vpProperties->getChildCount());
        ASSERT_EQ(4u, frustumProperties->getChildCount());

        const auto vpOffsetX = vpProperties->getChild("viewPortOffsetX");
        const auto vpOffsety = vpProperties->getChild("viewPortOffsetY");
        const auto vpWidth = vpProperties->getChild("viewPortWidth");
        const auto vpHeight = vpProperties->getChild("viewPortHeight");

        const auto nP = frustumProperties->getChild("nearPlane");
        const auto fP = frustumProperties->getChild("farPlane");
        const auto fov = frustumProperties->getChild("fieldOfView");
        const auto aR = frustumProperties->getChild("aspectRatio");

        // Test that internal indices match properties resolved by name
        EXPECT_EQ(vpProperties, inputs->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Viewport)));
        EXPECT_EQ(frustumProperties, inputs->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum)));

        EXPECT_EQ(vpOffsetX, vpProperties->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX)));
        EXPECT_EQ(vpOffsety, vpProperties->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetY)));
        EXPECT_EQ(vpWidth, vpProperties->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth)));
        EXPECT_EQ(vpHeight, vpProperties->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight)));
        EXPECT_EQ(nP, frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane)));
        EXPECT_EQ(fP, frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane)));
        EXPECT_EQ(fov, frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView)));
        EXPECT_EQ(aR, frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio)));

        ExpectPropertyTypeAndChildCount(inputs->getChild("viewPortProperties"), EPropertyType::Struct, 4);
        ExpectPropertyTypeAndChildCount(inputs->getChild("frustumProperties"), EPropertyType::Struct, 4);

        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortOffsetX"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortOffsetY"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortWidth"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortHeight"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("nearPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("farPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("fieldOfView"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("aspectRatio"), EPropertyType::Float, 0);
    }

    TEST_F(ARamsesCameraBinding, HasInputsAfterSettingOrthoCamera)
    {
        RamsesCameraBinding& cameraBinding = createCameraBindingForTest("");
        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();

        cameraBinding.setRamsesCamera(&orthoCam);
        const auto inputs = cameraBinding.getInputs();
        ASSERT_EQ(2u, inputs->getChildCount());

        const auto vpProperties = inputs->getChild("viewPortProperties");
        const auto frustumProperties = inputs->getChild("frustumProperties");
        ASSERT_EQ(4u, vpProperties->getChildCount());
        ASSERT_EQ(6u, frustumProperties->getChildCount());

        const auto vpOffsetX = vpProperties->getChild("viewPortOffsetX");
        const auto vpOffsety = vpProperties->getChild("viewPortOffsetY");
        const auto vpWidth = vpProperties->getChild("viewPortWidth");
        const auto vpHeight = vpProperties->getChild("viewPortHeight");
        const auto nP = frustumProperties->getChild("nearPlane");
        const auto fP = frustumProperties->getChild("farPlane");
        const auto lp = frustumProperties->getChild("leftPlane");
        const auto rP = frustumProperties->getChild("rightPlane");
        const auto bP = frustumProperties->getChild("bottomPlane");
        const auto tP = frustumProperties->getChild("topPlane");

        // Test that internal indices match properties resolved by name
        EXPECT_EQ(vpProperties, inputs->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Viewport)));
        EXPECT_EQ(frustumProperties, inputs->getChild(static_cast<size_t>(ECameraPropertyStructStaticIndex::Frustum)));

        EXPECT_EQ(vpOffsetX, vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX)));
        EXPECT_EQ(vpOffsety, vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex ::ViewPortOffsetY)));
        EXPECT_EQ(vpWidth, vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth)));
        EXPECT_EQ(vpHeight, vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight)));
        EXPECT_EQ(nP, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::NearPlane)));
        EXPECT_EQ(fP, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::FarPlane)));
        EXPECT_EQ(lp, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::LeftPlane)));
        EXPECT_EQ(rP, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::RightPlane)));
        EXPECT_EQ(bP, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::BottomPlane)));
        EXPECT_EQ(tP, frustumProperties->m_impl->getChild(static_cast<size_t>(EOrthographicCameraFrustumPropertyStaticIndex::TopPlane)));

        ExpectPropertyTypeAndChildCount(inputs->getChild("viewPortProperties"), EPropertyType::Struct, 4);
        ExpectPropertyTypeAndChildCount(inputs->getChild("frustumProperties"), EPropertyType::Struct, 6);

        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortOffsetX"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortOffsetY"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortWidth"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(vpProperties->getChild("viewPortHeight"), EPropertyType::Int32, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("nearPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("farPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("leftPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("rightPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("bottomPlane"), EPropertyType::Float, 0);
        ExpectPropertyTypeAndChildCount(frustumProperties->getChild("topPlane"), EPropertyType::Float, 0);
    }

    TEST_F(ARamsesCameraBinding, DoesNotOverwriteDefaultValues_WhenOrthoCameraAssigned)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&orthoCam);
        m_logicEngine.update();

        //Expect default values on the camera, because nothing was set so far
        ExpectDefaultViewportValues(orthoCam);
        ExpectDefaultOrthoCameraFrustumValues(orthoCam);
    }

    TEST_F(ARamsesCameraBinding, DoesNotOverwriteDefaultValues_WhenPerspectiveCameraAssigned)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::PerspectiveCamera& perspCam = createPerspectiveCameraForTest();
        cameraBinding.setRamsesCamera(&perspCam);
        m_logicEngine.update();

        //Expect default values on the camera, because nothing was set so far
        ExpectDefaultViewportValues(perspCam);
        ExpectDefaultPerspectiveCameraFrustumValues(perspCam);
    }

    TEST_F(ARamsesCameraBinding, ReportsErrorOnUpdate_WhenSettingZeroToViewportSize)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::OrthographicCamera& camera = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&camera);
        const auto inputs = cameraBinding.getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        // Setting illegal viewport values: width and height cannot be 0 so an error will be produced on ramses camera
        vpProperties->getChild("viewPortWidth")->set<int32_t>(0);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Camera viewport size must be positive! (width: 0; height: 16)");

        // Fix width, break height -> still generates error
        vpProperties->getChild("viewPortWidth")->set<int32_t>(8);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(0);

        // Expect default values on the camera, because setting values failed
        ExpectDefaultViewportValues(camera);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Camera viewport size must be positive! (width: 8; height: 0)");

        // Fix height and update recovers the errors
        vpProperties->getChild("viewPortHeight")->set<int32_t>(32);
        EXPECT_TRUE(m_logicEngine.update());

        EXPECT_EQ(camera.getViewportWidth(), 8u);
        EXPECT_EQ(camera.getViewportHeight(), 32u);
    }

    TEST_F(ARamsesCameraBinding, ReportsErrorOnUpdate_WhenSettingNegativeViewportSize)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::OrthographicCamera& camera = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&camera);
        const auto inputs = cameraBinding.getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        // Setting illegal viewport values: width and height cannot be 0 so an error will be produced on ramses camera
        vpProperties->getChild("viewPortWidth")->set<int32_t>(-1);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(-1);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Camera viewport size must be positive! (width: -1; height: -1)");

        // Setting positive values recovers from the error
        vpProperties->getChild("viewPortWidth")->set<int32_t>(10);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(12);
        EXPECT_TRUE(m_logicEngine.update());

        EXPECT_EQ(camera.getViewportWidth(), 10u);
        EXPECT_EQ(camera.getViewportHeight(), 12u);
    }

    TEST_F(ARamsesCameraBinding, ReportsErrorOnUpdate_WhenSettingInvalidFrustumValuesOnOrthoCamera)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&orthoCam);

        auto frustumProperties = cameraBinding.getInputs()->getChild("frustumProperties");
        // Setting illegal frustum values: left plane cannot be smaller than right plane so an error will be produced on ramses camera
        frustumProperties->getChild("leftPlane")->set<float>(2.f);
        frustumProperties->getChild("rightPlane")->set<float>(1.f);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "Camera::setFrustum failed - check validity of given frustum planes");

        //Still expect default values on the camera, because setting values failed
        ExpectDefaultOrthoCameraFrustumValues(orthoCam);

        // Recovers from the error once values are ok
        frustumProperties->getChild("rightPlane")->set<float>(3.f);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ARamsesCameraBinding, ReportsErrorOnUpdate_WhenSettingInvalidFrustumValuesOnPerspectiveCamera)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding.setRamsesCamera(&perspectiveCam);

        auto frustumProperties = cameraBinding.getInputs()->getChild("frustumProperties");
        // Setting illegal frustum values: fov and aspect ratio cannot be 0 so an error will be produced on ramses camera
        frustumProperties->getChild("fieldOfView")->set<float>(0.f);
        frustumProperties->getChild("aspectRatio")->set<float>(0.f);

        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "PerspectiveCamera::setFrustum failed - check validity of given frustum planes");

        // Fixing just the FOV does not fix the issue, need to also fix aspect ratio
        frustumProperties->getChild("fieldOfView")->set<float>(15.f);
        EXPECT_FALSE(m_logicEngine.update());
        ASSERT_EQ(m_logicEngine.getErrors().size(), 1u);
        EXPECT_EQ(m_logicEngine.getErrors()[0].message, "PerspectiveCamera::setFrustum failed - check validity of given frustum planes");

        //Still expect default values on the camera, because setting values failed
        ExpectDefaultViewportValues(perspectiveCam);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        frustumProperties->getChild("aspectRatio")->set<float>(1.f);
        EXPECT_TRUE(m_logicEngine.update());
    }

    TEST_F(ARamsesCameraBinding, InitializesInputPropertiesOfPerpespectiveCameraToMatchRamsesDefaultValues)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");

        const auto inputs = cameraBinding.getInputs();
        ASSERT_NE(nullptr, inputs);
        EXPECT_EQ(0u, inputs->getChildCount());

        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding.setRamsesCamera(&perspectiveCam);

        const auto vpProperties = inputs->getChild("viewPortProperties");
        const auto frustumProperties = inputs->getChild("frustumProperties");
        ASSERT_EQ(4u, vpProperties->getChildCount());
        ASSERT_EQ(4u, frustumProperties->getChildCount());

        EXPECT_EQ(*vpProperties->getChild("viewPortOffsetX")->get<int32_t>(), perspectiveCam.getViewportX());
        EXPECT_EQ(*vpProperties->getChild("viewPortOffsetY")->get<int32_t>(), perspectiveCam.getViewportY());
        EXPECT_EQ(static_cast<uint32_t>(*vpProperties->getChild("viewPortWidth")->get<int32_t>()), perspectiveCam.getViewportWidth());
        EXPECT_EQ(static_cast<uint32_t>(*vpProperties->getChild("viewPortHeight")->get<int32_t>()), perspectiveCam.getViewportHeight());

        EXPECT_EQ(*frustumProperties->getChild("nearPlane")->get<float>(), perspectiveCam.getNearPlane());
        EXPECT_EQ(*frustumProperties->getChild("farPlane")->get<float>(), perspectiveCam.getFarPlane());
        EXPECT_NEAR(*frustumProperties->getChild("fieldOfView")->get<float>(), perspectiveCam.getVerticalFieldOfView(), 0.001f);
        EXPECT_EQ(*frustumProperties->getChild("aspectRatio")->get<float>(), perspectiveCam.getAspectRatio());
    }

    TEST_F(ARamsesCameraBinding, InitializesInputPropertiesOfOrthographicCameraToMatchRamsesDefaultValues)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");

        const auto inputs = cameraBinding.getInputs();
        ASSERT_NE(nullptr, inputs);
        EXPECT_EQ(0u, inputs->getChildCount());

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding.setRamsesCamera(&orthoCam);

        const auto vpProperties = inputs->getChild("viewPortProperties");
        const auto frustumProperties = inputs->getChild("frustumProperties");
        ASSERT_EQ(4u, vpProperties->getChildCount());
        ASSERT_EQ(6u, frustumProperties->getChildCount());

        EXPECT_EQ(*vpProperties->getChild("viewPortOffsetX")->get<int32_t>(), orthoCam.getViewportX());
        EXPECT_EQ(*vpProperties->getChild("viewPortOffsetY")->get<int32_t>(), orthoCam.getViewportY());
        EXPECT_EQ(static_cast<uint32_t>(*vpProperties->getChild("viewPortWidth")->get<int32_t>()), orthoCam.getViewportWidth());
        EXPECT_EQ(static_cast<uint32_t>(*vpProperties->getChild("viewPortHeight")->get<int32_t>()), orthoCam.getViewportHeight());

        EXPECT_EQ(*frustumProperties->getChild("nearPlane")->get<float>(), orthoCam.getNearPlane());
        EXPECT_EQ(*frustumProperties->getChild("farPlane")->get<float>(), orthoCam.getFarPlane());
        EXPECT_EQ(*frustumProperties->getChild("leftPlane")->get<float>(), orthoCam.getLeftPlane());
        EXPECT_EQ(*frustumProperties->getChild("rightPlane")->get<float>(), orthoCam.getRightPlane());
        EXPECT_EQ(*frustumProperties->getChild("bottomPlane")->get<float>(), orthoCam.getBottomPlane());
        EXPECT_EQ(*frustumProperties->getChild("topPlane")->get<float>(), orthoCam.getTopPlane());
    }

    TEST_F(ARamsesCameraBinding, MarksInputsAsBindingInputsForPerspectiveCameraBinding)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding->setRamsesCamera(&perspectiveCam);

        const auto inputs = cameraBinding->getInputs();
        for (size_t i = 0; i < inputs->getChildCount(); ++i)
        {
            const auto inputStruct = inputs->getChild(i);
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, inputStruct->m_impl->getPropertySemantics());

            for (size_t j = 0; j < inputs->getChild(i)->getChildCount(); ++j)
            {
                const auto inputProperty = inputStruct->m_impl->getChild(j);
                EXPECT_EQ(internal::EPropertySemantics::BindingInput, inputProperty->m_impl->getPropertySemantics());
            }
        }
    }

    TEST_F(ARamsesCameraBinding, MarksInputsAsBindingInputsForOrthoCameraBinding)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");
        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding->setRamsesCamera(&orthoCam);

        const auto inputs = cameraBinding->getInputs();
        for (size_t i = 0; i < inputs->getChildCount(); ++i)
        {
            const auto inputStruct = inputs->getChild(i);
            EXPECT_EQ(internal::EPropertySemantics::BindingInput, inputStruct->m_impl->getPropertySemantics());

            for (size_t j = 0; j < inputs->getChild(i)->getChildCount(); ++j)
            {
                const auto inputProperty = inputStruct->m_impl->getChild(j);
                EXPECT_EQ(internal::EPropertySemantics::BindingInput, inputProperty->m_impl->getPropertySemantics());
            }
        }
    }

    TEST_F(ARamsesCameraBinding, ReturnsBoundRamsesCamera)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding->setRamsesCamera(&perspectiveCam);

        EXPECT_EQ(&perspectiveCam, cameraBinding->getRamsesCamera());

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding->setRamsesCamera(&orthoCam);
        EXPECT_EQ(&orthoCam, cameraBinding->getRamsesCamera());

        cameraBinding->setRamsesCamera(nullptr);
        EXPECT_EQ(nullptr, cameraBinding->getRamsesCamera());
    }

    TEST_F(ARamsesCameraBinding, DoesNotModifyRamsesWithoutUpdateBeingCalledWithPerspectiveCamera)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding->setRamsesCamera(&perspectiveCam);

        auto inputs = cameraBinding->getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        auto frustumProperties = inputs->getChild("frustumProperties");

        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(4);
        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(7);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(11);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(19);

        frustumProperties->getChild("nearPlane")->set<float>(3.1f);
        frustumProperties->getChild("farPlane")->set<float>(.2f);
        frustumProperties->getChild("fieldOfView")->set<float>(4.2f);
        frustumProperties->getChild("aspectRatio")->set<float>(2.1f);

        ExpectDefaultValues(perspectiveCam);
    }

    TEST_F(ARamsesCameraBinding, DoesNotModifyRamsesWithoutUpdateBeingCalledWithOrthoCamera)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding->setRamsesCamera(&orthoCam);

        auto inputs = cameraBinding->getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        auto frustumProperties = inputs->getChild("frustumProperties");

        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(4);
        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(7);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(11);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(19);

        frustumProperties->getChild("nearPlane")->set<float>(3.1f);
        frustumProperties->getChild("farPlane")->set<float>(.2f);
        frustumProperties->getChild("leftPlane")->set<float>(6.2f);
        frustumProperties->getChild("rightPlane")->set<float>(2.8f);
        frustumProperties->getChild("bottomPlane")->set<float>(1.9f);
        frustumProperties->getChild("topPlane")->set<float>(7.1f);

        ExpectDefaultValues(orthoCam);
    }

    TEST_F(ARamsesCameraBinding, ModifiesRamsesPerspectiveCamOnUpdate_OnlyAfterExplicitlyAssignedToInputs)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding->setRamsesCamera(&perspectiveCam);

        auto inputs = cameraBinding->getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        auto frustumProperties = inputs->getChild("frustumProperties");

        const int32_t newVpOffsetX = 23;
        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(newVpOffsetX);

        // Update not called yet -> still default values
        ExpectDefaultValues(perspectiveCam);

        cameraBinding->m_cameraBinding->update();
        // Only propagated vpOffsetX, the others have default values
        EXPECT_EQ(perspectiveCam.getViewportX(), newVpOffsetX);
        EXPECT_EQ(perspectiveCam.getViewportY(), 0);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 16u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 16u);

        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Set and test all properties
        const int32_t newVpOffsetY = 13;
        const int32_t newVpWidth = 56;
        const int32_t newVpHeight = 45;

        const float newFov = 30.f;
        const float newAR = 640.f / 480.f;
        const float newNearPlane = 4.4f;
        const float newFarPlane = 5.1f;

        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(newVpOffsetY);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(newVpWidth);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(newVpHeight);

        frustumProperties->getChild("fieldOfView")->set<float>(newFov);
        frustumProperties->getChild("aspectRatio")->set<float>(newAR);
        frustumProperties->getChild("nearPlane")->set<float>(newNearPlane);
        frustumProperties->getChild("farPlane")->set<float>(newFarPlane);
        cameraBinding->m_cameraBinding->update();

        EXPECT_EQ(perspectiveCam.getViewportX(), newVpOffsetX);
        EXPECT_EQ(perspectiveCam.getViewportY(), newVpOffsetY);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), static_cast<uint32_t>(newVpWidth));
        EXPECT_EQ(perspectiveCam.getViewportHeight(), static_cast<uint32_t>(newVpHeight));

        EXPECT_NEAR(perspectiveCam.getVerticalFieldOfView(), newFov, 0.001f);
        EXPECT_EQ(perspectiveCam.getAspectRatio(), newAR);
        EXPECT_EQ(perspectiveCam.getNearPlane(), newNearPlane);
        EXPECT_EQ(perspectiveCam.getFarPlane(), newFarPlane);
    }

    TEST_F(ARamsesCameraBinding, ModifiesRamsesOrthoCamOnUpdate_OnlyAfterExplicitlyAssignedToInputs)
    {
        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding->setRamsesCamera(&orthoCam);

        auto inputs = cameraBinding->getInputs();
        auto vpProperties = inputs->getChild("viewPortProperties");
        auto frustumProperties = inputs->getChild("frustumProperties");

        const int32_t newVpOffsetX = 23;
        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(newVpOffsetX);

        // Update not called yet -> still default values
        ExpectDefaultValues(orthoCam);

        cameraBinding->m_cameraBinding->update();
        // Only propagated vpOffsetX, the others have default values
        EXPECT_EQ(orthoCam.getViewportX(), newVpOffsetX);
        EXPECT_EQ(orthoCam.getViewportY(), 0);
        EXPECT_EQ(orthoCam.getViewportWidth(), 16u);
        EXPECT_EQ(orthoCam.getViewportHeight(), 16u);

        ExpectDefaultOrthoCameraFrustumValues(orthoCam);

        // Set and test all properties
        const int32_t newVpOffsetY = 13;
        const int32_t newVpWidth = 56;
        const int32_t newVpHeight = 45;

        const float newLeftPlane = 0.2f;
        const float newRightPlane = 0.3f;
        const float newBottomPlane = 0.4f;
        const float newTopPlane = 0.5f;
        const float newNearPlane = 4.f;
        const float newFarPlane = 5.1f;

        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(newVpOffsetY);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(newVpWidth);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(newVpHeight);

        frustumProperties->getChild("leftPlane")->set<float>(newLeftPlane);
        frustumProperties->getChild("rightPlane")->set<float>(newRightPlane);
        frustumProperties->getChild("bottomPlane")->set<float>(newBottomPlane);
        frustumProperties->getChild("topPlane")->set<float>(newTopPlane);
        frustumProperties->getChild("nearPlane")->set<float>(newNearPlane);
        frustumProperties->getChild("farPlane")->set<float>(newFarPlane);
        cameraBinding->m_cameraBinding->update();

        EXPECT_EQ(orthoCam.getViewportX(), newVpOffsetX);
        EXPECT_EQ(orthoCam.getViewportY(), newVpOffsetY);
        EXPECT_EQ(orthoCam.getViewportWidth(), static_cast<uint32_t>(newVpWidth));
        EXPECT_EQ(orthoCam.getViewportHeight(), static_cast<uint32_t>(newVpHeight));

        EXPECT_EQ(orthoCam.getLeftPlane(), newLeftPlane);
        EXPECT_EQ(orthoCam.getRightPlane(), newRightPlane);
        EXPECT_EQ(orthoCam.getBottomPlane(), newBottomPlane);
        EXPECT_EQ(orthoCam.getTopPlane(), newTopPlane);
        EXPECT_EQ(orthoCam.getNearPlane(), newNearPlane);
        EXPECT_EQ(orthoCam.getFarPlane(), newFarPlane);
    }

    TEST_F(ARamsesCameraBinding, PropagatesItsInputsToRamsesPerspectiveCameraOnUpdate_WithLinksInsteadOfSetCall)
    {
        const std::string_view scriptSrc = R"(
            function interface()
                OUT.vpProps = {
                    vpX = INT,
                    vpY = INT,
                    vpW = INT,
                    vpH = INT
                }
            end
            function run()
                OUT.vpProps = {
                    vpX = 5,
                    vpY = 10,
                    vpW = 35,
                    vpH = 19
                }
            end
        )";

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);

        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding->setRamsesCamera(&perspectiveCam);

        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpProps")->getChild("vpX"), *cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpProps")->getChild("vpY"), *cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetY")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpProps")->getChild("vpW"), *cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortWidth")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpProps")->getChild("vpH"), *cameraBinding->getInputs()->getChild("viewPortProperties")->getChild("viewPortHeight")));

        // Links have no effect before update() explicitly called
        ExpectDefaultValues(perspectiveCam);

        m_logicEngine.update();

        // Linked values got updates, not-linked values were not modified
        EXPECT_EQ(perspectiveCam.getViewportX(), 5);
        EXPECT_EQ(perspectiveCam.getViewportY(), 10);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 35u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 19u);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);
    }

    TEST_F(ARamsesCameraBinding, PropagatesItsInputsToRamsesOrthoCameraOnUpdate_WithLinksInsteadOfSetCall)
    {
        const std::string_view scriptSrc = R"(
            function interface()
                OUT.frustProps = {
                    lP = FLOAT,
                    rP = FLOAT,
                    bP = FLOAT,
                    tP = FLOAT,
                    nP = FLOAT,
                    fP = FLOAT
                }
            end
            function run()
                OUT.frustProps = {
                    lP = 0.2,
                    rP = 0.3,
                    bP = 0.4,
                    tP = 0.5,
                    nP = 0.6,
                    fP = 0.7
                }
            end
        )";

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);

        auto* cameraBinding = m_logicEngine.createRamsesCameraBinding("");

        ramses::OrthographicCamera& orthoCam = createOrthoCameraForTest();
        cameraBinding->setRamsesCamera(&orthoCam);

        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("lP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("leftPlane")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("rP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("rightPlane")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("bP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("bottomPlane")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("tP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("topPlane")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("nP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("nearPlane")));
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("fP"), *cameraBinding->getInputs()->getChild("frustumProperties")->getChild("farPlane")));

        // Links have no effect before update() explicitly called
        ExpectDefaultValues(orthoCam);

        m_logicEngine.update();

        // Linked values got updates, not-linked values were not modified
        EXPECT_EQ(orthoCam.getLeftPlane(), 0.2f);
        EXPECT_EQ(orthoCam.getRightPlane(), 0.3f);
        EXPECT_EQ(orthoCam.getBottomPlane(), 0.4f);
        EXPECT_EQ(orthoCam.getTopPlane(), 0.5f);
        EXPECT_EQ(orthoCam.getNearPlane(), 0.6f);
        EXPECT_EQ(orthoCam.getFarPlane(), 0.7f);
        ExpectDefaultViewportValues(orthoCam);
    }

    TEST_F(ARamsesCameraBinding, DoesNotOverrideExistingValuesAfterRamsesCameraIsAssignedToBinding)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();

        perspectiveCam.setViewport(3, 4, 10u, 11u);
        perspectiveCam.setFrustum(30.f, 640.f / 480.f, 2.3f, 5.6f);

        cameraBinding.setRamsesCamera(&perspectiveCam);

        EXPECT_EQ(perspectiveCam.getViewportX(), 3);
        EXPECT_EQ(perspectiveCam.getViewportY(), 4);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 10u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 11u);

        EXPECT_NEAR(perspectiveCam.getVerticalFieldOfView(), 30.f, 0.001f);
        EXPECT_EQ(perspectiveCam.getAspectRatio(), 640.f / 480.f);
        EXPECT_EQ(perspectiveCam.getNearPlane(), 2.3f);
        EXPECT_EQ(perspectiveCam.getFarPlane(), 5.6f);
    }

    TEST_F(ARamsesCameraBinding, StopsPropagatingValuesAfterTargetCameraSetToNull)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        cameraBinding.setRamsesCamera(&perspectiveCam);

        const int32_t vpOffsetXvalue1 = 34;
        const int32_t vpOffsetXvalue2 = 52;

        auto inputs = cameraBinding.getInputs();
        inputs->getChild("viewPortProperties")->getChild("viewPortOffsetX")->set<int32_t>(vpOffsetXvalue1);

        cameraBinding.m_cameraBinding->update();

        EXPECT_EQ(perspectiveCam.getViewportX(), vpOffsetXvalue1);

        inputs->getChild("viewPortProperties")->getChild("viewPortOffsetX")->set<int32_t>(vpOffsetXvalue2);
        cameraBinding.setRamsesCamera(nullptr);
        EXPECT_EQ(std::nullopt, cameraBinding.m_cameraBinding->update());
        EXPECT_EQ(perspectiveCam.getViewportX(), vpOffsetXvalue1);
    }



    //don't repeat internal:: everywhere
    using internal::RamsesCameraBindingImpl;

    // This fixture only contains serialization unit tests, for higher order tests see `ARamsesCameraBinding_SerializationWithFile`
    class ARamsesCameraBinding_SerializationLifecycle : public ::testing::Test
    {
    protected:
        flatbuffers::FlatBufferBuilder m_flatBufferBuilder;
    };

    // More unit tests with inputs/outputs declared in LogicNode (base class) serialization tests
    TEST_F(ARamsesCameraBinding_SerializationLifecycle, RemembersBaseClassData)
    {
        // Serialize
        {
            RamsesCameraBindingImpl binding("name");
            (void)RamsesCameraBindingImpl::Serialize(binding, m_flatBufferBuilder);
        }

        // Inspect flatbuffers data
        const rlogic_serialization::RamsesCameraBinding& serializedBinding = *rlogic_serialization::GetRamsesCameraBinding(m_flatBufferBuilder.GetBufferPointer());

        ASSERT_TRUE(serializedBinding.logicnode());
        ASSERT_TRUE(serializedBinding.logicnode()->name());
        EXPECT_EQ(serializedBinding.logicnode()->name()->string_view(), "name");

        ASSERT_TRUE(serializedBinding.logicnode()->inputs());
        EXPECT_EQ(serializedBinding.logicnode()->inputs()->rootType(), rlogic_serialization::EPropertyRootType::Struct);
        ASSERT_TRUE(serializedBinding.logicnode()->inputs()->children());
        EXPECT_EQ(serializedBinding.logicnode()->inputs()->children()->size(), 0u);

        EXPECT_FALSE(serializedBinding.logicnode()->outputs());

        // Deserialize
        {
            std::unique_ptr<RamsesCameraBindingImpl> deserializedBinding = RamsesCameraBindingImpl::Deserialize(serializedBinding, nullptr);

            ASSERT_TRUE(deserializedBinding);
            EXPECT_EQ(deserializedBinding->getName(), "name");
            EXPECT_EQ(deserializedBinding->getInputs()->getType(), EPropertyType::Struct);
            EXPECT_EQ(deserializedBinding->getInputs()->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(deserializedBinding->getInputs()->getName(), "IN");
            EXPECT_EQ(deserializedBinding->getInputs()->getChildCount(), 0u);
        }
    }


    TEST_F(ARamsesCameraBinding_SerializationLifecycle, RemembersRamsesCameraId)
    {
        RamsesTestSetup ramses;
        ramses::Camera* testCamera = ramses.createScene()->createOrthographicCamera();

        // Serialize
        {
            RamsesCameraBindingImpl binding("");
            binding.setRamsesCamera(testCamera);
            (void)RamsesCameraBindingImpl::Serialize(binding, m_flatBufferBuilder);
        }

        // Inspect flatbuffers data
        const rlogic_serialization::RamsesCameraBinding& serializedBinding = *rlogic_serialization::GetRamsesCameraBinding(m_flatBufferBuilder.GetBufferPointer());

        EXPECT_EQ(serializedBinding.ramsesCamera(), testCamera->getSceneObjectId().getValue());

        // Deserialize
        {
            std::unique_ptr<RamsesCameraBindingImpl> deserializedBinding = RamsesCameraBindingImpl::Deserialize(serializedBinding, testCamera);

            ASSERT_TRUE(deserializedBinding);
            EXPECT_EQ(deserializedBinding->getRamsesCamera(), testCamera);
        }
    }

    class ARamsesCameraBinding_SerializationWithFile : public ARamsesCameraBinding
    {
    protected:
        WithTempDirectory tempFolder;
    };

    TEST_F(ARamsesCameraBinding_SerializationWithFile, ContainsItsDataAfterLoading)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        const int32_t newVpOffsetX = 10;
        const int32_t newVpOffsetY = 13;
        const int32_t newVpWidth = 56;
        const int32_t newVpHeight = 45;

        const float newFov = 30.f;
        const float newAR = 640.f / 480.f;
        const float newNearPlane = 4.4f;
        const float newFarPlane = 5.1f;
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);

            auto inputs = cameraBinding.getInputs();
            auto vpProperties = inputs->getChild("viewPortProperties");
            auto frustumProperties = inputs->getChild("frustumProperties");

            vpProperties->getChild("viewPortOffsetX")->set<int32_t>(newVpOffsetX);
            vpProperties->getChild("viewPortOffsetY")->set<int32_t>(newVpOffsetY);
            vpProperties->getChild("viewPortWidth")->set<int32_t>(newVpWidth);
            vpProperties->getChild("viewPortHeight")->set<int32_t>(newVpHeight);

            frustumProperties->getChild("fieldOfView")->set<float>(newFov);
            frustumProperties->getChild("aspectRatio")->set<float>(newAR);
            frustumProperties->getChild("nearPlane")->set<float>(newNearPlane);
            frustumProperties->getChild("farPlane")->set<float>(newFarPlane);
            m_logicEngine.update();
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));
            const auto& loadedCameraBinding = *m_logicEngine.findCameraBinding("CameraBinding");
            EXPECT_EQ("CameraBinding", loadedCameraBinding.getName());
            EXPECT_EQ(loadedCameraBinding.getRamsesCamera()->getSceneObjectId(), perspectiveCam.getSceneObjectId());

            const auto& inputs = loadedCameraBinding.getInputs();
            ASSERT_EQ(inputs->getChildCount(), 2u);
            auto vpProperties = inputs->getChild("viewPortProperties");
            auto frustumProperties = inputs->getChild("frustumProperties");
            ASSERT_EQ(vpProperties->getChildCount(), 4u);
            ASSERT_EQ(vpProperties->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            ASSERT_EQ(frustumProperties->getChildCount(), 4u);
            ASSERT_EQ(frustumProperties->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);

            EXPECT_EQ(*vpProperties->getChild("viewPortOffsetX")->get<int32_t>(), newVpOffsetX);
            EXPECT_EQ(vpProperties->getChild("viewPortOffsetX")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(*vpProperties->getChild("viewPortOffsetY")->get<int32_t>(), newVpOffsetY);
            EXPECT_EQ(vpProperties->getChild("viewPortOffsetY")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(*vpProperties->getChild("viewPortWidth")->get<int32_t>(), newVpWidth);
            EXPECT_EQ(vpProperties->getChild("viewPortWidth")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(*vpProperties->getChild("viewPortHeight")->get<int32_t>(), newVpHeight);
            EXPECT_EQ(vpProperties->getChild("viewPortHeight")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);

            EXPECT_EQ(*frustumProperties->getChild("nearPlane")->get<float>(), newNearPlane);
            EXPECT_EQ(frustumProperties->getChild("nearPlane")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(*frustumProperties->getChild("farPlane")->get<float>(), newFarPlane);
            EXPECT_EQ(frustumProperties->getChild("farPlane")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_NEAR(*frustumProperties->getChild("fieldOfView")->get<float>(), newFov, 0.001f);
            EXPECT_EQ(frustumProperties->getChild("fieldOfView")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);
            EXPECT_EQ(*frustumProperties->getChild("aspectRatio")->get<float>(), newAR);
            EXPECT_EQ(frustumProperties->getChild("aspectRatio")->m_impl->getPropertySemantics(), internal::EPropertySemantics::BindingInput);

            // Test that internal indices match properties resolved by name
            EXPECT_EQ(vpProperties->getChild("viewPortOffsetX"), vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetX)));
            EXPECT_EQ(vpProperties->getChild("viewPortOffsetY"), vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortOffsetY)));
            EXPECT_EQ(vpProperties->getChild("viewPortWidth"), vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortWidth)));
            EXPECT_EQ(vpProperties->getChild("viewPortHeight"), vpProperties->m_impl->getChild(static_cast<size_t>(ECameraViewportPropertyStaticIndex::ViewPortHeight)));

            EXPECT_EQ(frustumProperties->getChild("nearPlane"), frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::NearPlane)));
            EXPECT_EQ(frustumProperties->getChild("farPlane"), frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FarPlane)));
            EXPECT_EQ(frustumProperties->getChild("fieldOfView"), frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::FieldOfView)));
            EXPECT_EQ(frustumProperties->getChild("aspectRatio"), frustumProperties->m_impl->getChild(static_cast<size_t>(EPerspectiveCameraFrustumPropertyStaticIndex::AspectRatio)));
        }
    }

    TEST_F(ARamsesCameraBinding_SerializationWithFile, KeepsItsProperties_WhenNoRamsesLinksAndSceneProvided)
    {
        {
            createCameraBindingForTest("CameraBinding");
            ASSERT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }

        {
            ASSERT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin"));
            auto loadedCameraBinding = m_logicEngine.findCameraBinding("CameraBinding");
            EXPECT_EQ(loadedCameraBinding->getRamsesCamera(), nullptr);
            EXPECT_EQ(loadedCameraBinding->getInputs()->getChildCount(), 0u);
            EXPECT_EQ(loadedCameraBinding->getOutputs(), nullptr);
            EXPECT_EQ(loadedCameraBinding->getName(), "CameraBinding");
        }
    }

    TEST_F(ARamsesCameraBinding_SerializationWithFile, RestoresLinkToRamsesCamera)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));
            const auto& cameraBinding = *m_logicEngine.findCameraBinding("CameraBinding");
            EXPECT_EQ(cameraBinding.getRamsesCamera(), &perspectiveCam);
        }
    }

    TEST_F(ARamsesCameraBinding_SerializationWithFile, ProducesError_WhenHavingLinkToRamsesCamera_ButNoSceneWasProvided)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }
        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("camerabinding.bin"));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0].message, "Fatal error during loading from file! Serialized Ramses Logic object 'CameraBinding' points to a Ramses object (id: 1), but no Ramses scene was provided to resolve the Ramses object!");
        }
    }

    TEST_F(ARamsesCameraBinding_SerializationWithFile, ProducesError_WhenHavingLinkToRamsesCamera_WhichWasDeleted)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }

        m_testScene.destroy(perspectiveCam);

        {
            EXPECT_FALSE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));
            auto errors = m_logicEngine.getErrors();
            ASSERT_EQ(errors.size(), 1u);
            EXPECT_EQ(errors[0].message, "Fatal error during loading from file! Serialized Ramses Logic object 'CameraBinding' points to a Ramses object (id: 1) which couldn't be found in the provided scene!");
        }
    }

    TEST_F(ARamsesCameraBinding_SerializationWithFile, DoesNotModifyRamsesCameraProperties_WhenNoValuesWereExplicitlySetBeforeSaving)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }
        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));
            EXPECT_TRUE(m_logicEngine.update());

            ExpectDefaultValues(perspectiveCam);
        }
    }

    // Tests that the camera properties don't overwrite ramses' values after loading from file, until
    // set() is called again explicitly after loadFromFile()
    TEST_F(ARamsesCameraBinding_SerializationWithFile, ReappliesAllPropertiesOfOneStructToRamsesCamera_WhenExplicitlySetAgain)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        {
            auto& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);
            // Set some values to the binding's inputs
            auto vpProperties = cameraBinding.getInputs()->getChild("viewPortProperties");
            vpProperties->getChild("viewPortOffsetX")->set<int32_t>(4);
            vpProperties->getChild("viewPortOffsetY")->set<int32_t>(5);
            vpProperties->getChild("viewPortWidth")->set<int32_t>(6);
            vpProperties->getChild("viewPortHeight")->set<int32_t>(7);

            auto frustumProperties = cameraBinding.getInputs()->getChild("frustumProperties");
            frustumProperties->getChild("fieldOfView")->set<float>(30.f);
            frustumProperties->getChild("aspectRatio")->set<float>(640.f / 480.f);
            frustumProperties->getChild("nearPlane")->set<float>(2.3f);
            frustumProperties->getChild("farPlane")->set<float>(5.6f);
            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }

        // Set viewport properties to different values to check if they are overwritten after load
        perspectiveCam.setViewport(9, 8, 1u, 2u);

        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));

            EXPECT_TRUE(m_logicEngine.update());

            // Camera binding does not re-apply its values to ramses camera viewport
            EXPECT_EQ(perspectiveCam.getViewportX(), 9);
            EXPECT_EQ(perspectiveCam.getViewportY(), 8);
            EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
            EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
            ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

            // Set only one value of viewport struct. Use the same value as before save on purpose! Calling set forces set on ramses
            m_logicEngine.findCameraBinding("CameraBinding")->getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")->set<int32_t>(4);
            EXPECT_TRUE(m_logicEngine.update());

            // vpOffsetX changed, the rest is taken from the initially saved inputs, not what was set on the camera!
            EXPECT_EQ(perspectiveCam.getViewportX(), 4);
            EXPECT_EQ(perspectiveCam.getViewportY(), 5);
            EXPECT_EQ(perspectiveCam.getViewportWidth(), 6u);
            EXPECT_EQ(perspectiveCam.getViewportHeight(), 7u);
            // frustum values will still be default values as there was no set of any value on the frustum struct and thus the update doesn't change the struct
            ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);
        }
    }

    // This is sort of a confidence test, testing a combination of:
    // - bindings only propagating their values to ramses camera if the value was set by an incoming link
    // - saving and loading files
    // The general expectation is that after loading + update(), the logic scene would overwrite only ramses
    // properties wrapped by a LogicBinding which is linked to a script
    TEST_F(ARamsesCameraBinding_SerializationWithFile, SetsOnlyRamsesCameraPropertiesForWhichTheBindingInputIsLinked_WhenCallingUpdateAfterLoading)
    {
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();

        // These values should not be overwritten by logic on update()
        perspectiveCam.setViewport(9, 8, 1u, 2u);

        {
            const std::string_view scriptSrc = R"(
            function interface()
                OUT.frustProps = {
                    fov = FLOAT,
                    aR = FLOAT,
                    nP = FLOAT,
                    fP = FLOAT
                }
            end
            function run()
                OUT.frustProps = {
                    fov = 30.0,
                    aR = 640.0 / 480.0,
                    nP = 2.3,
                    fP = 5.6
                }
            end
            )";

            LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);

            RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");
            cameraBinding.setRamsesCamera(&perspectiveCam);

            ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("fov"), *cameraBinding.getInputs()->getChild("frustumProperties")->getChild("fieldOfView")));
            ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("aR"), *cameraBinding.getInputs()->getChild("frustumProperties")->getChild("aspectRatio")));
            ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("nP"), *cameraBinding.getInputs()->getChild("frustumProperties")->getChild("nearPlane")));
            ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("frustProps")->getChild("fP"), *cameraBinding.getInputs()->getChild("frustumProperties")->getChild("farPlane")));

            EXPECT_TRUE(m_logicEngine.saveToFile("camerabinding.bin"));
        }

        // Modify 'linked' properties before loading to check if logic will overwrite them after load + update
        perspectiveCam.setFrustum(15.f, 320.f / 240.f, 4.1f, 7.9f);

        {
            EXPECT_TRUE(m_logicEngine.loadFromFile("camerabinding.bin", &m_testScene));

            EXPECT_TRUE(m_logicEngine.update());

            // Viewport properties were not linked -> their values are not modified
            EXPECT_EQ(perspectiveCam.getViewportX(), 9);
            EXPECT_EQ(perspectiveCam.getViewportY(), 8);
            EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
            EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
            // Frustum properties are linked -> values were updated
            EXPECT_NEAR(perspectiveCam.getVerticalFieldOfView(), 30.f, 0.001f);
            EXPECT_EQ(perspectiveCam.getAspectRatio(), 640.f / 480.f);
            EXPECT_EQ(perspectiveCam.getNearPlane(), 2.3f);
            EXPECT_EQ(perspectiveCam.getFarPlane(), 5.6f);

            // Manually setting values on ramses followed by a logic update has no effect
            // Logic is not "dirty" and it doesn't know it needs to update ramses
            perspectiveCam.setViewport(43, 34, 84u, 62u);
            EXPECT_TRUE(m_logicEngine.update());
            EXPECT_EQ(perspectiveCam.getViewportX(), 43);
            EXPECT_EQ(perspectiveCam.getViewportY(), 34);
            EXPECT_EQ(perspectiveCam.getViewportWidth(), 84u);
            EXPECT_EQ(perspectiveCam.getViewportHeight(), 62u);
        }
    }

    // Larger confidence tests which verify and document the entire data flow cycle of bindings
    // There are smaller tests which test only properties and their data propagation rules (see property unit tests)
    // There are also "dirtiness" tests which test when a camera is being re-updated (see logic engine dirtiness tests)
    // These tests test everything in combination

    class ARamsesCameraBinding_DataFlow : public ARamsesCameraBinding
    {
    };

    TEST_F(ARamsesCameraBinding_DataFlow, WithExplicitSet)
    {
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("");

        // Create camera and preset values
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        perspectiveCam.setViewport(9, 8, 1u, 2u);

        cameraBinding.setRamsesCamera(&perspectiveCam);

        m_logicEngine.update();

        // Nothing happened - binding did not overwrite preset values because no user value set()
        EXPECT_EQ(perspectiveCam.getViewportX(), 9);
        EXPECT_EQ(perspectiveCam.getViewportY(), 8);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Set only two view port properties
        auto vpProperties = cameraBinding.getInputs()->getChild("viewPortProperties");
        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(4);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(21);

        // Update not called yet -> still has preset values for vpOffsetX and vpWidth in ramses camera
        EXPECT_EQ(perspectiveCam.getViewportX(), 9);
        EXPECT_EQ(perspectiveCam.getViewportY(), 8);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Update() triggers all viewPortProperties to be set on ramses to the two values that were explicitly set
        // and the other two default values of the binding
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 4);
        EXPECT_EQ(perspectiveCam.getViewportY(), DefaultViewportOffsetY);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 21u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), DefaultViewportHeight);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Set two properties of each viewPort and frustum property struct
        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(13);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(63);
        auto frustumProperties = cameraBinding.getInputs()->getChild("frustumProperties");
        frustumProperties->getChild("nearPlane")->set<float>(2.3f);
        frustumProperties->getChild("farPlane")->set<float>(5.6f);

        // On update all values of both structs are set
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 4);
        EXPECT_EQ(perspectiveCam.getViewportY(), 13);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 21u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 63u);

        EXPECT_NEAR(perspectiveCam.getVerticalFieldOfView(), PerspectiveFrustumFOVdefault, 0.001f);
        EXPECT_EQ(perspectiveCam.getAspectRatio(), PerspectiveFrustumARdefault);
        EXPECT_EQ(perspectiveCam.getNearPlane(), 2.3f);
        EXPECT_EQ(perspectiveCam.getFarPlane(), 5.6f);

        // Calling update again does not "rewrite" the data to ramses. Check this by setting a value manually and call update() again
        perspectiveCam.setViewport(9, 8, 1u, 2u);
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 9);
        EXPECT_EQ(perspectiveCam.getViewportY(), 8);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);

        // Set all properties manually this time
        vpProperties->getChild("viewPortOffsetX")->set<int32_t>(4);
        vpProperties->getChild("viewPortOffsetY")->set<int32_t>(5);
        vpProperties->getChild("viewPortWidth")->set<int32_t>(6);
        vpProperties->getChild("viewPortHeight")->set<int32_t>(7);

        frustumProperties->getChild("fieldOfView")->set<float>(30.f);
        frustumProperties->getChild("aspectRatio")->set<float>(640.f / 480.f);
        frustumProperties->getChild("nearPlane")->set<float>(1.3f);
        frustumProperties->getChild("farPlane")->set<float>(7.6f);
        m_logicEngine.update();

        // All of the property values were passed to ramses
        EXPECT_EQ(perspectiveCam.getViewportX(), 4);
        EXPECT_EQ(perspectiveCam.getViewportY(), 5);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 6u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 7u);

        EXPECT_NEAR(perspectiveCam.getVerticalFieldOfView(), 30.f, 0.001f);
        EXPECT_EQ(perspectiveCam.getAspectRatio(), 640.f / 480.f);
        EXPECT_EQ(perspectiveCam.getNearPlane(), 1.3f);
        EXPECT_EQ(perspectiveCam.getFarPlane(), 7.6f);
    }

    TEST_F(ARamsesCameraBinding_DataFlow, WithLinks)
    {
        const std::string_view scriptSrc = R"(
            function interface()
                OUT.vpOffsetX = INT
            end
            function run()
                OUT.vpOffsetX = 14
            end
        )";

        LuaScript* script = m_logicEngine.createLuaScriptFromSource(scriptSrc);
        RamsesCameraBinding& cameraBinding = *m_logicEngine.createRamsesCameraBinding("CameraBinding");

        // Create camera and preset values
        ramses::PerspectiveCamera& perspectiveCam = createPerspectiveCameraForTest();
        perspectiveCam.setViewport(9, 8, 1u, 2u);

        cameraBinding.setRamsesCamera(&perspectiveCam);

        // Adding and removing link does not set anything in ramses
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpOffsetX"), *cameraBinding.getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")));
        ASSERT_TRUE(m_logicEngine.unlink(*script->getOutputs()->getChild("vpOffsetX"), *cameraBinding.getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")));
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 9);
        EXPECT_EQ(perspectiveCam.getViewportY(), 8);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Create link and calling update -> sets values to ramses
        ASSERT_TRUE(m_logicEngine.link(*script->getOutputs()->getChild("vpOffsetX"), *cameraBinding.getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")));
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 14);
        EXPECT_EQ(perspectiveCam.getViewportY(), DefaultViewportOffsetY);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), DefaultViewportWidth);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), DefaultViewportHeight);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // As long as link is active, binding overwrites value which was manually set directly to the ramses camera
        perspectiveCam.setViewport(9, 8, 1u, 2u);
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 14);
        EXPECT_EQ(perspectiveCam.getViewportY(), DefaultViewportOffsetY);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), DefaultViewportWidth);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), DefaultViewportHeight);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);

        // Remove link -> value is not overwritten any more
        ASSERT_TRUE(m_logicEngine.unlink(*script->getOutputs()->getChild("vpOffsetX"), *cameraBinding.getInputs()->getChild("viewPortProperties")->getChild("viewPortOffsetX")));
        perspectiveCam.setViewport(9, 8, 1u, 2u);
        m_logicEngine.update();
        EXPECT_EQ(perspectiveCam.getViewportX(), 9);
        EXPECT_EQ(perspectiveCam.getViewportY(), 8);
        EXPECT_EQ(perspectiveCam.getViewportWidth(), 1u);
        EXPECT_EQ(perspectiveCam.getViewportHeight(), 2u);
        ExpectDefaultPerspectiveCameraFrustumValues(perspectiveCam);
    }
}
