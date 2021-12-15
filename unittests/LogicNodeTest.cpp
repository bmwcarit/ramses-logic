//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gmock/gmock.h"

#include "ramses-logic/Property.h"
#include "impl/LogicNodeImpl.h"
#include "impl/PropertyImpl.h"

namespace rlogic::internal
{
    class LogicNodeImplMock : public LogicNodeImpl
    {
    public:
        // Forwarding constructor
        explicit LogicNodeImplMock(std::string_view name)
            : LogicNodeImpl(name, 1u)
        {
        }

        // Expose protected method for testing
        using LogicNodeImpl::setRootProperties;

        MOCK_METHOD(std::optional<LogicNodeRuntimeError>, update, (), (override, final));
    };

    class ALogicNodeImpl : public ::testing::Test
    {
    };

    TEST_F(ALogicNodeImpl, RemembersNameGivenInConstructor)
    {
        const LogicNodeImplMock logicNode("name");
        EXPECT_EQ(logicNode.getName(), "name");
    }

    TEST_F(ALogicNodeImpl, CanReceiveNewName)
    {
        LogicNodeImplMock logicNode("name");
        logicNode.setName("newName");
        EXPECT_EQ(logicNode.getName(), "newName");
    }

    TEST_F(ALogicNodeImpl, DirtyByDefault)
    {
        const LogicNodeImplMock logicNode("");
        EXPECT_TRUE(logicNode.isDirty());
    }

    TEST_F(ALogicNodeImpl, DirtyWhenSetDirty)
    {
        LogicNodeImplMock logicNode("");
        logicNode.setDirty(false);
        EXPECT_FALSE(logicNode.isDirty());
        logicNode.setDirty(true);
        EXPECT_TRUE(logicNode.isDirty());
    }

    TEST_F(ALogicNodeImpl, TakesOwnershipOfGivenProperties)
    {
        auto inputType = MakeStruct("IN", { TypeData{"subProperty", EPropertyType::Int32} });
        auto outputType = MakeStruct("OUT", { TypeData{"subProperty", EPropertyType::Int32} });
        // These usually come from subclasses deserialization code
        auto inputs = std::make_unique<Property>(std::make_unique<PropertyImpl>(inputType, EPropertySemantics::ScriptInput));
        auto outputs = std::make_unique<Property>(std::make_unique<PropertyImpl>(outputType, EPropertySemantics::ScriptOutput));

        LogicNodeImplMock logicNode("");
        logicNode.setRootProperties(std::move(inputs), std::move(outputs));

        EXPECT_EQ(logicNode.getInputs()->getName(), "IN");
        EXPECT_EQ(&logicNode.getInputs()->m_impl->getLogicNode(), &logicNode);
        EXPECT_EQ(logicNode.getInputs()->getChild(0)->getName(), "subProperty");
        EXPECT_EQ(&logicNode.getInputs()->getChild(0)->m_impl->getLogicNode(), &logicNode);

        EXPECT_EQ(logicNode.getOutputs()->getName(), "OUT");
        EXPECT_EQ(&logicNode.getOutputs()->m_impl->getLogicNode(), &logicNode);
        EXPECT_EQ(logicNode.getOutputs()->getChild(0)->getName(), "subProperty");
        EXPECT_EQ(&logicNode.getOutputs()->getChild(0)->m_impl->getLogicNode(), &logicNode);
    }
}
