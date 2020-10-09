//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/impl/RamsesAppearanceBindingImpl.h"

#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/Property.h"

#include "internals/impl/RamsesBindingImpl.h"

namespace rlogic
{
    RamsesAppearanceBinding::RamsesAppearanceBinding(std::unique_ptr<internal::RamsesAppearanceBindingImpl> impl) noexcept
        // The impl pointer is owned by this class, but a reference to the data is passed to the base class
        // TODO MSVC2017 fix for std::reference_wrapper with base class
        : RamsesBinding(std::ref(static_cast<internal::RamsesBindingImpl&>(*impl)))
        , m_appearanceBinding(std::move(impl))
    {
    }

    RamsesAppearanceBinding::~RamsesAppearanceBinding() noexcept = default;

    void RamsesAppearanceBinding::setRamsesAppearance(ramses::Appearance* appearance)
    {
        m_appearanceBinding->setRamsesAppearance(appearance);
    }

    ramses::Appearance* RamsesAppearanceBinding::getRamsesAppearance() const
    {
        return m_appearanceBinding->getRamsesAppearance();
    }
}
