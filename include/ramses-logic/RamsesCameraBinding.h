//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/RamsesBinding.h"

#include <memory>

namespace ramses
{
    class Camera;
}

namespace rlogic::internal
{
    class RamsesCameraBindingImpl;
}

namespace rlogic
{
    /**
     * The RamsesCameraBinding is a type of #rlogic::RamsesBinding which allows the #rlogic::LogicEngine to control instances
     * of ramses::Camera. RamsesCameraBinding's can be created with #rlogic::LogicEngine::createRamsesCameraBinding.
     *
     * There are two types of ramses::Camera:
     *  - ramses::PerspectiveCamera
     *  - ramses::OrthographicCamera.
     * Both camera types are defined through their viewport and their frustum properties. These are represented as two separate property structs
     * in the RamsesCameraBinding. Be aware if you set one or more values to one of the structs on the binding and update the LogicEngine it will lead
     * to all properties of this struct being set on the actual ramses::Camera.
     * For example if you only set the ViewportOffsetX of the Camera per linked script or directly on the binding it will set
     * ViewportOffsetX, ViewportOffsetY, ViewportWidth and ViewportHeight to whatever the state of the properties is at that moment.
     * So if you haven't set the other three properties on the binding before, the ramses::Camera will be updated to the default values
     * for those. The frustum values of the ramses::Camera are not affected when setting viewport values, and vice-versa. Check the ramses::Camera API to see which values belong together.
     * To avoid unexpected behavior, we highly recommend setting all viewport values together, and also setting all frustum planes together (either by link or by setting them directly).
     * This way unwanted behaviour can be avoided.
     *
     * Since the RamsesCameraBinding derives from #rlogic::RamsesBinding, it also provides the #rlogic::LogicNode::getInputs
     * and #rlogic::LogicNode::getOutputs method. For this particular implementation, the methods behave as follows:
     *  - #rlogic::LogicNode::getInputs: returns an empty struct with no child properties if no ramses::Camera is currently assigned
     *  - #rlogic::LogicNode::getInputs: returns inputs struct with two child properties: viewPortProperties and frustumProperties.
     *                                   Their child properties in turn vary for the two available camera types (ramses::PerspectiveCamera and ramses::OrthographicCamera)
     *      - Perspective Camera:  viewPortProperties --> viewPortOffsetX, viewPortOffsetY, viewPortWidth, viewPortHeight
     *                             frustumProperties  --> fieldOfView, aspectRatio, nearPlane, farPlane
     *      - Orthographic Camera: viewPortProperties --> same as for Perspective Camera
     *                             frustumProperties  --> leftPlane, rightPlane, bottomPlane, topPlane, nearPlane, farPlane
     *  - #rlogic::LogicNode::getOutputs: returns always nullptr, because a #RamsesCameraBinding does not have outputs,
     *    it implicitly controls the ramses Camera
     */
    class RamsesCameraBinding : public RamsesBinding
    {
    public:
        /**
         * Links this #RamsesCameraBinding with a ramses::Camera. After this call, #rlogic::LogicNode::getInputs will
         * return a struct property with children equivalent to the camera settings of the provided ramses \p camera. Setting the
         * Ramses camera to nullptr will erase all inputs, and further calls with different cameras will overwrite the
         * inputs according to the new camera. Bear in mind that after a call to #setRamsesCamera, pointers to properties of
         * this #RamsesCameraBinding from before the call will be invalid and must be re-queried, even if some or all of
         * the new camera's properties have the same name or type or even if you assign a pointer to the same camera again!
         *
         * @param camera to bind the RamsesCameraBinding to.
         */
        RLOGIC_API void setRamsesCamera(ramses::Camera* camera);

        /**
         * Returns the currently assigned Ramses Camera (or nullptr if none was assigned).
         * @return the currently bound ramses camera
         */
        [[nodiscard]] RLOGIC_API ramses::Camera* getRamsesCamera() const;

        /**
         * Constructor of RamsesCameraBinding. User is not supposed to call this - RamsesCameraBindings are created by other factory classes
         *
         * @param impl implementation details of the RamsesCameraBinding
         */
        explicit RamsesCameraBinding(std::unique_ptr<internal::RamsesCameraBindingImpl> impl) noexcept;

        /**
         * Destructor of RamsesCameraBinding.
         */
        ~RamsesCameraBinding() noexcept override;

        /**
         * Copy Constructor of RamsesCameraBinding is deleted because RamsesCameraBindings are not supposed to be copied
         *
         * @param other RamsesNodeBindings to copy from
         */
        RamsesCameraBinding(const RamsesCameraBinding& other) = delete;

        /**
         * Move Constructor of RamsesCameraBinding is deleted because RamsesCameraBindings are not supposed to be moved
         *
         * @param other RamsesCameraBinding to move from
         */
        RamsesCameraBinding(RamsesCameraBinding&& other) = delete;

        /**
         * Assignment operator of RamsesCameraBinding is deleted because RamsesCameraBindings are not supposed to be copied
         *
         * @param other RamsesCameraBinding to assign from
         */
        RamsesCameraBinding& operator=(const RamsesCameraBinding& other) = delete;

        /**
         * Move assignment operator of RamsesCameraBinding is deleted because RamsesCameraBindings are not supposed to be moved
         *
         * @param other RamsesCameraBinding to assign from
         */
        RamsesCameraBinding& operator=(RamsesCameraBinding&& other) = delete;

        /**
         * Implementation detail of RamsesCameraBinding
         */
        std::unique_ptr<internal::RamsesCameraBindingImpl> m_cameraBinding;
    };
}
