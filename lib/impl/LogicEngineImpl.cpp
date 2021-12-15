//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicEngineImpl.h"

#include "ramses-framework-api/RamsesVersion.h"
#include "ramses-logic/LogicNode.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/TimerNode.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LoggerImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/LuaConfigImpl.h"
#include "impl/LogicEngineReportImpl.h"

#include "internals/FileUtils.h"
#include "internals/TypeUtils.h"
#include "internals/FileFormatVersions.h"
#include "internals/RamsesObjectResolver.h"
#include "internals/ApiObjects.h"

#include "generated/LogicEngineGen.h"
#include "ramses-logic-build-config.h"

#include "fmt/format.h"

#include <string>
#include <fstream>
#include <streambuf>

namespace rlogic::internal
{
    LogicEngineImpl::LogicEngineImpl()
        : m_apiObjects(std::make_unique<ApiObjects>())
    {
    }

    LogicEngineImpl::~LogicEngineImpl() noexcept = default;

    LuaScript* LogicEngineImpl::createLuaScript(std::string_view source, const LuaConfigImpl& config, std::string_view scriptName)
    {
        m_errors.clear();
        return m_apiObjects->createLuaScript(source, config, scriptName, m_errors);
    }

    LuaModule* LogicEngineImpl::createLuaModule(std::string_view source, const LuaConfigImpl& config, std::string_view moduleName)
    {
        m_errors.clear();
        return m_apiObjects->createLuaModule(source, config, moduleName, m_errors);
    }

    bool LogicEngineImpl::extractLuaDependencies(std::string_view source, const std::function<void(const std::string&)>& callbackFunc)
    {
        m_errors.clear();
        const std::optional<std::vector<std::string>> extractedDependencies = LuaCompilationUtils::ExtractModuleDependencies(source, m_errors);
        if (!extractedDependencies)
            return false;

        for (const auto& dep : *extractedDependencies)
            callbackFunc(dep);

        return true;
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesNodeBinding(ramsesNode, rotationType,  name);
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesAppearanceBinding(ramsesAppearance, name);
    }

    RamsesCameraBinding* LogicEngineImpl::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createRamsesCameraBinding(ramsesCamera, name);
    }

    template <typename T>
    DataArray* LogicEngineImpl::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        m_errors.clear();
        if (data.empty())
        {
            m_errors.add(fmt::format("Cannot create DataArray '{}' with empty data.", name), nullptr);
            return nullptr;
        }

        return m_apiObjects->createDataArray(data, name);
    }

    rlogic::AnimationNode* LogicEngineImpl::createAnimationNode(const AnimationChannels& channels, std::string_view name)
    {
        m_errors.clear();

        auto containsDataArray = [this](const DataArray* da) {
            const auto& dataArrays = m_apiObjects->getApiObjectContainer<DataArray>();
            const auto it = std::find_if(dataArrays.cbegin(), dataArrays.cend(),
                [da](const auto& d) { return d == da; });
            return it != dataArrays.cend();
        };

        if (channels.empty())
        {
            m_errors.add(fmt::format("Failed to create AnimationNode '{}': must provide at least one channel.", name), nullptr);
            return nullptr;
        }
        for (const auto& channel : channels)
        {
            if (!channel.timeStamps || !channel.keyframes)
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': every channel must provide timestamps and keyframes data.", name), nullptr);
                return nullptr;
            }
            // Checked at channel creation type, can't fail here
            assert(CanPropertyTypeBeAnimated(channel.keyframes->getDataType()));
            if (channel.timeStamps->getDataType() != EPropertyType::Float)
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': all channel timestamps must be float type.", name), nullptr);
                return nullptr;
            }
            if (channel.timeStamps->getNumElements() != channel.keyframes->getNumElements())
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': number of keyframes must be same as number of timestamps.", name), nullptr);
                return nullptr;
            }
            const auto& timestamps = *channel.timeStamps->getData<float>();
            if (std::adjacent_find(timestamps.cbegin(), timestamps.cend(), std::greater_equal<float>()) != timestamps.cend())
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': timestamps have to be strictly in ascending order.", name), nullptr);
                return nullptr;
            }

            if (!containsDataArray(channel.timeStamps) ||
                !containsDataArray(channel.keyframes))
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': timestamps or keyframes were not found in this logic instance.", name), nullptr);
                return nullptr;
            }

            if ((channel.interpolationType == EInterpolationType::Linear_Quaternions || channel.interpolationType == EInterpolationType::Cubic_Quaternions) &&
                channel.keyframes->getDataType() != EPropertyType::Vec4f)
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': quaternion animation requires the channel keyframes to be of type vec4f.", name), nullptr);
                return nullptr;
            }

            if (channel.interpolationType == EInterpolationType::Cubic || channel.interpolationType == EInterpolationType::Cubic_Quaternions)
            {
                if (!channel.tangentsIn || !channel.tangentsOut)
                {
                    m_errors.add(fmt::format("Failed to create AnimationNode '{}': cubic interpolation requires tangents to be provided.", name), nullptr);
                    return nullptr;
                }
                if (channel.tangentsIn->getDataType() != channel.keyframes->getDataType() ||
                    channel.tangentsOut->getDataType() != channel.keyframes->getDataType())
                {
                    m_errors.add(fmt::format("Failed to create AnimationNode '{}': tangents must be of same data type as keyframes.", name), nullptr);
                    return nullptr;
                }
                if (channel.tangentsIn->getNumElements() != channel.keyframes->getNumElements() ||
                    channel.tangentsOut->getNumElements() != channel.keyframes->getNumElements())
                {
                    m_errors.add(fmt::format("Failed to create AnimationNode '{}': number of tangents in/out must be same as number of keyframes.", name), nullptr);
                    return nullptr;
                }
                if (!containsDataArray(channel.tangentsIn) ||
                    !containsDataArray(channel.tangentsOut))
                {
                    m_errors.add(fmt::format("Failed to create AnimationNode '{}': tangents were not found in this logic instance.", name), nullptr);
                    return nullptr;
                }
            }
            else if (channel.tangentsIn || channel.tangentsOut)
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': tangents were provided for other than cubic interpolation type.", name), nullptr);
                return nullptr;
            }
        }

        return m_apiObjects->createAnimationNode(channels, name);
    }

    TimerNode* LogicEngineImpl::createTimerNode(std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects->createTimerNode(name);
    }

    bool LogicEngineImpl::destroy(LogicObject& object)
    {
        m_errors.clear();
        return m_apiObjects->destroy(object, m_errors);
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_apiObjects->getLogicNodeDependencies().isLinked(logicNode.m_impl);
    }

    size_t LogicEngineImpl::activateLinksRecursive(PropertyImpl& output)
    {
        size_t activatedLinks = 0u;

        const auto childCount = output.getChildCount();
        for (size_t i = 0; i < childCount; ++i)
        {
            PropertyImpl& child = *output.getChild(i)->m_impl;

            if (TypeUtils::CanHaveChildren(child.getType()))
            {
                activatedLinks += activateLinksRecursive(child);
            }
            else
            {
                std::vector<PropertyImpl*>& outgoingProperties = child.getLinkedOutgoingProperties();

                for (auto* linkedInput : outgoingProperties)
                {
                    const bool valueChanged = linkedInput->setValue(child.getValue());
                    if (valueChanged || linkedInput->getPropertySemantics() == EPropertySemantics::AnimationInput)
                    {
                        linkedInput->getLogicNode().setDirty(true);
                        ++activatedLinks;
                    }
                }
            }
        }

        return activatedLinks;
    }

    bool LogicEngineImpl::update()
    {
        m_errors.clear();

        if (m_updateReportEnabled)
        {
            m_updateReport.clear();
            m_updateReport.sectionStarted(UpdateReport::ETimingSection::TotalUpdate);
            m_updateReport.sectionStarted(UpdateReport::ETimingSection::TopologySort);
        }

        const std::optional<NodeVector>& sortedNodes = m_apiObjects->getLogicNodeDependencies().getTopologicallySortedNodes();
        if (!sortedNodes)
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling update()!", nullptr);
            return false;
        }

        if (m_updateReportEnabled)
            m_updateReport.sectionFinished(UpdateReport::ETimingSection::TopologySort);

        // force dirty all timer nodes so they update their tickers
        for (TimerNode* timerNode : m_apiObjects->getApiObjectContainer<TimerNode>())
            timerNode->m_impl.setDirty(true);

        const bool success = updateNodes(*sortedNodes);

        if (m_updateReportEnabled)
            m_updateReport.sectionFinished(UpdateReport::ETimingSection::TotalUpdate);

        return success;
    }

    bool LogicEngineImpl::updateNodes(const NodeVector& sortedNodes)
    {
        for (LogicNodeImpl* nodeIter : sortedNodes)
        {
            LogicNodeImpl& node = *nodeIter;
            LogicNode* hlNode = m_updateReportEnabled ? m_apiObjects->getApiObject(node) : nullptr;

            if (!node.isDirty())
            {
                if (m_updateReportEnabled)
                    m_updateReport.nodeSkippedExecution(hlNode);

                if(m_nodeDirtyMechanismEnabled)
                    continue;
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionStarted(hlNode);

            const std::optional<LogicNodeRuntimeError> potentialError = node.update();
            if (potentialError)
            {
                m_errors.add(potentialError->message, m_apiObjects->getApiObject(node));
                return false;
            }

            Property* outputs = node.getOutputs();
            if (outputs != nullptr)
            {
                const size_t activatedLinks = activateLinksRecursive(*outputs->m_impl);

                if (m_updateReportEnabled)
                    m_updateReport.linksActivated(activatedLinks);
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionFinished();

            node.setDirty(false);
        }

        return true;
    }

    const std::vector<ErrorData>& LogicEngineImpl::getErrors() const
    {
        return m_errors.getErrors();
    }

    bool LogicEngineImpl::checkLogicVersionFromFile(std::string_view dataSourceDescription, uint32_t fileVersion)
    {
        // Backwards compatibility check
        if (fileVersion < g_FileFormatVersion)
        {
            // TODO Violin write unit test for this case once we have a breaking change in the serialization format
            // Currently not possible because 1 is the first version and 0 has special semantic (== version not set)
            m_errors.add(fmt::format("Version of data source '{}' is too old! Expected file version {} but found {}",
                dataSourceDescription, g_FileFormatVersion, fileVersion), nullptr);
            return false;
        }

        // Forward compatibility check
        if (fileVersion > g_FileFormatVersion)
        {
            m_errors.add(fmt::format("Version of data source '{}' is too new! Expected file version {} but found {}",
                dataSourceDescription, g_FileFormatVersion, fileVersion), nullptr);
            return false;
        }

        return true;
    }

    bool LogicEngineImpl::CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion)
    {
        // Only major version changes result in file incompatibilities
        return static_cast<int>(ramsesVersion.v_major()) == ramses::GetRamsesVersion().major;
    }

    bool LogicEngineImpl::loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* scene, bool enableMemoryVerification)
    {
        return loadFromByteData(rawBuffer, bufferSize, scene, enableMemoryVerification, fmt::format("data buffer '{}' (size: {})", rawBuffer, bufferSize));
    }

    bool LogicEngineImpl::loadFromFile(std::string_view filename, ramses::Scene* scene, bool enableMemoryVerification)
    {
        std::optional<std::vector<char>> maybeBytesFromFile = FileUtils::LoadBinary(std::string(filename));
        if (!maybeBytesFromFile)
        {
            m_errors.add(fmt::format("Failed to load file '{}'", filename), nullptr);
            return false;
        }

        const size_t fileSize = (*maybeBytesFromFile).size();
        return loadFromByteData((*maybeBytesFromFile).data(), fileSize, scene, enableMemoryVerification, fmt::format("file '{}' (size: {})", filename, fileSize));
    }

    // TODO Violin consider handling errors gracefully, e.g. don't change state when error occurs
    // Idea: collect data, and only move() in the end when everything was loaded correctly
    bool LogicEngineImpl::loadFromByteData(const void* byteData, size_t byteSize, ramses::Scene* scene, bool enableMemoryVerification, const std::string& dataSourceDescription)
    {
        m_errors.clear();

        if (enableMemoryVerification)
        {
            flatbuffers::Verifier bufferVerifier(static_cast<const uint8_t*>(byteData), byteSize);
            const bool bufferOK = rlogic_serialization::VerifyLogicEngineBuffer(bufferVerifier);

            if (!bufferOK)
            {
                m_errors.add(fmt::format("{} contains corrupted data!", dataSourceDescription), nullptr);
                return false;
            }
        }

        const auto* logicEngine = rlogic_serialization::GetLogicEngine(byteData);

        if (nullptr == logicEngine)
        {
            m_errors.add(fmt::format("{} doesn't contain logic engine data with readable version specifiers", dataSourceDescription), nullptr);
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        const auto& rlogicVersion = *logicEngine->rlogicVersion();

        LOG_INFO("Loading logic engine contents from '{}' which was exported with Ramses {} and Logic Engine {}", dataSourceDescription, ramsesVersion.v_string()->string_view(), rlogicVersion.v_string()->string_view());

        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected Ramses version {}.x.x but found {}",
                dataSourceDescription, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()), nullptr);
            return false;
        }

        if (!checkLogicVersionFromFile(dataSourceDescription, logicEngine->rlogicVersion()->v_fileFormatVersion()))
            return false;

        if (nullptr == logicEngine->apiObjects())
        {
            m_errors.add(fmt::format("Fatal error while loading {}: doesn't contain API objects!", dataSourceDescription), nullptr);
            return false;
        }

        RamsesObjectResolver ramsesResolver(m_errors, scene);

        std::unique_ptr<ApiObjects> deserializedObjects = ApiObjects::Deserialize(*logicEngine->apiObjects(), ramsesResolver, dataSourceDescription, m_errors);

        if (!deserializedObjects)
        {
            return false;
        }

        // No errors -> move data into member
        m_apiObjects = std::move(deserializedObjects);

        return true;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename)
    {
        m_errors.clear();

        if (!m_apiObjects->checkBindingsReferToSameRamsesScene(m_errors))
        {
            m_errors.add("Can't save a logic engine to file while it has references to more than one Ramses scene!", nullptr);
            return false;
        }

        // Refuse save() if logic graph has loops
        if (!m_apiObjects->getLogicNodeDependencies().getTopologicallySortedNodes())
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling saveToFile()!", nullptr);
            return false;
        }

        if (m_apiObjects->bindingsDirty())
        {
            LOG_WARN("Saving logic engine content with manually updated binding values without calling update() will result in those values being lost!");
        }

        flatbuffers::FlatBufferBuilder builder;
        ramses::RamsesVersion ramsesVersion = ramses::GetRamsesVersion();

        const auto ramsesVersionOffset = rlogic_serialization::CreateVersion(builder,
            ramsesVersion.major,
            ramsesVersion.minor,
            ramsesVersion.patch,
            builder.CreateString(ramsesVersion.string));
        builder.Finish(ramsesVersionOffset);

        const auto ramsesLogicVersionOffset = rlogic_serialization::CreateVersion(builder,
            g_PROJECT_VERSION_MAJOR,
            g_PROJECT_VERSION_MINOR,
            g_PROJECT_VERSION_PATCH,
            builder.CreateString(g_PROJECT_VERSION),
            g_FileFormatVersion);
        builder.Finish(ramsesLogicVersionOffset);

        const auto logicEngine = rlogic_serialization::CreateLogicEngine(builder,
            ramsesVersionOffset,
            ramsesLogicVersionOffset,
            ApiObjects::Serialize(*m_apiObjects, builder));
        builder.Finish(logicEngine);

        if (!FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize()))
        {
            m_errors.add(fmt::format("Failed to save content to path '{}'!", filename), nullptr);
            return false;
        }

        LOG_INFO("Saved logic engine to file: '{}'.", filename);

        return true;
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().unlink(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    ApiObjects& LogicEngineImpl::getApiObjects()
    {
        return *m_apiObjects;
    }

    void LogicEngineImpl::disableTrackingDirtyNodes()
    {
        m_nodeDirtyMechanismEnabled = false;
    }

    void LogicEngineImpl::enableUpdateReport(bool enable)
    {
        m_updateReportEnabled = enable;
        if (!m_updateReportEnabled)
            m_updateReport.clear();
    }

    LogicEngineReport LogicEngineImpl::getLastUpdateReport() const
    {
        return LogicEngineReport{ std::make_unique<LogicEngineReportImpl>(m_updateReport) };
    }

    template DataArray* LogicEngineImpl::createDataArray<float>(const std::vector<float>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec2f>(const std::vector<vec2f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec3f>(const std::vector<vec3f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec4f>(const std::vector<vec4f>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<int32_t>(const std::vector<int32_t>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec2i>(const std::vector<vec2i>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec3i>(const std::vector<vec3i>&, std::string_view name);
    template DataArray* LogicEngineImpl::createDataArray<vec4i>(const std::vector<vec4i>&, std::string_view name);
}
