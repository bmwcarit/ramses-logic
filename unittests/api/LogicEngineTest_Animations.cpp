//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-framework-api/RamsesFramework.h"
#include "ramses-client-api/RamsesClient.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/Node.h"
#include <thread>

namespace rlogic::internal
{
    class ALogicEngine_Animations : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            const auto dataArray = m_logicEngine.createDataArray(std::vector<float>{ 0.f, 1.f }, "dataarray");
            const AnimationChannel channel{ "channel", dataArray, dataArray, rlogic::EInterpolationType::Linear };
            m_animation1 = m_logicEngine.createAnimationNode({ channel }, "animNode1");
            m_animation2 = m_logicEngine.createAnimationNode({ channel }, "animNode2");
            m_animation3 = m_logicEngine.createAnimationNode({ channel }, "animNode3");
            m_timer = m_logicEngine.createTimerNode();
        }

    protected:
        void advanceAnimationsAndUpdate(float timeDelta)
        {
            m_animation1->getInputs()->getChild("timeDelta")->set(timeDelta);
            m_animation2->getInputs()->getChild("timeDelta")->set(timeDelta);
            m_animation3->getInputs()->getChild("timeDelta")->set(timeDelta);
            m_logicEngine.update();
        }

        void setTickerAndUpdate(int64_t tick)
        {
            m_timer->getInputs()->getChild("ticker_us")->set(tick);
            m_logicEngine.update();
        }

        void expectNodeValues(float expectedTranslate, float expectedRotate, float expectedScale)
        {
            // we lose more than just single float epsilon precision in timer+animation logic
            // but this error is still negligible considering input data <0, 1>
            static constexpr float maxError = 1e-6f;

            vec3f vals;
            m_node->getTranslation(vals[0], vals[1], vals[2]);
            EXPECT_NEAR(expectedTranslate, vals[0], maxError);
            EXPECT_NEAR(expectedTranslate, vals[1], maxError);
            EXPECT_NEAR(expectedTranslate, vals[2], maxError);
            ramses::ERotationConvention rotConv;
            m_node->getRotation(vals[0], vals[1], vals[2], rotConv);
            EXPECT_NEAR(expectedRotate, vals[0], maxError);
            EXPECT_NEAR(expectedRotate, vals[1], maxError);
            EXPECT_NEAR(expectedRotate, vals[2], maxError);
            m_node->getScaling(vals[0], vals[1], vals[2]);
            EXPECT_NEAR(expectedScale, vals[0], maxError);
            EXPECT_NEAR(expectedScale, vals[1], maxError);
            EXPECT_NEAR(expectedScale, vals[2], maxError);
        }

        ramses::RamsesFramework m_ramsesFramework;
        ramses::RamsesClient* m_ramsesClient{ m_ramsesFramework.createClient("client") };
        ramses::Scene* m_scene{ m_ramsesClient->createScene(ramses::sceneId_t{123u}) };
        ramses::Node* m_node{ m_scene->createNode() };

        LogicEngine m_logicEngine;
        AnimationNode* m_animation1 = nullptr;
        AnimationNode* m_animation2 = nullptr;
        AnimationNode* m_animation3 = nullptr;
        TimerNode* m_timer = nullptr;
    };

    TEST_F(ALogicEngine_Animations, ScriptsControllingAnimationsLinkedToScene)
    {
        const auto scriptMainSrc = R"(
        function interface()
            IN.start = BOOL
            OUT.animPlay = BOOL
        end
        function run()
            OUT.animPlay = IN.start
        end
        )";

        const auto scriptDelayPlaySrc = R"(
        function interface()
            IN.progress = FLOAT
            IN.delay = FLOAT
            OUT.animPlay = BOOL
        end
        function run()
            OUT.animPlay = IN.progress >= IN.delay
        end
        )";

        const auto scriptScalarToVecSrc = R"(
        function interface()
            IN.scalar = FLOAT
            OUT.vec = VEC3F
        end
        function run()
            OUT.vec = { IN.scalar, IN.scalar, IN.scalar }
        end
        )";

        const auto scriptMain = m_logicEngine.createLuaScript(scriptMainSrc);
        const auto scriptDelayPlayAnim2 = m_logicEngine.createLuaScript(scriptDelayPlaySrc);
        const auto scriptDelayPlayAnim3 = m_logicEngine.createLuaScript(scriptDelayPlaySrc);
        const auto scriptScalarToVec1 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec2 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec3 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);

        const auto* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node);

        // main -> anim1/progress -> delayPlay2 -> anim2/channel -> scalarToVec -> nodeRotation
        //             |          -> delayPlay3 -> anim3/channel -> scalarToVec -> nodeScaling
        //             |/channel -> scalarToVec -> nodeTranslation
        m_logicEngine.link(*scriptMain->getOutputs()->getChild("animPlay"), *m_animation1->getInputs()->getChild("play"));
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("progress"), *scriptDelayPlayAnim2->getInputs()->getChild("progress"));
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("progress"), *scriptDelayPlayAnim3->getInputs()->getChild("progress"));
        m_logicEngine.link(*scriptDelayPlayAnim2->getOutputs()->getChild("animPlay"), *m_animation2->getInputs()->getChild("play"));
        m_logicEngine.link(*scriptDelayPlayAnim3->getOutputs()->getChild("animPlay"), *m_animation3->getInputs()->getChild("play"));
        // link anim outputs to node bindings via helper to convert scalar to vec3
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("channel"), *scriptScalarToVec1->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation2->getOutputs()->getChild("channel"), *scriptScalarToVec2->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation3->getOutputs()->getChild("channel"), *scriptScalarToVec3->getInputs()->getChild("scalar"));
        m_logicEngine.link(*scriptScalarToVec1->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("translation"));
        m_logicEngine.link(*scriptScalarToVec2->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("rotation"));
        m_logicEngine.link(*scriptScalarToVec3->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("scaling"));

        // main animation will be looping
        m_animation1->getInputs()->getChild("loop")->set(true);
        // delayed animations will be restarted every cycle
        m_animation2->getInputs()->getChild("rewindOnStop")->set(true);
        m_animation3->getInputs()->getChild("rewindOnStop")->set(true);

        // set delays on delayed animations
        scriptDelayPlayAnim2->getInputs()->getChild("delay")->set(0.3f);
        scriptDelayPlayAnim3->getInputs()->getChild("delay")->set(0.6f);

        // initial state
        advanceAnimationsAndUpdate(0.f);
        expectNodeValues(0.f, 0.f, 0.f);

        // start main script
        scriptMain->getInputs()->getChild("start")->set(true);
        advanceAnimationsAndUpdate(0.f);
        expectNodeValues(0.f, 0.f, 0.f);

        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.1f, 0.f, 0.f);

        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.2f, 0.f, 0.f);

        // delayed anim2 plays
        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.3f, 0.1f, 0.f);

        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.4f, 0.2f, 0.f);

        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.5f, 0.3f, 0.f);

        // delayed anim3 plays
        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.6f, 0.4f, 0.1f);

        advanceAnimationsAndUpdate(0.1f);
        expectNodeValues(0.7f, 0.5f, 0.2f);

        advanceAnimationsAndUpdate(0.2f); // (use bigger time deltas from now on)
        expectNodeValues(0.9f, 0.7f, 0.4f);

        // main anim reaches end and loops from beginning
        // this causes delayed anims to be stopped and rewound
        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(0.1f, 0.f, 0.f);

        // delayed anim2 plays new cycle
        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(0.3f, 0.2f, 0.f);

        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(0.5f, 0.4f, 0.f);

        // delayed anim3 plays new cycle
        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(0.7f, 0.6f, 0.2f);

        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(0.9f, 0.8f, 0.4f);

        // disable looping of main animation, expect all animations reach end
        m_animation1->getInputs()->getChild("loop")->set(false);
        advanceAnimationsAndUpdate(0.2f);
        expectNodeValues(1.f, 1.f, 0.6f);
        advanceAnimationsAndUpdate(0.5f);
        expectNodeValues(1.f, 1.f, 1.f);
        advanceAnimationsAndUpdate(100.f);
        expectNodeValues(1.f, 1.f, 1.f);
    }

    TEST_F(ALogicEngine_Animations, ScriptsControllingAnimationsLinkedToScene_UsingTimerNodeWithUserProvidedTicker)
    {
        const auto scriptMainSrc = R"(
        function interface()
            IN.start = BOOL
            OUT.animPlay = BOOL
        end
        function run()
            OUT.animPlay = IN.start
        end
        )";

        const auto scriptDelayPlaySrc = R"(
        function interface()
            IN.progress = FLOAT
            IN.delay = FLOAT
            OUT.animPlay = BOOL
        end
        function run()
            OUT.animPlay = IN.progress >= IN.delay
        end
        )";

        const auto scriptScalarToVecSrc = R"(
        function interface()
            IN.scalar = FLOAT
            OUT.vec = VEC3F
        end
        function run()
            OUT.vec = { IN.scalar, IN.scalar, IN.scalar }
        end
        )";

        const auto scriptMain = m_logicEngine.createLuaScript(scriptMainSrc);
        const auto scriptDelayPlayAnim2 = m_logicEngine.createLuaScript(scriptDelayPlaySrc);
        const auto scriptDelayPlayAnim3 = m_logicEngine.createLuaScript(scriptDelayPlaySrc);
        const auto scriptScalarToVec1 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec2 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec3 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);

        const auto* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node);

        // main -> anim1/progress -> delayPlay2 -> anim2/channel -> scalarToVec -> nodeRotation
        //             |          -> delayPlay3 -> anim3/channel -> scalarToVec -> nodeScaling
        //             |/channel -> scalarToVec -> nodeTranslation
        m_logicEngine.link(*scriptMain->getOutputs()->getChild("animPlay"), *m_animation1->getInputs()->getChild("play"));
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("progress"), *scriptDelayPlayAnim2->getInputs()->getChild("progress"));
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("progress"), *scriptDelayPlayAnim3->getInputs()->getChild("progress"));
        m_logicEngine.link(*scriptDelayPlayAnim2->getOutputs()->getChild("animPlay"), *m_animation2->getInputs()->getChild("play"));
        m_logicEngine.link(*scriptDelayPlayAnim3->getOutputs()->getChild("animPlay"), *m_animation3->getInputs()->getChild("play"));
        // link anim outputs to node bindings via helper to convert scalar to vec3
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("channel"), *scriptScalarToVec1->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation2->getOutputs()->getChild("channel"), *scriptScalarToVec2->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation3->getOutputs()->getChild("channel"), *scriptScalarToVec3->getInputs()->getChild("scalar"));
        m_logicEngine.link(*scriptScalarToVec1->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("translation"));
        m_logicEngine.link(*scriptScalarToVec2->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("rotation"));
        m_logicEngine.link(*scriptScalarToVec3->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("scaling"));

        // main animation will be looping
        m_animation1->getInputs()->getChild("loop")->set(true);
        // delayed animations will be restarted every cycle
        m_animation2->getInputs()->getChild("rewindOnStop")->set(true);
        m_animation3->getInputs()->getChild("rewindOnStop")->set(true);

        // set delays on delayed animations
        scriptDelayPlayAnim2->getInputs()->getChild("delay")->set(0.3f);
        scriptDelayPlayAnim3->getInputs()->getChild("delay")->set(0.6f);

        // link all animations to timer
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation1->getInputs()->getChild("timeDelta"));
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation2->getInputs()->getChild("timeDelta"));
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation3->getInputs()->getChild("timeDelta"));

        // start main script
        scriptMain->getInputs()->getChild("start")->set(true);

        // start non-zero ticker (i.e. user provided)
        // first timer set only initializes internal timestamp, wont output any timeDelta
        setTickerAndUpdate(1);
        expectNodeValues(0.f, 0.f, 0.f);

        setTickerAndUpdate(100001);
        expectNodeValues(0.1f, 0.f, 0.f);

        setTickerAndUpdate(200001);
        expectNodeValues(0.2f, 0.f, 0.f);

        // delayed anim2 plays
        setTickerAndUpdate(300001);
        expectNodeValues(0.3f, 0.1f, 0.f);

        setTickerAndUpdate(400001);
        expectNodeValues(0.4f, 0.2f, 0.f);

        setTickerAndUpdate(500001);
        expectNodeValues(0.5f, 0.3f, 0.f);

        // delayed anim3 plays
        setTickerAndUpdate(600001);
        expectNodeValues(0.6f, 0.4f, 0.1f);

        setTickerAndUpdate(700001);
        expectNodeValues(0.7f, 0.5f, 0.2f);

        setTickerAndUpdate(900001); // (use bigger time deltas from now on)
        expectNodeValues(0.9f, 0.7f, 0.4f);

        // main anim reaches end and loops from beginning
        // this causes delayed anims to be stopped and rewound
        setTickerAndUpdate(1100001);
        expectNodeValues(0.1f, 0.f, 0.f);

        // delayed anim2 plays new cycle
        setTickerAndUpdate(1300001);
        expectNodeValues(0.3f, 0.2f, 0.f);

        setTickerAndUpdate(1500001);
        expectNodeValues(0.5f, 0.4f, 0.f);

        // delayed anim3 plays new cycle
        setTickerAndUpdate(1700001);
        expectNodeValues(0.7f, 0.6f, 0.2f);

        setTickerAndUpdate(1900001);
        expectNodeValues(0.9f, 0.8f, 0.4f);

        // disable looping of main animation, expect all animations reach end
        m_animation1->getInputs()->getChild("loop")->set(false);
        setTickerAndUpdate(2100001);
        expectNodeValues(1.f, 1.f, 0.6f);
        setTickerAndUpdate(2600001);
        expectNodeValues(1.f, 1.f, 1.f);
        setTickerAndUpdate(100000001);
        expectNodeValues(1.f, 1.f, 1.f);
    }

    TEST_F(ALogicEngine_Animations, AnimationProgressesWhenUsingTimerWithAutogeneratedTicker)
    {
        const auto scriptScalarToVecSrc = R"(
        function interface()
            IN.scalar = FLOAT
            OUT.vec = VEC3F
        end
        function run()
            OUT.vec = { IN.scalar, IN.scalar, IN.scalar }
        end
        )";

        const auto scriptScalarToVec1 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec2 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);
        const auto scriptScalarToVec3 = m_logicEngine.createLuaScript(scriptScalarToVecSrc);

        const auto* nodeBinding = m_logicEngine.createRamsesNodeBinding(*m_node);

        // link anim outputs to node bindings via helper to convert scalar to vec3
        m_logicEngine.link(*m_animation1->getOutputs()->getChild("channel"), *scriptScalarToVec1->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation2->getOutputs()->getChild("channel"), *scriptScalarToVec2->getInputs()->getChild("scalar"));
        m_logicEngine.link(*m_animation3->getOutputs()->getChild("channel"), *scriptScalarToVec3->getInputs()->getChild("scalar"));
        m_logicEngine.link(*scriptScalarToVec1->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("translation"));
        m_logicEngine.link(*scriptScalarToVec2->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("rotation"));
        m_logicEngine.link(*scriptScalarToVec3->getOutputs()->getChild("vec"), *nodeBinding->getInputs()->getChild("scaling"));

        // start all animations
        m_animation1->getInputs()->getChild("play")->set(true);
        m_animation2->getInputs()->getChild("play")->set(true);
        m_animation3->getInputs()->getChild("play")->set(true);

        // link all animations to timer
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation1->getInputs()->getChild("timeDelta"));
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation2->getInputs()->getChild("timeDelta"));
        m_logicEngine.link(*m_timer->getOutputs()->getChild("timeDelta"), *m_animation3->getInputs()->getChild("timeDelta"));

        // set timer to auto-generate ticker mode
        m_timer->getInputs()->getChild("ticker_us")->set<int64_t>(0);

        // first timer set only initializes internal timestamp, wont output any timeDelta
        m_logicEngine.update();
        expectNodeValues(0.f, 0.f, 0.f);

        // loop for at most 2 secs
        // break when one of animated outputs reaches 0.1 sec progress
        for (int i = 0; i < 100; ++i)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 20 });
            m_logicEngine.update();

            vec3f vals;
            m_node->getTranslation(vals[0], vals[1], vals[2]);
            if (vals[0] >= 0.1f)
                break;
        }

        vec3f vals;
        m_node->getTranslation(vals[0], vals[1], vals[2]);
        EXPECT_GE(vals[0], 0.1f);
        EXPECT_GE(vals[1], 0.1f);
        EXPECT_GE(vals[2], 0.1f);
        ramses::ERotationConvention rotConv;
        m_node->getRotation(vals[0], vals[1], vals[2], rotConv);
        EXPECT_GE(vals[0], 0.1f);
        EXPECT_GE(vals[1], 0.1f);
        EXPECT_GE(vals[2], 0.1f);
        m_node->getScaling(vals[0], vals[1], vals[2]);
        EXPECT_GE(vals[0], 0.1f);
        EXPECT_GE(vals[1], 0.1f);
        EXPECT_GE(vals[2], 0.1f);
    }
}
