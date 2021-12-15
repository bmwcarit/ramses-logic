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

namespace rlogic::internal
{
    class LogicNodeDummyImpl : public LogicNodeImpl
    {
    public:
        explicit LogicNodeDummyImpl(std::string_view name, bool createNestedProperties = false)
            : LogicNodeImpl(name, 1u)
        {
            setRootProperties(
                std::make_unique<Property>(std::make_unique<PropertyImpl>(CreateTestInputsType(createNestedProperties), EPropertySemantics::ScriptInput)),
                std::make_unique<Property>(std::make_unique<PropertyImpl>(CreateTestOutputsType(createNestedProperties), EPropertySemantics::ScriptOutput)));
        }

        std::optional<LogicNodeRuntimeError> update() override
        {
            return std::nullopt;
        }

    private:
        static HierarchicalTypeData CreateTestInputsType(bool createNestedProperties)
        {
            HierarchicalTypeData inputsStruct = MakeStruct("IN", {
                {"input1", EPropertyType::Int32},
                {"input2", EPropertyType::Int32},
                });

            if (createNestedProperties)
            {
                inputsStruct.children.emplace_back(MakeStruct("inputStruct", { {"nested", EPropertyType::Int32} }));
                inputsStruct.children.emplace_back(MakeArray("inputArray", 1, EPropertyType::Int32));
            }

            return inputsStruct;
        }

        static HierarchicalTypeData CreateTestOutputsType(bool createNestedProperties)
        {
            HierarchicalTypeData outputsStruct = MakeStruct("OUT", {
                {"output1", EPropertyType::Int32},
                {"output2", EPropertyType::Int32},
                });

            if (createNestedProperties)
            {
                outputsStruct.children.emplace_back(MakeStruct("outputStruct", { {"nested", EPropertyType::Int32} }));
                outputsStruct.children.emplace_back(MakeArray("outputArray", 1, EPropertyType::Int32));
            }

            return outputsStruct;
        }
    };
}

// TODO Violin delete this class, not needed for tests - should be able to test with impl only
namespace rlogic
{
    class LogicNodeDummy : public LogicNode
    {
    public:
        explicit LogicNodeDummy(std::unique_ptr<internal::LogicNodeDummyImpl> impl)
            : LogicNode(std::move(impl))
            /* NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast) */
            , m_node(static_cast<internal::LogicNodeDummyImpl&>(LogicNode::m_impl))
        {
        }
        static std::unique_ptr<LogicNodeDummy> Create(std::string_view name)
        {
            return std::make_unique<LogicNodeDummy>(std::make_unique<internal::LogicNodeDummyImpl>(name));
        }

        internal::LogicNodeDummyImpl& m_node;
    };
}
