//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/Property.h"
#include "impl/AnimationNodeImpl.h"
#include "impl/DataArrayImpl.h"
#include "internals/ErrorReporting.h"
#include "generated/AnimationNodeGen.h"
#include "flatbuffers/flatbuffers.h"
#include <numeric>

namespace rlogic::internal
{
    class AnAnimationNode : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            m_dataFloat = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f });
            m_dataVec2 = m_logicEngine.createDataArray(std::vector<vec2f>{ { 1.f, 2.f }, { 3.f, 4.f }, { 5.f, 6.f } });
            // Quaternions which are not normalized (ie. not of unit length). Used for tests to check they are normalized correctly
            m_dataVec4 = m_logicEngine.createDataArray(std::vector<vec4f>{ { 2.f, 0.f, 0.f, 0.f }, { 0.f, 2.f, 0.f, 0.f } , { 0.f, 0.f, 2.f, 0.f } });
        }

    protected:
        template <typename T>
        void advanceAnimationAndExpectValues(AnimationNode& animNode, float timeDelta, const T& expectedValue)
        {
            EXPECT_TRUE(animNode.getInputs()->getChild("timeDelta")->set(timeDelta));
            EXPECT_TRUE(m_logicEngine.update());
            const auto val = *animNode.getOutputs()->getChild("channel")->get<T>();
            if constexpr (std::is_same_v<T, vec2f>)
            {
                EXPECT_FLOAT_EQ(expectedValue[0], val[0]);
                EXPECT_FLOAT_EQ(expectedValue[1], val[1]);
            }
            else if constexpr (std::is_same_v<T, vec2i>)
            {
                EXPECT_EQ(expectedValue[0], val[0]);
                EXPECT_EQ(expectedValue[1], val[1]);
            }
            else if constexpr (std::is_same_v<T, vec4f>)
            {
                EXPECT_FLOAT_EQ(expectedValue[0], val[0]);
                EXPECT_FLOAT_EQ(expectedValue[1], val[1]);
                EXPECT_FLOAT_EQ(expectedValue[2], val[2]);
                EXPECT_FLOAT_EQ(expectedValue[3], val[3]);
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                EXPECT_EQ(expectedValue, val);
            }
            else
            {
                ASSERT_TRUE(false) << "test missing for type";
            }
        }

        void advanceAnimationAndExpectValues_twoChannels(AnimationNode& animNode, float timeDelta, const vec2f& expectedValue1, const vec2f& expectedValue2)
        {
            animNode.getInputs()->getChild("timeDelta")->set(timeDelta);
            m_logicEngine.update();
            const auto val1 = *animNode.getOutputs()->getChild("channel1")->get<vec2f>();
            EXPECT_FLOAT_EQ(expectedValue1[0], val1[0]);
            EXPECT_FLOAT_EQ(expectedValue1[1], val1[1]);
            const auto val2 = *animNode.getOutputs()->getChild("channel2")->get<vec2f>();
            EXPECT_FLOAT_EQ(expectedValue2[0], val2[0]);
            EXPECT_FLOAT_EQ(expectedValue2[1], val2[1]);
        }

        LogicEngine m_logicEngine;
        DataArray* m_dataFloat = nullptr;
        DataArray* m_dataVec2 = nullptr;
        DataArray* m_dataVec4 = nullptr;
    };

    TEST_F(AnAnimationNode, IsCreated)
    {
        const AnimationChannel channel{ "channel", m_dataFloat, m_dataVec2 };
        const AnimationChannels channels{ channel, channel };
        const auto animNode = m_logicEngine.createAnimationNode(channels, "animNode");
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
        ASSERT_NE(nullptr, animNode);
        EXPECT_EQ(animNode, m_logicEngine.findAnimationNode("animNode"));

        EXPECT_EQ("animNode", animNode->getName());
        EXPECT_FLOAT_EQ(3.f, animNode->getDuration());
        EXPECT_EQ(channels, animNode->getChannels());
    }

    TEST_F(AnAnimationNode, IsDestroyed)
    {
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataVec2 } }, "animNode");
        EXPECT_TRUE(m_logicEngine.destroy(*animNode));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
        EXPECT_EQ(nullptr, m_logicEngine.findAnimationNode("animNode"));
    }

    TEST_F(AnAnimationNode, FailsToBeDestroyedIfFromOtherLogicInstance)
    {
        auto animNode = m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataVec2 } }, "animNode");

        LogicEngine otherEngine;
        EXPECT_FALSE(otherEngine.destroy(*animNode));
        ASSERT_FALSE(otherEngine.getErrors().empty());
        EXPECT_EQ("Can't find AnimationNode in logic engine!", otherEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, ChangesName)
    {
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataVec2 } }, "animNode");

        animNode->setName("an");
        EXPECT_EQ("an", animNode->getName());
        EXPECT_EQ(animNode, m_logicEngine.findAnimationNode("an"));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());
    }

    TEST_F(AnAnimationNode, CanContainVariousAnimationChannels)
    {
        const auto timeStamps1 = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f });
        const auto timeStamps2 = m_logicEngine.createDataArray(std::vector<float>{ 3.f, 4.f, 5.f });
        const auto data1 = m_logicEngine.createDataArray(std::vector<vec2f>{ { 11.f, 22.f }, { 33.f, 44.f } });
        const auto data2 = m_logicEngine.createDataArray(std::vector<vec2i>{ { 11, 22 }, { 44, 55 }, { 66, 77 } });

        const AnimationChannel channel1{ "channel1", timeStamps1, data1, EInterpolationType::Step };
        const AnimationChannel channel2{ "channel2", timeStamps1, data1, EInterpolationType::Linear };
        const AnimationChannel channel3{ "channel3", timeStamps2, data2, EInterpolationType::Linear };
        const AnimationChannel channel4{ "channel4", timeStamps1, data1, EInterpolationType::Cubic, data1, data1 };
        const AnimationChannels channels{ channel1, channel2, channel3, channel4 };

        const auto animNode = m_logicEngine.createAnimationNode(channels, "animNode");

        EXPECT_TRUE(m_logicEngine.getErrors().empty());
        ASSERT_NE(nullptr, animNode);
        EXPECT_EQ(animNode, m_logicEngine.findAnimationNode("animNode"));

        EXPECT_EQ("animNode", animNode->getName());
        EXPECT_FLOAT_EQ(5.f, animNode->getDuration());
        EXPECT_EQ(channels, animNode->getChannels());
    }

    TEST_F(AnAnimationNode, HasPropertiesMatchingChannels)
    {
        const AnimationChannel channel1{ "channel1", m_dataFloat, m_dataFloat };
        const AnimationChannel channel2{ "channel2", m_dataFloat, m_dataVec4, EInterpolationType::Linear_Quaternions };
        const auto animNode = m_logicEngine.createAnimationNode({ channel1, channel2 }, "animNode");

        const auto rootIn = animNode->getInputs();
        EXPECT_EQ("IN", rootIn->getName());
        ASSERT_EQ(4u, rootIn->getChildCount());
        EXPECT_EQ("timeDelta", rootIn->getChild(0u)->getName());
        EXPECT_EQ("play", rootIn->getChild(1u)->getName());
        EXPECT_EQ("loop", rootIn->getChild(2u)->getName());
        EXPECT_EQ("rewindOnStop", rootIn->getChild(3u)->getName());
        EXPECT_EQ(EPropertyType::Float, rootIn->getChild(0u)->getType());
        EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(1u)->getType());
        EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(2u)->getType());
        EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(3u)->getType());

        const auto rootOut = animNode->getOutputs();
        EXPECT_EQ("OUT", rootOut->getName());
        ASSERT_EQ(3u, rootOut->getChildCount());
        EXPECT_EQ("progress", rootOut->getChild(0u)->getName());
        EXPECT_EQ("channel1", rootOut->getChild(1u)->getName());
        EXPECT_EQ("channel2", rootOut->getChild(2u)->getName());
        EXPECT_EQ(EPropertyType::Float, rootOut->getChild(0u)->getType());
        EXPECT_EQ(EPropertyType::Float, rootOut->getChild(1u)->getType());
        EXPECT_EQ(EPropertyType::Vec4f, rootOut->getChild(2u)->getType());
    }

    TEST_F(AnAnimationNode, DeterminesDurationFromHighestTimestamp)
    {
        const auto timeStamps1 = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f });
        const auto timeStamps2 = m_logicEngine.createDataArray(std::vector<float>{ 4.f, 5.f, 6.f });

        const auto animNode1 = m_logicEngine.createAnimationNode({ { "channel", timeStamps1, m_dataVec2 } }, "animNode1");
        EXPECT_FLOAT_EQ(3.f, animNode1->getDuration());
        const auto animNode2 = m_logicEngine.createAnimationNode({ { "channel1", timeStamps1, m_dataVec2 }, { "channel2", timeStamps2, m_dataVec2 } }, "animNode2");
        EXPECT_FLOAT_EQ(6.f, animNode2->getDuration());
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfMissingTimestampsOrKeyframes)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2 };

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({}, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': must provide at least one channel.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", nullptr, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': every channel must provide timestamps and keyframes data.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, nullptr }, validChannel }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': every channel must provide timestamps and keyframes data.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfTimestampsOrKeyframesTypeInvalid)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2 };
        const auto dataVec2OtherSize = m_logicEngine.createDataArray(std::vector<vec2f>{ { 1.f, 2.f } }); // single element only

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataVec2, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': all channel timestamps must be float type.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, dataVec2OtherSize } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': number of keyframes must be same as number of timestamps.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfTimestampsNotStrictlyAscending)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2 };

        const auto timeStampsDescending = m_logicEngine.createDataArray(std::vector<float>{ { 1.f, 3.f, 2.f } });
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", timeStampsDescending, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': timestamps have to be strictly in ascending order.", m_logicEngine.getErrors().front().message);

        const auto timeStampsNotStrictAscend = m_logicEngine.createDataArray(std::vector<float>{ { 1.f, 2.f, 2.f } });
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", timeStampsNotStrictAscend, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': timestamps have to be strictly in ascending order.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfTangentsProvidedForNonCubicInterpolation)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2 };

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Linear, m_dataVec2, nullptr } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents were provided for other than cubic interpolation type.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Linear, nullptr, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents were provided for other than cubic interpolation type.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfQuaternionInterpolationWithNonVec4fKeyframes)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2 };

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Linear_Quaternions } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': quaternion animation requires the channel keyframes to be of type vec4f.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic_Quaternions, m_dataVec2, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': quaternion animation requires the channel keyframes to be of type vec4f.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic_Quaternions, m_dataVec2, m_dataVec2 }, validChannel }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': quaternion animation requires the channel keyframes to be of type vec4f.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfInputRequirementsNotMet_specificToCubicInerpolation)
    {
        const AnimationChannel validChannel{ "ok", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, m_dataVec2, m_dataVec2 };
        EXPECT_NE(nullptr, m_logicEngine.createAnimationNode({ validChannel }, "animNode"));

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, m_dataVec2, nullptr } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': cubic interpolation requires tangents to be provided.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, nullptr, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': cubic interpolation requires tangents to be provided.", m_logicEngine.getErrors().front().message);

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, m_dataVec2, m_dataFloat } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents must be of same data type as keyframes.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, m_dataFloat, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents must be of same data type as keyframes.", m_logicEngine.getErrors().front().message);

        const auto dataVec2OtherSize = m_logicEngine.createDataArray(std::vector<vec2f>{ { 1.f, 2.f } }); // single element only
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, m_dataVec2, dataVec2OtherSize } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': number of tangents in/out must be same as number of keyframes.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ validChannel, { "channel", m_dataFloat, m_dataVec2, EInterpolationType::Cubic, dataVec2OtherSize, m_dataVec2 } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': number of tangents in/out must be same as number of keyframes.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, FailsToBeCreatedIfDataArrayFromOtherLogicInstance)
    {
        LogicEngine otherInstance;
        auto otherInstanceData = otherInstance.createDataArray(std::vector<float>{ 1.f, 2.f, 3.f });

        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", otherInstanceData, m_dataFloat } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': timestamps or keyframes were not found in this logic instance.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, otherInstanceData } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': timestamps or keyframes were not found in this logic instance.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataFloat, EInterpolationType::Cubic, otherInstanceData, m_dataFloat } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents were not found in this logic instance.", m_logicEngine.getErrors().front().message);
        EXPECT_EQ(nullptr, m_logicEngine.createAnimationNode({ { "channel", m_dataFloat, m_dataFloat, EInterpolationType::Cubic, m_dataFloat, otherInstanceData } }, "animNode"));
        EXPECT_EQ("Failed to create AnimationNode 'animNode': tangents were not found in this logic instance.", m_logicEngine.getErrors().front().message);
    }

    TEST_F(AnAnimationNode, CanBeSerializedAndDeserialized)
    {
        {
            LogicEngine otherEngine;

            const auto timeStamps1 = otherEngine.createDataArray(std::vector<float>{ 1.f, 2.f }, "ts1");
            const auto timeStamps2 = otherEngine.createDataArray(std::vector<float>{ 3.f, 4.f, 5.f }, "ts2");
            const auto data1 = otherEngine.createDataArray(std::vector<vec2i>{ { 11, 22 }, { 33, 44 } }, "data1");
            const auto data2 = otherEngine.createDataArray(std::vector<vec2i>{ { 11, 22 }, { 44, 55 }, { 66, 77 } }, "data2");

            const AnimationChannel channel1{ "channel1", timeStamps1, data1, EInterpolationType::Step };
            const AnimationChannel channel2{ "channel2", timeStamps1, data1, EInterpolationType::Linear };
            const AnimationChannel channel3{ "channel3", timeStamps2, data2, EInterpolationType::Linear };
            const AnimationChannel channel4{ "channel4", timeStamps1, data1, EInterpolationType::Cubic, data1, data1 };

            otherEngine.createAnimationNode({ channel1, channel2, channel3, channel4 }, "animNode1");
            otherEngine.createAnimationNode({ channel4, channel3, channel2, channel1 }, "animNode2");

            ASSERT_TRUE(otherEngine.saveToFile("logic_animNodes.bin"));
        }

        ASSERT_TRUE(m_logicEngine.loadFromFile("logic_animNodes.bin"));
        EXPECT_TRUE(m_logicEngine.getErrors().empty());

        EXPECT_EQ(2u, m_logicEngine.animationNodes().size());
        const auto animNode1 = m_logicEngine.findAnimationNode("animNode1");
        const auto animNode2 = m_logicEngine.findAnimationNode("animNode2");
        ASSERT_TRUE(animNode1 && animNode2);

        EXPECT_EQ("animNode1", animNode1->getName());
        EXPECT_EQ("animNode2", animNode2->getName());
        EXPECT_FLOAT_EQ(5.f, animNode1->getDuration());
        EXPECT_FLOAT_EQ(5.f, animNode2->getDuration());

        // ptrs are different after loading, find data arrays to verify the references in loaded anim nodes match
        const auto ts1 = m_logicEngine.findDataArray("ts1");
        const auto ts2 = m_logicEngine.findDataArray("ts2");
        const auto data1 = m_logicEngine.findDataArray("data1");
        const auto data2 = m_logicEngine.findDataArray("data2");
        const AnimationChannel channel1{ "channel1", ts1, data1, EInterpolationType::Step };
        const AnimationChannel channel2{ "channel2", ts1, data1, EInterpolationType::Linear };
        const AnimationChannel channel3{ "channel3", ts2, data2, EInterpolationType::Linear };
        const AnimationChannel channel4{ "channel4", ts1, data1, EInterpolationType::Cubic, data1, data1 };
        const AnimationChannels expectedChannels1{ channel1, channel2, channel3, channel4 };
        const AnimationChannels expectedChannels2{ channel4, channel3, channel2, channel1 };

        EXPECT_EQ(expectedChannels1, animNode1->getChannels());
        EXPECT_EQ(expectedChannels2, animNode2->getChannels());

        // verify properties after loading
        // check properties same for both anim nodes
        for (const auto animNode : { animNode1, animNode2 })
        {
            const auto rootIn = animNode->getInputs();
            EXPECT_EQ("IN", rootIn->getName());
            ASSERT_EQ(4u, rootIn->getChildCount());
            EXPECT_EQ("timeDelta", rootIn->getChild(0u)->getName());
            EXPECT_EQ("play", rootIn->getChild(1u)->getName());
            EXPECT_EQ("loop", rootIn->getChild(2u)->getName());
            EXPECT_EQ("rewindOnStop", rootIn->getChild(3u)->getName());
            EXPECT_EQ(EPropertyType::Float, rootIn->getChild(0u)->getType());
            EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(1u)->getType());
            EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(2u)->getType());
            EXPECT_EQ(EPropertyType::Bool, rootIn->getChild(3u)->getType());

            const auto rootOut = animNode->getOutputs();
            EXPECT_EQ("OUT", rootOut->getName());
            ASSERT_EQ(5u, rootOut->getChildCount());
            EXPECT_EQ("progress", rootOut->getChild(0u)->getName());
            EXPECT_EQ(EPropertyType::Float, rootOut->getChild(0u)->getType());
            EXPECT_EQ(EPropertyType::Vec2i, rootOut->getChild(1u)->getType());
            EXPECT_EQ(EPropertyType::Vec2i, rootOut->getChild(2u)->getType());
            EXPECT_EQ(EPropertyType::Vec2i, rootOut->getChild(3u)->getType());
            EXPECT_EQ(EPropertyType::Vec2i, rootOut->getChild(4u)->getType());
        }
        // check output names separately
        const auto rootOut1 = animNode1->getOutputs();
        EXPECT_EQ("channel1", rootOut1->getChild(1u)->getName());
        EXPECT_EQ("channel2", rootOut1->getChild(2u)->getName());
        EXPECT_EQ("channel3", rootOut1->getChild(3u)->getName());
        EXPECT_EQ("channel4", rootOut1->getChild(4u)->getName());
        const auto rootOut2 = animNode2->getOutputs();
        EXPECT_EQ("channel4", rootOut2->getChild(1u)->getName());
        EXPECT_EQ("channel3", rootOut2->getChild(2u)->getName());
        EXPECT_EQ("channel2", rootOut2->getChild(3u)->getName());
        EXPECT_EQ("channel1", rootOut2->getChild(4u)->getName());
    }

    TEST_F(AnAnimationNode, WillSerializeAnimationInputStatesButNotProgress)
    {
        {
            LogicEngine otherEngine;

            const auto timeStamps = otherEngine.createDataArray(std::vector<float>{ 1.f, 2.f }, "ts");
            const auto data = otherEngine.createDataArray(std::vector<int32_t>{ 10, 20 }, "data");
            const AnimationChannel channel{ "channel", timeStamps, data, EInterpolationType::Linear };
            const auto animNode = otherEngine.createAnimationNode({ channel }, "animNode");

            EXPECT_TRUE(animNode->getInputs()->getChild("play")->set(true));
            EXPECT_TRUE(animNode->getInputs()->getChild("loop")->set(true));
            EXPECT_TRUE(animNode->getInputs()->getChild("rewindOnStop")->set(true));
            EXPECT_TRUE(animNode->getInputs()->getChild("timeDelta")->set(3.5f));
            EXPECT_TRUE(otherEngine.update());
            EXPECT_EQ(15, *animNode->getOutputs()->getChild("channel")->get<int32_t>());
            EXPECT_FLOAT_EQ(0.75f, *animNode->getOutputs()->getChild("progress")->get<float>());

            ASSERT_TRUE(otherEngine.saveToFile("logic_animNodes.bin"));
        }

        ASSERT_TRUE(m_logicEngine.loadFromFile("logic_animNodes.bin"));
        const auto animNode = m_logicEngine.findAnimationNode("animNode");
        ASSERT_TRUE(animNode);

        // update node with zero timeDelta to check its state
        EXPECT_TRUE(animNode->getInputs()->getChild("timeDelta")->set(0.f));
        EXPECT_TRUE(m_logicEngine.update());

        // kept input states but not progress
        EXPECT_TRUE(*animNode->getInputs()->getChild("play")->get<bool>());
        EXPECT_TRUE(*animNode->getInputs()->getChild("loop")->get<bool>());
        EXPECT_TRUE(*animNode->getInputs()->getChild("rewindOnStop")->get<bool>());
        EXPECT_EQ(10, *animNode->getOutputs()->getChild("channel")->get<int32_t>());
        EXPECT_FLOAT_EQ(0.f, *animNode->getOutputs()->getChild("progress")->get<float>());

        // can play again
        advanceAnimationAndExpectValues<int32_t>(*animNode, 1.5f, 15);
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_step_vec2f)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 10.f }, { 1.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Step } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.f, { 0.f, 10.f });
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.99f, { 0.f, 10.f }); // still no change
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.0100001f, { 1.f, 20.f }); // step to next keyframe value at its timestamp
        advanceAnimationAndExpectValues<vec2f>(*animNode, 100.f, { 1.f, 20.f }); // no change pass end of animation
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_step_vec2i)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2i>{ { 0, 10 }, { 1, 20 } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Step } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.f, { 0, 10 });
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.99f, { 0, 10 }); // still no change
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.0100001f, { 1, 20 }); // step to next keyframe value at its timestamp
        advanceAnimationAndExpectValues<vec2i>(*animNode, 100.f, { 1, 20 }); // no change pass end of animation
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_linear_vec2f)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 10.f }, { 1.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.f, { 0.f, 10.f });
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.1f, { 0.1f, 11.f }); // time 0.1
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.4f, { 0.5f, 15.f }); // time 0.5
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.4f, { 0.9f, 19.f }); // time 0.9
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.1f, { 1.f, 20.f }); // time 1.0
        advanceAnimationAndExpectValues<vec2f>(*animNode, 100.f, { 1.f, 20.f }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_linear_vec2i)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2i>{ { 0, 10 }, { 1, 20 } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.f, { 0, 10 });
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.1f, { 0, 11 }); // time 0.1
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.4f, { 1, 15 }); // time 0.5
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.4f, { 1, 19 }); // time 0.9
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.1f, { 1, 20 }); // time 1.0
        advanceAnimationAndExpectValues<vec2i>(*animNode, 100.f, { 1, 20 }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_linear_quaternions)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f, 2.f });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, m_dataVec4, EInterpolationType::Linear_Quaternions } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.f, { 1, 0, 0, 0 });
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.5f, { .70710677f, .70710677f, 0, 0 }); // time 0.5
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.5f, { 0, 1, 0, 0 }); // time 1.0
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.5f, { 0, .70710677f, .70710677f, 0 }); // time 1.5
        advanceAnimationAndExpectValues<vec4f>(*animNode, 100.f, { 0, 0, 1, 0 }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_cubic_vec2f)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 10.f }, { 1.f, 20.f } });
        const auto tangentsZero = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 0.f }, { 0.f, 0.f } });
        const auto tangentsIn = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 0.f }, { -1.f, -2.f } });
        const auto tangentsOut = m_logicEngine.createDataArray(std::vector<vec2f>{ { 2.f, 5.f }, { 0.f, 0.f } });
        // animation with one channel using zero tangents and another channel with non-zero tangents
        const auto animNode = m_logicEngine.createAnimationNode({
            { "channel1", timeStamps, data, EInterpolationType::Cubic, tangentsZero, tangentsZero },
            { "channel2", timeStamps, data, EInterpolationType::Cubic, tangentsIn, tangentsOut },
            });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues_twoChannels(*animNode, 0.f, { 0.f, 10.f }, { 0.f, 10.f });
        advanceAnimationAndExpectValues_twoChannels(*animNode, 0.1f, { 0.028f, 10.28f }, { 0.199f, 10.703f }); // time 0.1
        advanceAnimationAndExpectValues_twoChannels(*animNode, 0.4f, { 0.5f, 15.f }, { 0.875f, 15.875f }); // time 0.5
        advanceAnimationAndExpectValues_twoChannels(*animNode, 0.4f, { 0.972f, 19.72f }, { 1.071f, 19.927f }); // time 0.9
        advanceAnimationAndExpectValues_twoChannels(*animNode, 0.1f, { 1.f, 20.f }, { 1.f, 20.f }); // time 1.0
        advanceAnimationAndExpectValues_twoChannels(*animNode, 100.f, { 1.f, 20.f }, { 1.f, 20.f }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_cubic_quaternions)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f, 2.f });
        const auto tangentsZero = m_logicEngine.createDataArray(std::vector<vec4f>{ { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } });
        const auto animNode = m_logicEngine.createAnimationNode({{ "channel", timeStamps, m_dataVec4, EInterpolationType::Cubic_Quaternions, tangentsZero, tangentsZero }});

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.f, { 1, 0, 0, 0 });
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { .98328203f, .18208927f, 0, 0 }); // time 0.25
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { .70710677f, .70710677f, 0, 0 }); // time 0.5
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { .18208927f, .98328203f, 0, 0 }); // time 0.75
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { 0, 1, 0, 0 }); // time 1.0
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.75f, { 0, .18208927f, .98328203f, 0 }); // time 1.75
        advanceAnimationAndExpectValues<vec4f>(*animNode, 100.f, { 0, 0, 1, 0 }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_cubic_quaternions_withTangents)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f, 2.f });
        const auto tangentsIn = m_logicEngine.createDataArray(std::vector<vec4f>{ { 0.f, 0.f, 0.f, 0.f }, { 1.f, 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f, 0.f } });
        const auto tangentsOut = m_logicEngine.createDataArray(std::vector<vec4f>{ { 1.f, 1.f, 0.f, 0.f}, {  1.f, 1.f, 0.f, 0.f }, { 0.f, 0.f, 0.f, 0.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, m_dataVec4, EInterpolationType::Cubic_Quaternions, tangentsIn, tangentsOut } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.f, { 1, 0, 0, 0 });
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { 0.9749645f, 0.22236033f, 0, 0 }); // time 0.25
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { .70710677f, .70710677f, 0, 0 }); // time 0.5
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { 0.13598002f, 0.99071163f, 0, 0 }); // time 0.75
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.25f, { 0, 1, 0, 0 }); // time 1.0
        advanceAnimationAndExpectValues<vec4f>(*animNode, 0.75f, { -0.055011157f, 0.12835936f, 0.99020082f, 0 }); // time 1.75
        advanceAnimationAndExpectValues<vec4f>(*animNode, 100.f, { 0, 0, 1, 0 }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatesKeyframeValues_cubic_vec2i)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2i>{ { 0, 10 }, { 1, 20 } });
        const auto tangentsIn = m_logicEngine.createDataArray(std::vector<vec2i>{ { 0, 0 }, { -1, -2 } });
        const auto tangentsOut = m_logicEngine.createDataArray(std::vector<vec2i>{ { 2, 5 }, { 0, 0 } });
        const auto animNode = m_logicEngine.createAnimationNode({
            { "channel", timeStamps, data, EInterpolationType::Cubic, tangentsIn, tangentsOut },
            });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.f, { 0, 10 });
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.1f, { 0, 11 }); // time 0.1
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.4f, { 1, 16 }); // time 0.5
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.4f, { 1, 20 }); // time 0.9
        advanceAnimationAndExpectValues<vec2i>(*animNode, 0.1f, { 1, 20 }); // time 1.0
        advanceAnimationAndExpectValues<vec2i>(*animNode, 100.f, { 1, 20 }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, InterpolatedValueBeforeFirstTimestampIsFirstKeyframe)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 1.f, 2.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2f>{ { 1.f, 20.f }, { 2.f, 30.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.f, { 1.f, 20.f });
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.5f, { 1.f, 20.f }); // time 0.5
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.5f, { 1.f, 20.f }); // time 1.0
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.5f, { 1.5f, 25.f }); // time 1.5
        advanceAnimationAndExpectValues<vec2f>(*animNode, 100.f, { 2.f, 30.f }); // stays at last keyframe after animation end
    }

    TEST_F(AnAnimationNode, CanPauseAndResumePlayViaProperty)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<vec2f>{ { 0.f, 10.f }, { 1.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.f, { 0.f, 10.f });
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.2f, { 0.2f, 12.f }); // anim time 0.2

        animNode->getInputs()->getChild("play")->set(false);
        advanceAnimationAndExpectValues<vec2f>(*animNode, 100.f, { 0.2f, 12.f }); // no change

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.2f, { 0.4f, 14.f }); // anim time 0.4
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.2f, { 0.6f, 16.f }); // anim time 0.6

        animNode->getInputs()->getChild("play")->set(false);
        advanceAnimationAndExpectValues<vec2f>(*animNode, 100.f, { 0.6f, 16.f }); // no change

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.2f, { 0.8f, 18.f }); // anim time 0.8
        advanceAnimationAndExpectValues<vec2f>(*animNode, 0.2f, { 1.f, 20.f }); // anim time 1.0
    }

    TEST_F(AnAnimationNode, WillNotUpdateIfTimeDeltaNegative)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 10.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);

        EXPECT_TRUE(animNode->getInputs()->getChild("timeDelta")->set(-0.4f));
        EXPECT_FALSE(m_logicEngine.update());
        // no change, invalid timeDelta was ignored
        EXPECT_FLOAT_EQ(14.f, *animNode->getOutputs()->getChild("channel")->get<float>());

        advanceAnimationAndExpectValues(*animNode, 0.4f, 18.f);
    }

    TEST_F(AnAnimationNode, CanPlayLoopingAnimation)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 10.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);
        animNode->getInputs()->getChild("loop")->set(true);

        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 18.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 12.f); // crossed end and restarted
        advanceAnimationAndExpectValues(*animNode, 0.4f, 16.f);
        advanceAnimationAndExpectValues(*animNode, 0.39f, 19.9f);
        advanceAnimationAndExpectValues(*animNode, 0.02f, 10.1f); // crossed end and restarted

        animNode->getInputs()->getChild("loop")->set(false);
        advanceAnimationAndExpectValues(*animNode, 100.f, 20.f); // crossed end and stays at last keyframe
    }

    TEST_F(AnAnimationNode, CanStartLoopingEvenAfterAnimationFinished)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 10.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);

        advanceAnimationAndExpectValues(*animNode, 100.f, 20.f); // crossed end and animation finished

        animNode->getInputs()->getChild("loop")->set(true); // will restart animation
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 18.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 12.f); // crossed end and restarted
        advanceAnimationAndExpectValues(*animNode, 0.4f, 16.f);
    }

    TEST_F(AnAnimationNode, WillRewindAnimationOnStop)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 10.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);
        animNode->getInputs()->getChild("rewindOnStop")->set(true);

        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);

        animNode->getInputs()->getChild("play")->set(false); // will rewind
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.5f, 10.f);

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f); // started from beginning
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 18.f);
        advanceAnimationAndExpectValues(*animNode, 100.f, 20.f);

        animNode->getInputs()->getChild("play")->set(false); // will rewind
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.5f, 10.f);

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f); // started from beginning
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);
    }

    TEST_F(AnAnimationNode, WillRewindAnimationWhenRewindEnabledEvenAfterAnimationFinishedAndStopped)
    {
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 10.f, 20.f } });
        const auto animNode = m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues(*animNode, 100.f, 20.f);
        animNode->getInputs()->getChild("play")->set(false); // animation finished and stopped

        animNode->getInputs()->getChild("rewindOnStop")->set(true); // will rewind
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f);
        advanceAnimationAndExpectValues(*animNode, 0.5f, 10.f);

        animNode->getInputs()->getChild("play")->set(true);
        advanceAnimationAndExpectValues(*animNode, 0.f, 10.f); // started from beginning
        advanceAnimationAndExpectValues(*animNode, 0.4f, 14.f);
        advanceAnimationAndExpectValues(*animNode, 0.4f, 18.f);
    }

    TEST_F(AnAnimationNode, GivesStableResultsWithExtremelySmallTimeDelta)
    {
        constexpr float Eps = std::numeric_limits<float>::epsilon();
        const auto timeStamps = m_logicEngine.createDataArray(std::vector<float>{ Eps * 100, Eps * 200 });
        const auto data = m_logicEngine.createDataArray(std::vector<float>{ { 1.f,  2.f } });
        auto& animNode = *m_logicEngine.createAnimationNode({ { "channel", timeStamps, data, EInterpolationType::Linear } });

        animNode.getInputs()->getChild("play")->set(true);
        // initialize output value by updating with zero timedelta and expect first keyframe value
        advanceAnimationAndExpectValues(animNode, 0.f, 1.f);

        float lastValue = 0.f;
        for (int i = 0; i < 500; ++i)
        {
            // advance with epsilon steps
            animNode.getInputs()->getChild("timeDelta")->set(Eps);
            m_logicEngine.update();
            const auto val = *animNode.getOutputs()->getChild("channel")->get<float>();

            // expect interpolated value to go through keyframes in stable manner
            EXPECT_TRUE(val >= lastValue);
            lastValue = val;
        }
        // expect last keyframe value
        EXPECT_FLOAT_EQ(2.f, lastValue);
    }

    class AnAnimationNode_SerializationLifecycle : public AnAnimationNode
    {
    protected:
        enum class ESerializationIssue
        {
            AllValid,
            NameMissing,
            ChannelsMissing,
            RootInMissing,
            RootOutMissing,
            ChannelNameMissing,
            ChannelTimestampsMissing,
            ChannelKeyframesMissing,
            ChannelTangentsInMissing,
            ChannelTangentsOutMissing,
            InvalidInterpolationType,
            PropertyInMissing,
            PropertyOutMissing,
            PropertyInWrongName,
            PropertyOutWrongName
        };

        std::unique_ptr<AnimationNodeImpl> deserializeSerializedDataWithIssue(ESerializationIssue issue)
        {
            flatbuffers::FlatBufferBuilder flatBufferBuilder;
            SerializationMap serializationMap;
            DeserializationMap deserializationMap;

            {
                const auto data = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f });

                HierarchicalTypeData inputs = MakeStruct("IN", {});
                if (issue == ESerializationIssue::PropertyInWrongName)
                {
                    inputs.children.push_back(MakeType("wrongInput", EPropertyType::Float));
                }
                else
                {
                    inputs.children.push_back(MakeType("timeDelta", EPropertyType::Float));
                }
                if (issue != ESerializationIssue::PropertyInMissing)
                    inputs.children.push_back(MakeType("play", EPropertyType::Bool));
                inputs.children.push_back(MakeType("loop", EPropertyType::Bool));
                inputs.children.push_back(MakeType("rewindOnStop", EPropertyType::Bool));
                auto inputsImpl = std::make_unique<PropertyImpl>(std::move(inputs), EPropertySemantics::AnimationInput);

                HierarchicalTypeData outputs = MakeStruct("OUT", {});
                if (issue == ESerializationIssue::PropertyOutWrongName)
                {
                    outputs.children.push_back(MakeType("wrongOutput", EPropertyType::Float));
                }
                else
                {
                    outputs.children.push_back(MakeType("progress", EPropertyType::Float));
                }
                if (issue != ESerializationIssue::PropertyOutMissing)
                    outputs.children.push_back(MakeType("channel", EPropertyType::Float));
                auto outputsImpl = std::make_unique<PropertyImpl>(std::move(outputs), EPropertySemantics::AnimationOutput);

                const auto dataFb = DataArrayImpl::Serialize(data->m_impl, flatBufferBuilder);
                flatBufferBuilder.Finish(dataFb);
                const auto dataFbSerialized = flatbuffers::GetRoot<rlogic_serialization::DataArray>(flatBufferBuilder.GetBufferPointer());
                deserializationMap.storeDataArray(*dataFbSerialized, *data);

                std::vector<flatbuffers::Offset<rlogic_serialization::Channel>> channelsFB;
                channelsFB.push_back(rlogic_serialization::CreateChannel(
                    flatBufferBuilder,
                    issue == ESerializationIssue::ChannelNameMissing ? 0 : flatBufferBuilder.CreateString("channel"),
                    issue == ESerializationIssue::ChannelTimestampsMissing ? 0 : dataFb,
                    issue == ESerializationIssue::ChannelKeyframesMissing ? 0 : dataFb,
                    issue == ESerializationIssue::InvalidInterpolationType ? static_cast<rlogic_serialization::EInterpolationType>(10) : rlogic_serialization::EInterpolationType::Cubic,
                    issue == ESerializationIssue::ChannelTangentsInMissing ? 0 : dataFb,
                    issue == ESerializationIssue::ChannelTangentsOutMissing ? 0 : dataFb
                ));

                const auto animNodeFB = rlogic_serialization::CreateAnimationNode(
                    flatBufferBuilder,
                    issue == ESerializationIssue::NameMissing ? 0 : flatBufferBuilder.CreateString("animNode"),
                    issue == ESerializationIssue::ChannelsMissing ? 0 : flatBufferBuilder.CreateVector(channelsFB),
                    issue == ESerializationIssue::RootInMissing ? 0 : PropertyImpl::Serialize(*inputsImpl, flatBufferBuilder, serializationMap),
                    issue == ESerializationIssue::RootOutMissing ? 0 : PropertyImpl::Serialize(*outputsImpl, flatBufferBuilder, serializationMap)
                );

                flatBufferBuilder.Finish(animNodeFB);
            }

            const auto& serialized = *flatbuffers::GetRoot<rlogic_serialization::AnimationNode>(flatBufferBuilder.GetBufferPointer());
            return AnimationNodeImpl::Deserialize(serialized, m_errorReporting, deserializationMap);
        }

        ErrorReporting m_errorReporting;
    };

    TEST_F(AnAnimationNode_SerializationLifecycle, FailsDeserializationIfEssentialDataMissing)
    {
        EXPECT_TRUE(deserializeSerializedDataWithIssue(AnAnimationNode_SerializationLifecycle::ESerializationIssue::AllValid));
        EXPECT_TRUE(m_errorReporting.getErrors().empty());

        for (const auto issue : { ESerializationIssue::NameMissing, ESerializationIssue::ChannelsMissing, ESerializationIssue::RootInMissing, ESerializationIssue::RootOutMissing })
        {
            EXPECT_FALSE(deserializeSerializedDataWithIssue(issue));
            ASSERT_FALSE(m_errorReporting.getErrors().empty());
            EXPECT_EQ("Fatal error during loading of AnimationNode from serialized data: missing name, channels or in/out property data!", m_errorReporting.getErrors().front().message);
            m_errorReporting.clear();
        }
    }

    TEST_F(AnAnimationNode_SerializationLifecycle, FailsDeserializationIfChannelDataMissing)
    {
        for (const auto issue : { ESerializationIssue::ChannelTimestampsMissing, ESerializationIssue::ChannelKeyframesMissing })
        {
            EXPECT_FALSE(deserializeSerializedDataWithIssue(issue));
            ASSERT_FALSE(m_errorReporting.getErrors().empty());
            EXPECT_EQ("Fatal error during loading of AnimationNode 'animNode' channel data: missing name, timestamps or keyframes!", m_errorReporting.getErrors().front().message);
            m_errorReporting.clear();
        }
    }

    TEST_F(AnAnimationNode_SerializationLifecycle, FailsDeserializationIfTangentsMissing)
    {
        for (const auto issue : { ESerializationIssue::ChannelTangentsInMissing, ESerializationIssue::ChannelTangentsOutMissing })
        {
            EXPECT_FALSE(deserializeSerializedDataWithIssue(issue));
            ASSERT_FALSE(m_errorReporting.getErrors().empty());
            EXPECT_EQ("Fatal error during loading of AnimationNode 'animNode' channel 'channel' data: missing tangents!", m_errorReporting.getErrors().front().message);
            m_errorReporting.clear();
        }
    }

    TEST_F(AnAnimationNode_SerializationLifecycle, FailsDeserializationIfInvalidInterpolationType)
    {
        EXPECT_FALSE(deserializeSerializedDataWithIssue(ESerializationIssue::InvalidInterpolationType));
        ASSERT_FALSE(m_errorReporting.getErrors().empty());
        EXPECT_EQ("Fatal error during loading of AnimationNode 'animNode' channel 'channel' data: missing or invalid interpolation type!", m_errorReporting.getErrors().front().message);
        m_errorReporting.clear();

        for (const auto issue : { ESerializationIssue::PropertyInMissing, ESerializationIssue::PropertyOutMissing, ESerializationIssue::PropertyInWrongName, ESerializationIssue::PropertyOutWrongName })
        {
            EXPECT_FALSE(deserializeSerializedDataWithIssue(issue));
            ASSERT_FALSE(m_errorReporting.getErrors().empty());
            EXPECT_EQ("Fatal error during loading of AnimationNode 'animNode': missing or invalid properties!", m_errorReporting.getErrors().front().message);
            m_errorReporting.clear();
        }
    }
}
