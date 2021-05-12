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
#include "ramses-logic/Property.h"

namespace rlogic
{
    class LogicNodeDummyImpl : public internal::LogicNodeImpl
    {
    public:
        explicit LogicNodeDummyImpl(std::string_view name, bool createNestedProperties = false)
            : internal::LogicNodeImpl(name,
                std::make_unique<internal::PropertyImpl>("IN", EPropertyType::Struct, internal::EPropertySemantics::ScriptInput),
                std::make_unique<internal::PropertyImpl>("OUT", EPropertyType::Struct, internal::EPropertySemantics::ScriptOutput))
        {
            getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("input1", EPropertyType::Int32, internal::EPropertySemantics::ScriptInput));
            getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("input2", EPropertyType::Int32, internal::EPropertySemantics::ScriptInput));

            getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("output1", EPropertyType::Int32, internal::EPropertySemantics::ScriptOutput));
            getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("output2", EPropertyType::Int32, internal::EPropertySemantics::ScriptOutput));

            if (createNestedProperties)
            {
                getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("inputStruct", EPropertyType::Struct, internal::EPropertySemantics::ScriptInput));
                getInputs()->getChild("inputStruct")->m_impl->addChild(std::make_unique<internal::PropertyImpl>("nested", EPropertyType::Int32, internal::EPropertySemantics::ScriptInput));

                getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("outputStruct", EPropertyType::Struct, internal::EPropertySemantics::ScriptOutput));
                getOutputs()->getChild("outputStruct")->m_impl->addChild(std::make_unique<internal::PropertyImpl>("nested", EPropertyType::Int32, internal::EPropertySemantics::ScriptOutput));

                getInputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("inputArray", EPropertyType::Array, internal::EPropertySemantics::ScriptInput));
                getInputs()->getChild("inputArray")->m_impl->addChild(std::make_unique<internal::PropertyImpl>("", EPropertyType::Int32, internal::EPropertySemantics::ScriptInput));

                getOutputs()->m_impl->addChild(std::make_unique<internal::PropertyImpl>("outputArray", EPropertyType::Array, internal::EPropertySemantics::ScriptOutput));
                getOutputs()->getChild("outputArray")->m_impl->addChild(std::make_unique<internal::PropertyImpl>("", EPropertyType::Int32, internal::EPropertySemantics::ScriptOutput));
            }
        }

        std::optional<internal::LogicNodeRuntimeError> update() override
        {
            return std::nullopt;
        }

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
