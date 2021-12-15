//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/TimerNodeImpl.h"
#include "ramses-logic/Property.h"
#include "impl/PropertyImpl.h"
#include "internals/ErrorReporting.h"
#include "generated/TimerNodeGen.h"
#include "flatbuffers/flatbuffers.h"
#include "fmt/format.h"

namespace rlogic::internal
{
    TimerNodeImpl::TimerNodeImpl(std::string_view name, uint64_t id) noexcept
        : LogicNodeImpl(name, id)
    {
        HierarchicalTypeData inputs = MakeStruct("IN", {
            {"ticker_us", EPropertyType::Int64}
            });
        auto inputsImpl = std::make_unique<PropertyImpl>(std::move(inputs), EPropertySemantics::ScriptInput);

        HierarchicalTypeData outputs = MakeStruct("OUT", {
            {"timeDelta", EPropertyType::Float},
            {"ticker_us", EPropertyType::Int64}
            });
        auto outputsImpl = std::make_unique<PropertyImpl>(std::move(outputs), EPropertySemantics::ScriptOutput);

        setRootProperties(std::make_unique<Property>(std::move(inputsImpl)), std::make_unique<Property>(std::move(outputsImpl)));
    }

    std::optional<LogicNodeRuntimeError> TimerNodeImpl::update()
    {
        const int64_t ticker = *getInputs()->getChild(0u)->get<int64_t>();
        if (ticker < 0)
            return LogicNodeRuntimeError{ fmt::format("TimerNode '{}' failed to update - cannot use negative ticker ({})", getName(), ticker) };

        int64_t outTimeDelta_us = 0;
        int64_t outTicker_us = 0;

        if (ticker == 0) // built-in ticker using system clock
        {
            const auto nowTS = std::chrono::steady_clock::now();
            if (!m_lastTimePoint)
                m_lastTimePoint = nowTS;

            assert(nowTS >= *m_lastTimePoint);
            const auto timeDelta = std::chrono::duration_cast<std::chrono::microseconds>(nowTS - *m_lastTimePoint);
            outTimeDelta_us = static_cast<int64_t>(timeDelta.count());
            outTicker_us = std::chrono::duration_cast<std::chrono::microseconds>(nowTS.time_since_epoch()).count();
            m_lastTimePoint = nowTS;
        }
        else // user provided ticker
        {
            if (!m_lastTick)
                m_lastTick = ticker;
            if (ticker < m_lastTick)
                return LogicNodeRuntimeError{ fmt::format("TimerNode '{}' failed to update - ticker must be monotonically increasing (lastTick={} newTick={})", getName(), *m_lastTick, ticker) };

            outTimeDelta_us = ticker - *m_lastTick;
            outTicker_us = ticker;
            m_lastTick = ticker;
        }

        // Calculate time delta in double and and narrow to float for output.
        // Safe double representation of ticker can be assumed - user input is checked for same range due to Lua using doubles
        // and system clock will not generate higher value for few centuries.
        assert(outTimeDelta_us < static_cast<int64_t>(1LLU << 53u));
        const auto outTimeDelta = static_cast<float>(1e-6 * static_cast<double>(outTimeDelta_us));

        getOutputs()->getChild(0u)->m_impl->setValue(outTimeDelta);
        getOutputs()->getChild(1u)->m_impl->setValue(outTicker_us);

        return std::nullopt;
    }

    flatbuffers::Offset<rlogic_serialization::TimerNode> TimerNodeImpl::Serialize(
        const TimerNodeImpl& timerNode,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        return rlogic_serialization::CreateTimerNode(
            builder,
            builder.CreateString(timerNode.getName()),
            timerNode.getId(),
            PropertyImpl::Serialize(*timerNode.getInputs()->m_impl, builder, serializationMap),
            PropertyImpl::Serialize(*timerNode.getOutputs()->m_impl, builder, serializationMap)
        );
    }

    std::unique_ptr<TimerNodeImpl> TimerNodeImpl::Deserialize(
        const rlogic_serialization::TimerNode& timerNodeFB,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!timerNodeFB.name() || timerNodeFB.id() == 0u || !timerNodeFB.rootInput() || !timerNodeFB.rootOutput())
        {
            errorReporting.add("Fatal error during loading of TimerNode from serialized data: missing name, id or in/out property data!", nullptr);
            return nullptr;
        }

        const auto name = timerNodeFB.name()->string_view();
        auto deserialized = std::make_unique<TimerNodeImpl>(name, timerNodeFB.id());

        // deserialize and overwrite constructor generated properties
        auto rootInProperty = PropertyImpl::Deserialize(*timerNodeFB.rootInput(), EPropertySemantics::ScriptInput, errorReporting, deserializationMap);
        auto rootOutProperty = PropertyImpl::Deserialize(*timerNodeFB.rootOutput(), EPropertySemantics::ScriptOutput, errorReporting, deserializationMap);
        if (rootInProperty->getChildCount() != 1u ||
            rootOutProperty->getChildCount() != 2u ||
            !rootInProperty->getChild(0u) || rootInProperty->getChild(0u)->getName() != "ticker_us" ||
            !rootOutProperty->getChild(0u) || rootOutProperty->getChild(0u)->getName() != "timeDelta" ||
            !rootOutProperty->getChild(1u) || rootOutProperty->getChild(1u)->getName() != "ticker_us")
        {
            errorReporting.add(fmt::format("Fatal error during loading of TimerNode '{}': missing or invalid properties!", name), nullptr);
            return nullptr;
        }
        deserialized->setRootProperties(std::make_unique<Property>(std::move(rootInProperty)), std::make_unique<Property>(std::move(rootOutProperty)));

        return deserialized;
    }
}
