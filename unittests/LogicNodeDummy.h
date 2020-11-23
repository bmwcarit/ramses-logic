//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

#include "ramses-logic/LogicNode.h"

namespace rlogic
{
    class LogicNodeDummyImpl : public internal::LogicNodeImpl
    {
    public:
        explicit LogicNodeDummyImpl(std::string_view name)
            : internal::LogicNodeImpl(name,
                std::make_unique<internal::PropertyImpl>("IN", EPropertyType::Struct, internal::EInputOutputProperty::Input),
                std::make_unique<internal::PropertyImpl>("OUT", EPropertyType::Struct, internal::EInputOutputProperty::Output))
            , updateId(0)
        {
            getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("input1", EPropertyType::Int32, internal::EInputOutputProperty::Input));
            getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("input2", EPropertyType::Int32, internal::EInputOutputProperty::Input));

            getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("output1", EPropertyType::Int32, internal::EInputOutputProperty::Output));
            getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("output2", EPropertyType::Int32, internal::EInputOutputProperty::Output));
        }

        bool update() override
        {
            updateId = ++UpdateCounter;
            return true;
        }
        uint32_t        updateId;
        static uint32_t UpdateCounter;

    private:
    };

    class LogicNodeDummy : public LogicNode
    {
    public:
        explicit LogicNodeDummy(std::unique_ptr<LogicNodeDummyImpl> impl)
            : LogicNode(std::ref(static_cast<internal::LogicNodeImpl&>(*impl)))
            , m_node(std::move(impl))
        {
        }
        static std::unique_ptr<LogicNodeDummy> Create(std::string_view name)
        {
            return std::make_unique<LogicNodeDummy>(std::make_unique<LogicNodeDummyImpl>(name));
        }

        std::unique_ptr<LogicNodeDummyImpl> m_node;
    };
}
