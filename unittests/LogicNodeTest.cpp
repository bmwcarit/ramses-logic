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
    class DerivedLogicNode : public LogicNodeImpl
    {
    public:
        // Forwarding constructor
        DerivedLogicNode(std::string_view name, std::unique_ptr<PropertyImpl> inputs, std::unique_ptr<PropertyImpl> outputs)
            : LogicNodeImpl(name, std::move(inputs), std::move(outputs))
        {
        }

        MOCK_METHOD(std::optional<LogicNodeRuntimeError>, update, (), (override, final));
    };

    class ALogicNodeImpl : public ::testing::Test
    {
    protected:
        std::unique_ptr<PropertyImpl> m_testInputs {std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::ScriptInput)};
    };

    TEST_F(ALogicNodeImpl, RemembersNameGivenInConstructor)
    {
        const DerivedLogicNode logicNode("name", std::move(m_testInputs), nullptr);
        EXPECT_EQ(logicNode.getName(), "name");
    }

    TEST_F(ALogicNodeImpl, CanReceiveNewName)
    {
        DerivedLogicNode logicNode("name", std::move(m_testInputs), nullptr);
        logicNode.setName("newName");
        EXPECT_EQ(logicNode.getName(), "newName");
    }

    TEST_F(ALogicNodeImpl, DirtyByDefault)
    {
        const DerivedLogicNode logicNode("", std::move(m_testInputs), nullptr);
        EXPECT_TRUE(logicNode.isDirty());
    }

    TEST_F(ALogicNodeImpl, DirtyWhenSetDirty)
    {
        DerivedLogicNode logicNode("", std::move(m_testInputs), nullptr);
        logicNode.setDirty(false);
        EXPECT_FALSE(logicNode.isDirty());
        logicNode.setDirty(true);
        EXPECT_TRUE(logicNode.isDirty());
    }

    TEST_F(ALogicNodeImpl, TakesOwnershipOfGivenProperties)
    {
        // These usually come from subclasses deserialization code
        std::unique_ptr<PropertyImpl> inputs = std::make_unique<PropertyImpl>("IN", EPropertyType::Struct, EPropertySemantics::ScriptInput);
        inputs->addChild(std::make_unique<PropertyImpl>("subProperty", EPropertyType::Int32, EPropertySemantics::ScriptInput));
        std::unique_ptr<PropertyImpl> outputs = std::make_unique<PropertyImpl>("OUT", EPropertyType::Struct, EPropertySemantics::ScriptOutput);
        outputs->addChild(std::make_unique<PropertyImpl>("subProperty", EPropertyType::Int32, EPropertySemantics::ScriptOutput));

        DerivedLogicNode logicNode("", std::move(inputs), std::move(outputs));

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
