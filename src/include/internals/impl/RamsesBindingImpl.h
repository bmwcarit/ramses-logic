//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/impl/LogicNodeImpl.h"

namespace rlogic::internal
{
    // Currently this is just a common base class for all ramses bindings
    class RamsesBindingImpl : public LogicNodeImpl
    {
    public:
        // Move-able (noexcept); Not copy-able
        ~RamsesBindingImpl() noexcept override                           = default;
        RamsesBindingImpl(RamsesBindingImpl&& other) noexcept            = default;
        RamsesBindingImpl& operator=(RamsesBindingImpl&& other) noexcept = default;
        RamsesBindingImpl(const RamsesBindingImpl& other)                = delete;
        RamsesBindingImpl& operator=(const RamsesBindingImpl& other)     = delete;

    protected:
        explicit RamsesBindingImpl(std::string_view name) noexcept;
        RamsesBindingImpl(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs);
    };
}
