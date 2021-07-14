//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesNodeBindingImpl.h"

#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/Property.h"

namespace rlogic
{
    RamsesNodeBinding::RamsesNodeBinding(std::unique_ptr<internal::RamsesNodeBindingImpl> impl) noexcept
        // The impl pointer is owned by this class, but a reference to the data is passed to the base class
        // TODO MSVC2017 fix for std::reference_wrapper with base class
        : RamsesBinding(std::ref(static_cast<internal::RamsesBindingImpl&>(*impl)))
        , m_nodeBinding(std::move(impl))
    {
    }

    ramses::Node& RamsesNodeBinding::getRamsesNode() const
    {
        return m_nodeBinding->getRamsesNode();
    }

    bool RamsesNodeBinding::setRotationConvention(ramses::ERotationConvention rotationConvention)
    {
        return m_nodeBinding->setRotationConvention(rotationConvention);
    }

    ramses::ERotationConvention RamsesNodeBinding::getRotationConvention() const
    {
        return m_nodeBinding->getRotationConvention();
    }

    RamsesNodeBinding::~RamsesNodeBinding() noexcept = default;
}
