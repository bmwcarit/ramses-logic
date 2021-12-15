//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/LogicNode.h"
#include <memory>

namespace rlogic::internal
{
    class TimerNodeImpl;
}

namespace rlogic
{
    /**
    * Timer node can be used to provide timing information to animation nodes (#rlogic::AnimationNode) or any other logic nodes.
    * - Property inputs:
    *     - ticker_us (int64)  - (optional) user provided ticker in microseconds (by default, see below to learn how to use other time units).
    *                          - ticker value must be monotonically increasing and positive otherwise node update will fail.
    *                          - if the input is 0 (default) then this TimerNode uses system clock to generate ticker by itself,
    *                            this is recommended for simple use cases when application does not need more advanced timing control.
    * - Property outputs:
    *     - timeDelta (float)  - time elapsed since last update, this is what an animation node needs to animate, see #rlogic::AnimationNode.
    *                          - the value is in seconds with the exception when user provided input 'ticker_us' is using other than microseconds,
    *                            in that case the time unit will be 10^-6 of unit provided in 'ticker_us' input
    *     - ticker_us (int64)  - this output is only useful if timer node generates ticker by itself, then this is the system clock time
    *                            since epoch in microseconds
    *                          - in case of user provided ticker (i.e. non-zero 'ticker_us' input) this output will contain the same value
    *                            (user ticker is just passed through).
    *
    * Timer node works in one of two modes - it generates ticker by itself or uses user provided ticker - and then calculates timeDelta.
    * Mainly due to the auto-generate mode the inputs and outputs have defined time units, however timer node can also be used in a fully
    * time unit agnostic mode, see inputs/outputs description above for details. The timeDelta unit was chosen to be in seconds (float) by default,
    * because that is the most commonly used time unit for GLTF animations which are expected to be the main use case for Ramses logic animations.
    * Even though TimerNode was mainly designed to be used in combination with #rlogic::AnimationNode, it can be used with any other
    * logic node (e.g. a LuaScript) if it benefits from any of its outputs.
    * Note that unlike other logic nodes a TimerNode is always updated on every #rlogic::LogicEngine::update call regardless of if any of its
    * inputs were modified or not.
    *
    * It is recommended to use single instance of TimerNode and link its 'timeDelta' to all animation nodes which are supposed to be running
    * in the same time context/space. More advanced use cases can utilize multiple instances of TimerNode and set up different time contexts
    * for different sets of animations, with different time advance speeds, units or even non-linear time progress.
    */
    class TimerNode : public LogicNode
    {
    public:
        /**
        * Constructor of TimerNode. User is not supposed to call this - TimerNodes are created by other factory classes
        *
        * @param impl implementation details of the TimerNode
        */
        explicit TimerNode(std::unique_ptr<internal::TimerNodeImpl> impl) noexcept;

        /**
        * Destructor of TimerNode.
        */
        ~TimerNode() noexcept override;

        /**
        * Copy Constructor of TimerNode is deleted because TimerNodes are not supposed to be copied
        */
        TimerNode(const TimerNode&) = delete;

        /**
        * Move Constructor of TimerNode is deleted because TimerNodes are not supposed to be moved
        */
        TimerNode(TimerNode&&) = delete;

        /**
        * Assignment operator of TimerNode is deleted because TimerNodes are not supposed to be copied
        */
        TimerNode& operator=(const TimerNode&) = delete;

        /**
        * Move assignment operator of TimerNode is deleted because TimerNodes are not supposed to be moved
        */
        TimerNode& operator=(TimerNode&&) = delete;

        /**
        * Implementation of TimerNode
        */
        internal::TimerNodeImpl& m_timerNodeImpl;
    };
}
