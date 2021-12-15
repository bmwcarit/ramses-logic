//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicObject.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "impl/LogicObjectImpl.h"

namespace rlogic
{
    LogicObject::LogicObject(std::unique_ptr<internal::LogicObjectImpl> impl) noexcept
        : m_impl{ std::move(impl) }
    {
    }

    LogicObject::~LogicObject() noexcept = default;

    std::string_view LogicObject::getName() const
    {
        return m_impl->getName();
    }

    uint64_t LogicObject::getId() const
    {
        return m_impl->getId();
    }

    template <typename T>
    const T* LogicObject::internalCast() const
    {
        return dynamic_cast<const T*>(this);
    }

    template <typename T>
    T* LogicObject::internalCast()
    {
        return dynamic_cast<T*>(this);
    }

    void LogicObject::setName(std::string_view name)
    {
        m_impl->setName(name);
    }

    template RLOGIC_API const LogicObject*             LogicObject::internalCast() const;
    template RLOGIC_API const LogicNode*               LogicObject::internalCast() const;
    template RLOGIC_API const RamsesBinding*           LogicObject::internalCast() const;
    template RLOGIC_API const LuaModule*               LogicObject::internalCast() const;
    template RLOGIC_API const LuaScript*               LogicObject::internalCast() const;
    template RLOGIC_API const RamsesNodeBinding*       LogicObject::internalCast() const;
    template RLOGIC_API const RamsesAppearanceBinding* LogicObject::internalCast() const;
    template RLOGIC_API const RamsesCameraBinding*     LogicObject::internalCast() const;
    template RLOGIC_API const DataArray*               LogicObject::internalCast() const;
    template RLOGIC_API const AnimationNode*           LogicObject::internalCast() const;
    template RLOGIC_API const TimerNode*               LogicObject::internalCast() const;

    template RLOGIC_API LogicObject*             LogicObject::internalCast();
    template RLOGIC_API LogicNode*               LogicObject::internalCast();
    template RLOGIC_API RamsesBinding*           LogicObject::internalCast();
    template RLOGIC_API LuaModule*               LogicObject::internalCast();
    template RLOGIC_API LuaScript*               LogicObject::internalCast();
    template RLOGIC_API RamsesNodeBinding*       LogicObject::internalCast();
    template RLOGIC_API RamsesAppearanceBinding* LogicObject::internalCast();
    template RLOGIC_API RamsesCameraBinding*     LogicObject::internalCast();
    template RLOGIC_API DataArray*               LogicObject::internalCast();
    template RLOGIC_API AnimationNode*           LogicObject::internalCast();
    template RLOGIC_API TimerNode*               LogicObject::internalCast();
}
