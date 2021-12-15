//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/LogicNode.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/EPropertyType.h"
#include <string>
#include <memory>

namespace rlogic::internal
{
    class AnimationNodeImpl;
}

namespace rlogic
{
    /**
    * Animation node can be used to animate properties in logic network.
    * Animation node itself is a logic node and has a set of input and output properties:
    * - Fixed inputs:
    *     - timeDelta (float)   - how much time to advance the animation if playing
    *                             (units should match with #rlogic::AnimationChannel::timeStamps)
    *                           - typical application running in a loop will provide timeDelta once per loop
    *                             as duration elapsed between last and current loop
    *     - play (bool)         - will advance in animation if true or no update if false
    *     - loop (bool)         - if true, will loop animation when playing (i.e. start over whenever end is reached)
    *     - rewindOnStop (bool) - if true, whenever animation is stopped (play=false) it will jump
    *                             to the beginning (as if it never started)
    *                           - animation will rewind also if not playing and this input is switched from false to true
    *     - timeRange (vec2f)   - by default animation is played from time 0 to the last timestamp of its longest channel,
    *                             this can be changed by using this input by providing vec2f{ timeRangeBegin, timeRangeEnd },
    *                             animation will then play strictly within this time range (applies also to loop and rewindOnStop)
    *                           - time range end is optional, if set to 0 or negative then the original maximum duration
    *                             will be used (#getDuration)
    *                           - if end is specified (positive value) it must always be larger than begin
    *                             or else the node update will fail
    * - Fixed outputs:
    *     - progress (float)  - a [0;1] normalized progress of animation where 0 is beginning, 1 is end
    * - Channel outputs: Each animation channel provided at creation time (#rlogic::LogicEngine::createAnimationNode)
    *                    will be represented as output property with name of the channel (#rlogic::AnimationChannel::name)
    *                    and a value of type matching element in #rlogic::AnimationChannel::keyframes.
    *                    Channel value output is a result of keyframes interpolation based on applied time deltas,
    *                    it can be linked to another logic node input to process the animation result.
    *
    * On #rlogic::LogicEngine::update all animation nodes will be updated if and only if there was any of the inputs set
    * (regardless if value changed or not), for this reason it is important that application (directly to node input
    * or indirectly via logic network) sets timeDelta regularly (typically every loop/frame).
    * Now when animation is playing (play input is true) and a non-negative timeDelta is set, the logic will
    * advance the animation progress, i.e. it will:
    *     - add timeDelta to already elapsed time of played animation (from previous updates)
    *     - for each channel it will:
    *         - lookup closest previous and next timestamp/keyframe pair according to the new total elapsed play time,
    *         - interpolate between them according to the interpolation type of that channel,
    *         - and finally set this value to the channel's output property.
    *     - update 'progress' output accordingly
    *
    * Note that all channel outputs will always have a value determined by corresponding keyframes, this includes
    * also when the time falls outside of the first/last animation timestamps:
    *     - channel output value equals first keyframe for any time at or before the first keyframe timestamp
    *     - channel output value equals last keyframe for any time at or after the last keyframe timestamp
    * This can be useful for example when needing to initialize the outputs before playing the animation yet,
    * when updating the animation node with timeDelta 0, the logic will execute and update outputs to their
    * first keyframes.
    */
    class AnimationNode : public LogicNode
    {
    public:
        /**
        * Constructor of AnimationNode. User is not supposed to call this - AnimationNodes are created by other factory classes
        *
        * @param impl implementation details of the AnimationNode
        */
        explicit AnimationNode(std::unique_ptr<internal::AnimationNodeImpl> impl) noexcept;

        /**
        * Destructor of AnimationNode.
        */
        ~AnimationNode() noexcept override;

        /**
        * Gets maximum duration of this animation's channel data.
        * The duration is determined by the highest timestamp value in timestamps data of all channels
        * (#rlogic::AnimationChannel::timeStamps) and is not affected by 'timeRange' property input.
        *
        * @return maximum duration of this animation.
        */
        [[nodiscard]] RLOGIC_API float getDuration() const;

        /**
        * Returns channel data used in this animation (as provided at creation time #rlogic::LogicEngine::createAnimationNode).
        *
        * @return animation channels used in this animation.
        */
        [[nodiscard]] RLOGIC_API const AnimationChannels& getChannels() const;

        /**
        * Copy Constructor of AnimationNode is deleted because AnimationNodes are not supposed to be copied
        *
        * @param other AnimationNodes to copy from
        */
        AnimationNode(const AnimationNode& other) = delete;

        /**
        * Move Constructor of AnimationNode is deleted because AnimationNodes are not supposed to be moved
        *
        * @param other AnimationNodes to move from
        */
        AnimationNode(AnimationNode&& other) = delete;

        /**
        * Assignment operator of AnimationNode is deleted because AnimationNodes are not supposed to be copied
        *
        * @param other AnimationNodes to assign from
        */
        AnimationNode& operator=(const AnimationNode& other) = delete;

        /**
        * Move assignment operator of AnimationNode is deleted because AnimationNodes are not supposed to be moved
        *
        * @param other AnimationNodes to assign from
        */
        AnimationNode& operator=(AnimationNode&& other) = delete;

        /**
        * Implementation of AnimationNode
        */
        internal::AnimationNodeImpl& m_animationNodeImpl;
    };
}
