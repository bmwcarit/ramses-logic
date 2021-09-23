//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/AnimationNodeImpl.h"
#include "impl/PropertyImpl.h"
#include "impl/DataArrayImpl.h"
#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/DataArray.h"
#include "internals/EPropertySemantics.h"
#include "internals/ErrorReporting.h"
#include "generated/AnimationNodeGen.h"
#include "flatbuffers/flatbuffers.h"
#include "fmt/format.h"
#include <cmath>

namespace rlogic::internal
{
    AnimationNodeImpl::AnimationNodeImpl(AnimationChannels channels, std::string_view name) noexcept
        : LogicNodeImpl(name)
        , m_channels{ std::move(channels) }
    {
        HierarchicalTypeData inputs = MakeStruct("IN", {
            {"timeDelta", EPropertyType::Float},   // EInputIdx_TimeDelta
            {"play", EPropertyType::Bool},         // EInputIdx_Play
            {"loop", EPropertyType::Bool},         // EInputIdx_Loop
            {"rewindOnStop", EPropertyType::Bool}  // EInputIdx_RewindOnStop
            });
        auto inputsImpl = std::make_unique<PropertyImpl>(std::move(inputs), EPropertySemantics::AnimationInput);

        HierarchicalTypeData outputs = MakeStruct("OUT", {
            {"progress", EPropertyType::Float},    // EPropertyOutputIndex::Progress
            });
        for (const auto& channel : m_channels)
        {
            assert(channel.timeStamps && channel.keyframes);
            assert(channel.timeStamps->getNumElements() == channel.keyframes->getNumElements());
            assert(!channel.tangentsIn || channel.timeStamps->getNumElements() == channel.tangentsIn->getNumElements());
            assert(!channel.tangentsOut || channel.timeStamps->getNumElements() == channel.tangentsOut->getNumElements());
            outputs.children.push_back(MakeType(std::string{ channel.name }, channel.keyframes->getDataType()));
            // overall duration equals longest channel in animation
            m_duration = std::max(m_duration, channel.timeStamps->getData<float>()->back());
        }
        auto outputsImpl = std::make_unique<PropertyImpl>(std::move(outputs), EPropertySemantics::AnimationOutput);

        setRootProperties(std::make_unique<Property>(std::move(inputsImpl)), std::make_unique<Property>(std::move(outputsImpl)));
    }

    float AnimationNodeImpl::getDuration() const
    {
        return m_duration;
    }

    const AnimationChannels& AnimationNodeImpl::getChannels() const
    {
        return m_channels;
    }

    std::optional<LogicNodeRuntimeError> AnimationNodeImpl::update()
    {
        float timeDelta = *getInputs()->getChild(EInputIdx_TimeDelta)->get<float>();
        if (timeDelta < 0.f)
            return LogicNodeRuntimeError{ fmt::format("AnimationNode '{}' failed to update - cannot use negative timeDelta ({})", getName(), timeDelta) };

        const bool play = *getInputs()->getChild(EInputIdx_Play)->get<bool>();
        if (!play)
        {
            if (m_elapsedPlayTime > 0.f && *getInputs()->getChild(EInputIdx_RewindOnStop)->get<bool>())
            {
                // rewind, i.e. reset progress and update with zero timeDelta
                m_elapsedPlayTime = 0.f;
                timeDelta = 0.f;
            }
            else
            {
                return std::nullopt;
            }
        }

        const bool loop = *getInputs()->getChild(EInputIdx_Loop)->get<bool>();
        if (m_elapsedPlayTime >= m_duration && !loop)
            return std::nullopt;

        m_elapsedPlayTime += timeDelta;

        if (loop)
        {
            // when looping enabled and elapsed time passed total duration,
            // wrap it around and start from beginning (by calculating a remainder
            // after dividing by duration)
            m_elapsedPlayTime = fmodf(m_elapsedPlayTime, m_duration);
        }

        const float progress = std::min(m_elapsedPlayTime / m_duration, 1.f);

        for (size_t i = 0u; i < m_channels.size(); ++i)
            updateChannel(i);

        getOutputs()->getChild(EOutputIdx_Progress)->m_impl->setValue(progress, true);

        return std::nullopt;
    }

    void AnimationNodeImpl::updateChannel(size_t channelIdx)
    {
        const auto& channel = m_channels[channelIdx];
        assert(channel.timeStamps->getDataType() == EPropertyType::Float && channel.timeStamps->getNumElements() > 0);
        const auto& timeStamps = *channel.timeStamps->getData<float>();

        // find upper/lower timestamp neighbor of elapsed timestamp
        auto tsUpperIt = std::upper_bound(timeStamps.cbegin(), timeStamps.cend(), m_elapsedPlayTime);
        const auto tsLowerIt = (tsUpperIt == timeStamps.cbegin() ? timeStamps.cbegin() : tsUpperIt - 1);
        tsUpperIt = (tsUpperIt == timeStamps.cend() ? tsUpperIt - 1 : tsUpperIt);

        // get index into corresponding keyframes
        const auto lowerIdx = static_cast<size_t>(std::distance(timeStamps.cbegin(), tsLowerIt));
        const auto upperIdx = static_cast<size_t>(std::distance(timeStamps.cbegin(), tsUpperIt));
        assert(lowerIdx < channel.keyframes->getNumElements());
        assert(upperIdx < channel.keyframes->getNumElements());

        // calculate interpolation ratio between the elapsed time and timestamp neighbors [0.0, 1.0] (0.0=lower, 1.0=upper)
        float interpRatio = 0.f;
        float timeBetweenKeys = *tsUpperIt - *tsLowerIt;
        if (tsUpperIt != tsLowerIt)
            interpRatio = (m_elapsedPlayTime - *tsLowerIt) / timeBetweenKeys;
        // no clamping needed mathematically but to avoid float precision issues
        interpRatio = std::clamp(interpRatio, 0.f, 1.f);

        PropertyValue interpolatedValue;
        std::visit([&](const auto& v) {
            switch (channel.interpolationType)
            {
            case EInterpolationType::Step:
                interpolatedValue = v[lowerIdx];
                break;
            case EInterpolationType::Linear:
            case EInterpolationType::Linear_Quaternions:
                interpolatedValue = interpolateKeyframes_linear(v[lowerIdx], v[upperIdx], interpRatio);
                break;
            case EInterpolationType::Cubic:
            case EInterpolationType::Cubic_Quaternions:
            {
                using ValueType = std::remove_const_t<std::remove_reference_t<decltype(v.front())>>;
                const auto& tIn = *channel.tangentsIn->getData<ValueType>();
                const auto& tOut = *channel.tangentsOut->getData<ValueType>();
                interpolatedValue = interpolateKeyframes_cubic(v[lowerIdx], v[upperIdx], tOut[lowerIdx], tIn[upperIdx], interpRatio, timeBetweenKeys);
                break;
            }
            }
        }, channel.keyframes->m_impl.getDataVariant());

        if (channel.interpolationType == EInterpolationType::Linear_Quaternions || channel.interpolationType == EInterpolationType::Cubic_Quaternions)
        {
            auto& asQuaternion = std::get<vec4f>(interpolatedValue);
            const float normalizationFactor = 1 / std::sqrt(
                asQuaternion[0] * asQuaternion[0] +
                asQuaternion[1] * asQuaternion[1] +
                asQuaternion[2] * asQuaternion[2] +
                asQuaternion[3] * asQuaternion[3]);

            asQuaternion[0] *= normalizationFactor;
            asQuaternion[1] *= normalizationFactor;
            asQuaternion[2] *= normalizationFactor;
            asQuaternion[3] *= normalizationFactor;
        }

        // 'progress' is at index 0, channel outputs are shifted by one
        getOutputs()->getChild(channelIdx + EOutputIdx_ChannelsBegin)->m_impl->setValue(std::move(interpolatedValue), true);
    }

    template <typename T>
    T AnimationNodeImpl::interpolateKeyframes_linear(T lowerVal, T upperVal, float interpRatio)
    {
        // this will be compiled for variant visitor but must not be executed
        if constexpr (std::is_same_v<T, bool>)
            assert(false);

        if constexpr (std::is_floating_point_v<T>)
        {
            return lowerVal + interpRatio * (upperVal - lowerVal);
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return lowerVal + std::lround(interpRatio * static_cast<float>(upperVal - lowerVal));
        }
        else
        {
            T val = lowerVal;
            auto valIt = val.begin();
            auto lowerValIt = lowerVal.cbegin();
            auto upperValIt = upperVal.cbegin();

            // decompose vecXy and interpolate each component separately
            for (; valIt != val.cend(); ++valIt, ++lowerValIt, ++upperValIt)
                *valIt = interpolateKeyframes_linear(*lowerValIt, *upperValIt, interpRatio);

            return val;
        }
    }

    template <typename T>
    T AnimationNodeImpl::interpolateKeyframes_cubic(T lowerVal, T upperVal, T lowerTangentOut, T upperTangentIn, float interpRatio, float timeBetweenKeys)
    {
        // this will be compiled for variant visitor but must not be executed
        if constexpr (std::is_same_v<T, bool>)
            assert(false);

        if constexpr (std::is_floating_point_v<T>)
        {
            // GLTF v2 Appendix C (https://github.com/KhronosGroup/glTF/tree/master/specification/2.0?ts=4#appendix-c-spline-interpolation)
            const float t = interpRatio;
            const float t2 = t * t;
            const float t3 = t2 * t;
            const T p0 = lowerVal;
            const T p1 = upperVal;
            const T m0 = timeBetweenKeys * lowerTangentOut;
            const T m1 = timeBetweenKeys * upperTangentIn;
            return (2.f*t3 - 3.f*t2 + 1.f) * p0 + (t3 - 2.f*t2 + t) * m0 + (-2.f*t3 + 3.f*t2) * p1 + (t3 - t2) * m1;
        }
        else if constexpr (std::is_integral_v<T>)
        {
            return std::lround(interpolateKeyframes_cubic(
                static_cast<float>(lowerVal),
                static_cast<float>(upperVal),
                static_cast<float>(lowerTangentOut),
                static_cast<float>(upperTangentIn),
                interpRatio,
                timeBetweenKeys));
        }
        else
        {
            T val = lowerVal;
            auto valIt = val.begin();
            auto lowerValIt = lowerVal.cbegin();
            auto upperValIt = upperVal.cbegin();
            auto lowerTangentOutIt = lowerTangentOut.cbegin();
            auto upperTangentInIt = upperTangentIn.cbegin();

            // decompose vecXy and interpolate each component separately
            for (; valIt != val.cend(); ++valIt, ++lowerValIt, ++upperValIt, ++lowerTangentOutIt, ++upperTangentInIt)
                *valIt = interpolateKeyframes_cubic(*lowerValIt, *upperValIt, *lowerTangentOutIt, *upperTangentInIt, interpRatio, timeBetweenKeys);

            return val;
        }
    }

    flatbuffers::Offset<rlogic_serialization::AnimationNode> AnimationNodeImpl::Serialize(
        const AnimationNodeImpl& animNode,
        flatbuffers::FlatBufferBuilder& builder,
        SerializationMap& serializationMap)
    {
        std::vector<flatbuffers::Offset<rlogic_serialization::Channel>> channelsFB;
        channelsFB.reserve(animNode.m_channels.size());
        for (const auto& channel : animNode.m_channels)
        {
            rlogic_serialization::EInterpolationType interpTypeFB = rlogic_serialization::EInterpolationType::MAX;
            switch (channel.interpolationType)
            {
            case EInterpolationType::Step:
                interpTypeFB = rlogic_serialization::EInterpolationType::Step;
                break;
            case EInterpolationType::Linear:
                interpTypeFB = rlogic_serialization::EInterpolationType::Linear;
                break;
            case EInterpolationType::Cubic:
                interpTypeFB = rlogic_serialization::EInterpolationType::Cubic;
                break;
            case EInterpolationType::Linear_Quaternions:
                interpTypeFB = rlogic_serialization::EInterpolationType::Linear_Quaternions;
                break;
            case EInterpolationType::Cubic_Quaternions:
                interpTypeFB = rlogic_serialization::EInterpolationType::Cubic_Quaternions;
                break;
            }

            channelsFB.push_back(rlogic_serialization::CreateChannel(
                builder,
                builder.CreateString(channel.name),
                serializationMap.resolveDataArrayOffset(*channel.timeStamps),
                serializationMap.resolveDataArrayOffset(*channel.keyframes),
                interpTypeFB,
                channel.tangentsIn ? serializationMap.resolveDataArrayOffset(*channel.tangentsIn) : 0,
                channel.tangentsOut ? serializationMap.resolveDataArrayOffset(*channel.tangentsOut) : 0
                ));
        }

        return rlogic_serialization::CreateAnimationNode(
            builder,
            builder.CreateString(animNode.getName()),
            builder.CreateVector(channelsFB),
            PropertyImpl::Serialize(*animNode.getInputs()->m_impl, builder, serializationMap),
            PropertyImpl::Serialize(*animNode.getOutputs()->m_impl, builder, serializationMap)
        );
    }

    std::unique_ptr<AnimationNodeImpl> AnimationNodeImpl::Deserialize(
        const rlogic_serialization::AnimationNode& animNodeFB,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!animNodeFB.name() || !animNodeFB.channels() || !animNodeFB.rootInput() || !animNodeFB.rootOutput())
        {
            errorReporting.add("Fatal error during loading of AnimationNode from serialized data: missing name, channels or in/out property data!", nullptr);
            return nullptr;
        }
        const auto name = animNodeFB.name()->string_view();

        AnimationChannels channels;
        channels.reserve(animNodeFB.channels()->size());
        for (const auto* channelFB : *animNodeFB.channels())
        {
            if (!channelFB->name() ||
                !channelFB->timestamps() ||
                !channelFB->keyframes())
            {
                errorReporting.add(fmt::format("Fatal error during loading of AnimationNode '{}' channel data: missing name, timestamps or keyframes!", name), nullptr);
                return nullptr;
            }

            AnimationChannel channel;
            channel.name = channelFB->name()->string_view();
            channel.timeStamps = &deserializationMap.resolveDataArray(*channelFB->timestamps());
            channel.keyframes = &deserializationMap.resolveDataArray(*channelFB->keyframes());

            switch (channelFB->interpolationType())
            {
            case rlogic_serialization::EInterpolationType::Step:
                channel.interpolationType = EInterpolationType::Step;
                break;
            case rlogic_serialization::EInterpolationType::Linear:
                channel.interpolationType = EInterpolationType::Linear;
                break;
            case rlogic_serialization::EInterpolationType::Cubic:
                channel.interpolationType = EInterpolationType::Cubic;
                break;
            case rlogic_serialization::EInterpolationType::Linear_Quaternions:
                channel.interpolationType = EInterpolationType::Linear_Quaternions;
                break;
            case rlogic_serialization::EInterpolationType::Cubic_Quaternions:
                channel.interpolationType = EInterpolationType::Cubic_Quaternions;
                break;
            default:
                errorReporting.add(fmt::format("Fatal error during loading of AnimationNode '{}' channel '{}' data: missing or invalid interpolation type!", name, channel.name), nullptr);
                return nullptr;
            }

            if (channel.interpolationType == EInterpolationType::Cubic || channel.interpolationType == EInterpolationType::Cubic_Quaternions)
            {
                if (!channelFB->tangentsIn() ||
                    !channelFB->tangentsOut())
                {
                    errorReporting.add(fmt::format("Fatal error during loading of AnimationNode '{}' channel '{}' data: missing tangents!", name, channel.name), nullptr);
                    return nullptr;
                }

                channel.tangentsIn = &deserializationMap.resolveDataArray(*channelFB->tangentsIn());
                channel.tangentsOut = &deserializationMap.resolveDataArray(*channelFB->tangentsOut());
            }

            channels.push_back(std::move(channel));
        }

        auto deserialized = std::make_unique<AnimationNodeImpl>(std::move(channels), name);

        // deserialize and overwrite constructor generated properties
        auto rootInProperty = PropertyImpl::Deserialize(*animNodeFB.rootInput(), EPropertySemantics::AnimationInput, errorReporting, deserializationMap);
        auto rootOutProperty = PropertyImpl::Deserialize(*animNodeFB.rootOutput(), EPropertySemantics::AnimationOutput, errorReporting, deserializationMap);
        if (!rootInProperty->getChild(EInputIdx_TimeDelta) || rootInProperty->getChild(EInputIdx_TimeDelta)->getName() != "timeDelta" ||
            !rootInProperty->getChild(EInputIdx_Play) || rootInProperty->getChild(EInputIdx_Play)->getName() != "play" ||
            !rootInProperty->getChild(EInputIdx_Loop) || rootInProperty->getChild(EInputIdx_Loop)->getName() != "loop" ||
            !rootInProperty->getChild(EInputIdx_RewindOnStop) || rootInProperty->getChild(EInputIdx_RewindOnStop)->getName() != "rewindOnStop" ||
            !rootOutProperty->getChild(EOutputIdx_Progress) || rootOutProperty->getChild(EOutputIdx_Progress)->getName() != "progress" ||
            rootOutProperty->getChildCount() != deserialized->getChannels().size() + 1)
        {
            errorReporting.add(fmt::format("Fatal error during loading of AnimationNode '{}': missing or invalid properties!", name), nullptr);
            return nullptr;
        }
        deserialized->setRootProperties(std::make_unique<Property>(std::move(rootInProperty)), std::make_unique<Property>(std::move(rootOutProperty)));

        return deserialized;
    }
}
