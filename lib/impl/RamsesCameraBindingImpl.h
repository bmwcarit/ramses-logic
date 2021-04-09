//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/RamsesBindingImpl.h"

#include <memory>
#include "ramses-client-api/RamsesObjectTypes.h"

namespace ramses
{
    class Camera;
}

namespace rlogic_serialization
{
    struct RamsesCameraBinding;
}

namespace flatbuffers
{
    class FlatBufferBuilder;
    template <typename T> struct Offset;
}

namespace rlogic::internal
{
    class PropertyImpl;
    class ErrorReporting;

    enum class ECameraPropertyStructStaticIndex : size_t
    {
        Viewport = 0,
        Frustum = 1,
    };

    enum class ECameraViewportPropertyStaticIndex : size_t
    {
        ViewPortOffsetX = 0,
        ViewPortOffsetY = 1,
        ViewPortWidth = 2,
        ViewPortHeight = 3,
    };

    enum class EPerspectiveCameraFrustumPropertyStaticIndex : size_t
    {
        NearPlane = 0,
        FarPlane = 1,
        FieldOfView = 2,
        AspectRatio = 3,
    };

    enum class EOrthographicCameraFrustumPropertyStaticIndex : size_t
    {
        NearPlane = 0,
        FarPlane = 1,
        LeftPlane = 2,
        RightPlane = 3,
        BottomPlane = 4,
        TopPlane = 5,
    };

    class RamsesCameraBindingImpl : public RamsesBindingImpl
    {
    public:
        [[nodiscard]] static std::unique_ptr<RamsesCameraBindingImpl> Create(std::string_view name);
        [[nodiscard]] static std::unique_ptr<RamsesCameraBindingImpl> Create(const rlogic_serialization::RamsesCameraBinding& cameraBinding, ramses::Camera* camera);

        void setRamsesCamera(ramses::Camera* camera);
        [[nodiscard]] ramses::Camera* getRamsesCamera() const;
        [[nodiscard]] ramses::ERamsesObjectType getCameraType() const;

        bool update() override;

        flatbuffers::Offset<rlogic_serialization::RamsesCameraBinding> serialize(flatbuffers::FlatBufferBuilder& builder) const;
    private:
        explicit RamsesCameraBindingImpl(std::string_view name);
        RamsesCameraBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, ramses::Camera& camera);
        ramses::Camera* m_camera = nullptr;

        void CreateCameraProperties(ramses::ERamsesObjectType cameraType);
    };
}
