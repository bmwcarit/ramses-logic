//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "internals/LogicNodeConnector.h"
#include "internals/impl/PropertyImpl.h"
#include "internals/impl/LogicNodeImpl.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"

#include "LogicNodeDummy.h"



namespace rlogic::internal
{
    TEST(ALogicNodeConnector, ReturnsFalseForIsLinkedIfLogicNodesAreNotLinked)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        LogicNodeConnector lc;
        EXPECT_FALSE(lc.isLinked(source->m_impl));
        EXPECT_FALSE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, ReturnsTrueDuringLinkingIfLinkedSuccessfully)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto output = source->getOutputs()->getChild("output1");
        auto input  = target->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        EXPECT_TRUE(lc.link(*output->m_impl, *input->m_impl));
        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, ReturnsFalseDuringLinkinIfAlreadyLinked)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto output = source->getOutputs()->getChild("output1");
        auto input  = target->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        EXPECT_TRUE(lc.link(*output->m_impl, *input->m_impl));
        EXPECT_FALSE(lc.link(*output->m_impl, *input->m_impl));

        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, ReturnsFalseForIsLinkedAfterUnlink)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto output = source->getOutputs()->getChild("output1");
        auto input  = target->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        lc.link(*output->m_impl, *input->m_impl);

        lc.unlink(*input->m_impl);
        EXPECT_FALSE(lc.isLinked(source->m_impl));
        EXPECT_FALSE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, ReturnsTrueForIsLinkedIfStillALinkIsAvailableAfterUnlink)
    {
        auto source = LogicNodeDummy::Create("source");
        auto middle = LogicNodeDummy::Create("middle");
        auto target = LogicNodeDummy::Create("target");

        auto sourceOutput = source->getOutputs()->getChild("output1");
        auto middleInput  = middle->getInputs()->getChild("input1");
        auto middleOutput = middle->getOutputs()->getChild("output1");
        auto targetInput  = target->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        lc.link(*sourceOutput->m_impl, *middleInput->m_impl);
        lc.link(*middleOutput->m_impl, *targetInput->m_impl);

        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(middle->m_impl));
        EXPECT_TRUE(lc.isLinked(target->m_impl));

        lc.unlink(*middleInput->m_impl);

        EXPECT_FALSE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(middle->m_impl));
        EXPECT_TRUE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, ReturnsFalseForIsLinkedAfterUnlinkAll)
    {
        auto source = LogicNodeDummy::Create("source");
        auto middle = LogicNodeDummy::Create("middle");
        auto target = LogicNodeDummy::Create("target");

        auto sourceOutput = source->getOutputs()->getChild("output1");
        auto middleInput  = middle->getInputs()->getChild("input1");
        auto middleOutput = middle->getOutputs()->getChild("output1");
        auto targetInput  = target->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        lc.link(*sourceOutput->m_impl, *middleInput->m_impl);
        lc.link(*middleOutput->m_impl, *targetInput->m_impl);

        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(middle->m_impl));
        EXPECT_TRUE(lc.isLinked(target->m_impl));

        lc.unlinkAll(middle->m_impl);

        EXPECT_FALSE(lc.isLinked(source->m_impl));
        EXPECT_FALSE(lc.isLinked(middle->m_impl));
        EXPECT_FALSE(lc.isLinked(target->m_impl));
    }

    TEST(ALogicNodeConnector, DoesNotUnlinkUnrelatedLinks)
    {
        auto source = LogicNodeDummy::Create("source");
        auto middle = LogicNodeDummy::Create("middle");
        auto target1 = LogicNodeDummy::Create("target");
        auto target2 = LogicNodeDummy::Create("target2");

        auto sourceOutput = source->getOutputs()->getChild("output1");
        auto middleInput  = middle->getInputs()->getChild("input1");
        auto middleOutput = middle->getOutputs()->getChild("output1");
        auto target1Input1  = target1->getInputs()->getChild("input1");
        auto target1Input2  = target1->getInputs()->getChild("input2");
        auto target2Input1  = target2->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        lc.link(*sourceOutput->m_impl, *middleInput->m_impl);
        lc.link(*middleOutput->m_impl, *target1Input1->m_impl);
        lc.link(*middleOutput->m_impl, *target1Input2->m_impl);
        lc.link(*sourceOutput->m_impl, *target2Input1->m_impl);

        EXPECT_TRUE(lc.isLinked(target1->m_impl.get()));
        EXPECT_TRUE(lc.isLinked(target2->m_impl.get()));

        lc.unlinkAll(middle->m_impl.get());

        EXPECT_TRUE(lc.isLinked(source->m_impl.get()));
        EXPECT_FALSE(lc.isLinked(middle->m_impl.get()));
        EXPECT_FALSE(lc.isLinked(target1->m_impl.get()));
        EXPECT_TRUE(lc.isLinked(target2->m_impl.get()));
    }

    TEST(ALinkManager, DoesReturnSourcePropertyForTargetPropertyIfLinked)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto output = source->getOutputs()->getChild("output1");
        auto input  = target->getInputs()->getChild("input1");

        internal::LogicNodeConnector lm;
        lm.link(*output->m_impl, *input->m_impl);
        auto property = lm.getLinkedOutput(*input->m_impl);
        ASSERT_NE(nullptr, property);
        EXPECT_EQ(property, output->m_impl.get());
    }

    TEST(ALinkManager, DoesReturnTargetPropertyForSourcePropertyIfLinked)
    {
        auto source  = LogicNodeDummy::Create("source");
        auto middle  = LogicNodeDummy::Create("middle");
        auto target1 = LogicNodeDummy::Create("target");
        auto target2 = LogicNodeDummy::Create("target2");

        auto sourceOutput  = source->getOutputs()->getChild("output1");
        auto middleInput   = middle->getInputs()->getChild("input1");
        auto middleOutput  = middle->getOutputs()->getChild("output1");
        auto target1Input1 = target1->getInputs()->getChild("input1");
        auto target1Input2 = target1->getInputs()->getChild("input2");
        auto target2Input1 = target2->getInputs()->getChild("input1");

        LogicNodeConnector lc;
        lc.link(*sourceOutput->m_impl, *middleInput->m_impl);
        lc.link(*middleOutput->m_impl, *target1Input1->m_impl);
        lc.link(*middleOutput->m_impl, *target1Input2->m_impl);
        lc.link(*sourceOutput->m_impl, *target2Input1->m_impl);

        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_TRUE(lc.isLinked(middle->m_impl));
        EXPECT_TRUE(lc.isLinked(target1->m_impl));
        EXPECT_TRUE(lc.isLinked(target2->m_impl));

        lc.unlinkAll(middle->m_impl);

        EXPECT_TRUE(lc.isLinked(source->m_impl));
        EXPECT_FALSE(lc.isLinked(middle->m_impl));
        EXPECT_FALSE(lc.isLinked(target1->m_impl));
        EXPECT_TRUE(lc.isLinked(target2->m_impl));
    }

    TEST(ALogicNodeConnector, DoesReturnNullptrForSourcePropertyIfNotLinked)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto input  = target->getInputs()->getChild("input1");

        LogicNodeConnector lm;

        auto property = lm.getLinkedOutput(*input->m_impl);
        ASSERT_EQ(nullptr, property);
    }

    TEST(ALogicNodeConnector, DoesReturnNullptrForTargetPropertyIfNotLinked)
    {
        auto source = LogicNodeDummy::Create("source");
        auto target = LogicNodeDummy::Create("target");

        auto input = target->getInputs()->getChild("input1");

        internal::LogicNodeConnector lm;

        auto property = lm.getLinkedOutput(*input->m_impl);
        ASSERT_EQ(nullptr, property);
    }

    TEST(ALogicNodeConnector, ConnectsLogicNodesSoThatValuesArePropagated)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.inString1 = STRING
                IN.inString2 = STRING
                OUT.outString = STRING
            end
            function run()
                OUT.outString = IN.inString1 .. IN.inString2
            end
        )";

        auto script1 = logicEngine.createLuaScriptFromSource(scriptSource, "Script1");
        auto script2 = logicEngine.createLuaScriptFromSource(scriptSource, "Script2");
        auto script3 = logicEngine.createLuaScriptFromSource(scriptSource, "Script3");

        auto script1Input2 = script1->getInputs()->getChild("inString2");
        auto script2Input1 = script2->getInputs()->getChild("inString1");
        auto script2Input2 = script2->getInputs()->getChild("inString2");
        auto script3Input1 = script3->getInputs()->getChild("inString1");
        auto script3Input2 = script3->getInputs()->getChild("inString2");
        auto script1Ouput  = script1->getOutputs()->getChild("outString");
        auto script2Ouput  = script2->getOutputs()->getChild("outString");
        auto script3Ouput  = script3->getOutputs()->getChild("outString");

        logicEngine.link(*script1Ouput, *script2Input1);
        logicEngine.link(*script2Ouput, *script3Input1);

        script1Input2->set(std::string("Script1"));
        script2Input2->set(std::string("Script2"));
        script3Input2->set(std::string("Script3"));

        logicEngine.update();

        EXPECT_EQ("Script1Script2Script3", script3Ouput->get<std::string>());
    }

    TEST(ALogicNodeConnector, ReturnsTrueForIfLinkedForNestedProperties)
    {
        LogicEngine logicEngine;
        auto        scriptSource = R"(
            function interface()
                IN.input = {
                    inFloat = FLOAT
                }
                OUT.output = {
                    outFloat = FLOAT
                }
            end
            function run()
            end
        )";

        auto sourceScript = logicEngine.createLuaScriptFromSource(scriptSource, "SourceScript");
        auto middleScript = logicEngine.createLuaScriptFromSource(scriptSource, "MiddleScript");
        auto targetScript = logicEngine.createLuaScriptFromSource(scriptSource, "TargetScript");

        auto sourceOutputFloat = sourceScript->getOutputs()->getChild("output")->getChild("outFloat");
        auto middleInputFloat  = middleScript->getInputs()->getChild("input")->getChild("inFloat");
        auto middleOutputFloat = middleScript->getOutputs()->getChild("output")->getChild("outFloat");
        auto targetInputFloat  = targetScript->getInputs()->getChild("input")->getChild("inFloat");

        internal::LogicNodeConnector connector;

        connector.link(*sourceOutputFloat->m_impl, *middleInputFloat->m_impl);
        connector.link(*middleOutputFloat->m_impl, *targetInputFloat->m_impl);

        EXPECT_TRUE(connector.isLinked(sourceScript->m_impl));
        EXPECT_TRUE(connector.isLinked(middleScript->m_impl));
        EXPECT_TRUE(connector.isLinked(targetScript->m_impl));

        connector.unlink(*targetInputFloat->m_impl);

        EXPECT_TRUE(connector.isLinked(sourceScript->m_impl.get()));
        EXPECT_TRUE(connector.isLinked(middleScript->m_impl.get()));
        EXPECT_FALSE(connector.isLinked(targetScript->m_impl.get()));

        connector.unlink(*middleInputFloat->m_impl);

        EXPECT_FALSE(connector.isLinked(sourceScript->m_impl.get()));
        EXPECT_FALSE(connector.isLinked(middleScript->m_impl.get()));
        EXPECT_FALSE(connector.isLinked(targetScript->m_impl.get()));
    }
}
