//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesCameraBindingImpl.h"

#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/Property.h"

#include "impl/RamsesBindingImpl.h"

namespace rlogic
{
    RamsesCameraBinding::RamsesCameraBinding(std::unique_ptr<internal::RamsesCameraBindingImpl> impl) noexcept
        // The impl pointer is owned by this class, but a reference to the data is passed to the base class
        // TODO MSVC2017 fix for std::reference_wrapper with base class
        : RamsesBinding(std::ref(static_cast<internal::RamsesBindingImpl&>(*impl)))
        , m_cameraBinding(std::move(impl))
    {
    }

    RamsesCameraBinding::~RamsesCameraBinding() noexcept = default;

    ramses::Camera& RamsesCameraBinding::getRamsesCamera() const
    {
        return m_cameraBinding->getRamsesCamera();
    }
}
