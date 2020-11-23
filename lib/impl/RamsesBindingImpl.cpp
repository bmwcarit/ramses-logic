//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------


#include "impl/RamsesBindingImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/Property.h"

namespace rlogic::internal
{
    RamsesBindingImpl::RamsesBindingImpl(std::string_view name) noexcept
        : LogicNodeImpl(name)
    {
    }

    RamsesBindingImpl::RamsesBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs)
        : LogicNodeImpl(name, std::move(inputs), std::move(outputs))
    {
    }
}
