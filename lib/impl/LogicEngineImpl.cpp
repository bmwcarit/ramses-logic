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
#include "ramses-logic/AnimationNodeConfig.h"

#include "impl/LogicNodeImpl.h"
#include "impl/LoggerImpl.h"
#include "impl/LuaModuleImpl.h"
#include "impl/LuaConfigImpl.h"
#include "impl/SaveFileConfigImpl.h"
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

    rlogic::AnimationNode* LogicEngineImpl::createAnimationNode(const AnimationNodeConfig& config, std::string_view name)
    {
        m_errors.clear();

        auto containsDataArray = [this](const DataArray* da) {
            const auto& dataArrays = m_apiObjects->getApiObjectContainer<DataArray>();
            const auto it = std::find_if(dataArrays.cbegin(), dataArrays.cend(),
                [da](const auto& d) { return d == da; });
            return it != dataArrays.cend();
        };

        if (config.getChannels().empty())
        {
            m_errors.add(fmt::format("Failed to create AnimationNode '{}': must provide at least one channel.", name), nullptr);
            return nullptr;
        }

        for (const auto& channel : config.getChannels())
        {
            if (!containsDataArray(channel.timeStamps) ||
                !containsDataArray(channel.keyframes))
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': timestamps or keyframes were not found in this logic instance.", name), nullptr);
                return nullptr;
            }

            if ((channel.tangentsIn && !containsDataArray(channel.tangentsIn)) ||
                (channel.tangentsOut && !containsDataArray(channel.tangentsOut)))
            {
                m_errors.add(fmt::format("Failed to create AnimationNode '{}': tangents were not found in this logic instance.", name), nullptr);
                return nullptr;
            }
        }

        return m_apiObjects->createAnimationNode(*config.m_impl, name);
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
                const auto& outgoingLinks = child.getOutgoingLinks();
                for (const auto& outLink : outgoingLinks)
                {
                    PropertyImpl* linkedProp = outLink.property;
                    const bool valueChanged = linkedProp->setValue(child.getValue());
                    if (valueChanged || linkedProp->getPropertySemantics() == EPropertySemantics::AnimationInput)
                    {
                        linkedProp->getLogicNode().setDirty(true);
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

        if (m_statisticsEnabled || m_updateReportEnabled)
        {
            m_updateReport.clear();
            m_updateReport.sectionStarted(UpdateReport::ETimingSection::TotalUpdate);
        }
        if (m_updateReportEnabled)
        {
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

        // force dirty all timer nodes and their dependents so they update their tickers
        setTimerNodesAndDependentsDirty();

        const bool success = updateNodes(*sortedNodes);

        if (m_statisticsEnabled || m_updateReportEnabled)
        {
            m_updateReport.sectionFinished(UpdateReport::ETimingSection::TotalUpdate);
            m_statistics.collect(m_updateReport, sortedNodes->size());
            if (m_statistics.checkUpdateFrameFinished())
                m_statistics.calculateAndLog();
        }

        return success;
    }

    bool LogicEngineImpl::updateNodes(const NodeVector& sortedNodes)
    {
        for (LogicNodeImpl* nodeIter : sortedNodes)
        {
            LogicNodeImpl& node = *nodeIter;

            if (!node.isDirty())
            {
                if (m_updateReportEnabled)
                    m_updateReport.nodeSkippedExecution(node);

                if(m_nodeDirtyMechanismEnabled)
                    continue;
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionStarted(node);
            if (m_statisticsEnabled)
                m_statistics.nodeExecuted();

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

                if (m_statisticsEnabled || m_updateReportEnabled)
                    m_updateReport.linksActivated(activatedLinks);
            }

            if (m_updateReportEnabled)
                m_updateReport.nodeExecutionFinished();

            node.setDirty(false);
        }

        return true;
    }

    void LogicEngineImpl::setTimerNodesAndDependentsDirty()
    {
        for (TimerNode* timerNode : m_apiObjects->getApiObjectContainer<TimerNode>())
        {
            // force set timer node itself dirty so it can update its ticker
            timerNode->m_impl.setDirty(true);

            // force set all nodes linked to timeDelta dirty (timeDelta is often constant but that does not mean update is not needed)
            auto timeDeltaOutput = timerNode->getOutputs()->getChild(0u);
            assert(timeDeltaOutput && timeDeltaOutput->getName() == "timeDelta");
            const auto& outgoingLinks = timeDeltaOutput->m_impl->getOutgoingLinks();
            for (const auto& outLink : outgoingLinks)
                outLink.property->getLogicNode().setDirty(true);
        }
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

        LOG_INFO("Loading logic engine content from '{}' which was exported with Ramses {} and Logic Engine {}", dataSourceDescription, ramsesVersion.v_string()->string_view(), rlogicVersion.v_string()->string_view());

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

        if (logicEngine->assetMetedata())
        {
            LogAssetMetadata(*logicEngine->assetMetedata());
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

    bool LogicEngineImpl::saveToFile(std::string_view filename, const SaveFileConfigImpl& config)
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

        const auto exporterVersionOffset = rlogic_serialization::CreateVersion(builder,
            config.getExporterMajorVersion(),
            config.getExporterMinorVersion(),
            config.getExporterPatchVersion(),
            builder.CreateString(""),
            config.getExporterFileFormatVersion());
        builder.Finish(exporterVersionOffset);

        const auto assetMetadataOffset = rlogic_serialization::CreateMetadata(builder,
            builder.CreateString(config.getMetadataString()),
            exporterVersionOffset);
        builder.Finish(assetMetadataOffset);

        const auto logicEngine = rlogic_serialization::CreateLogicEngine(builder,
            ramsesVersionOffset,
            ramsesLogicVersionOffset,
            ApiObjects::Serialize(*m_apiObjects, builder),
            assetMetadataOffset);
        builder.Finish(logicEngine);

        if (!FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize()))
        {
            m_errors.add(fmt::format("Failed to save content to path '{}'!", filename), nullptr);
            return false;
        }

        LOG_INFO("Saved logic engine to file: '{}'.", filename);

        return true;
    }

    void LogicEngineImpl::LogAssetMetadata(const rlogic_serialization::Metadata& assetMetadata)
    {
        const std::string_view metadataString = assetMetadata.metadataString() ? assetMetadata.metadataString()->string_view() : "none";
        LOG_INFO("Logic Engine content metadata: '{}'", metadataString);
        const std::string exporterVersion = assetMetadata.exporterVersion() ?
            fmt::format("{}.{}.{} (file format version {})",
                assetMetadata.exporterVersion()->v_major(),
                assetMetadata.exporterVersion()->v_minor(),
                assetMetadata.exporterVersion()->v_patch(),
                assetMetadata.exporterVersion()->v_fileFormatVersion()) : "undefined";
        LOG_INFO("Exporter version: {}", exporterVersion);
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, false, m_errors);
    }

    bool LogicEngineImpl::linkWeak(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects->getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, true, m_errors);
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
        return LogicEngineReport{ std::make_unique<LogicEngineReportImpl>(m_updateReport, *m_apiObjects) };
    }

    void LogicEngineImpl::setStatisticsLoggingRate(size_t loggingRate)
    {
        m_statistics.setLoggingRate(loggingRate);
        m_statisticsEnabled = (loggingRate != 0u);
    }

    void LogicEngineImpl::setStatisticsLogLevel(ELogMessageType logLevel)
    {
        m_statistics.setLogLevel(logLevel);
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
