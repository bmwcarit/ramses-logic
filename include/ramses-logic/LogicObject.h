//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include <string>
#include <memory>

namespace rlogic::internal
{
    class LogicObjectImpl;
}

namespace rlogic
{
    /**
    * A base class for all rlogic API objects
    */
    class LogicObject
    {
    public:
        /**
        * Returns the name of this object.
        *
        * @return the name of this object
        */
        [[nodiscard]] RLOGIC_API std::string_view getName() const;

        /**
        * Sets the name of this object.
        *
        * @param name new name of the object
        */
        RLOGIC_API void setName(std::string_view name);

        /**
        * Deleted copy constructor
        */
        LogicObject(const LogicObject&) = delete;

        /**
        * Deleted assignment operator
        */
        LogicObject& operator=(const LogicObject&) = delete;

    protected:
        /**
        * Constructor of #LogicObject. User is not supposed to call this - LogcNodes are created by subclasses
        *
        * @param impl implementation details of the #LogicObject
        */
        explicit LogicObject(std::unique_ptr<internal::LogicObjectImpl> impl) noexcept;

        /**
        * Destructor of #LogicObject
        */
        virtual ~LogicObject() noexcept;

        std::unique_ptr<internal::LogicObjectImpl> m_impl;
    };
}
