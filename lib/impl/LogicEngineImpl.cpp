//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LogicEngineImpl.h"
#include "impl/LuaScriptImpl.h"

#include "impl/LoggerImpl.h"
#include "internals/FileUtils.h"
#include "internals/TypeUtils.h"
#include "internals/RamsesObjectResolver.h"

// TODO Violin remove these header dependencies
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "ramses-logic-build-config.h"
#include "generated/LogicEngineGen.h"

#include "ramses-framework-api/RamsesVersion.h"

#include <string>
#include <fstream>
#include <streambuf>

#include "fmt/format.h"

namespace rlogic::internal
{
    LuaScript* LogicEngineImpl::createLuaScriptFromFile(std::string_view filename, std::string_view scriptName)
    {
        m_errors.clear();

        std::ifstream iStream;
        // ifstream has no support for string_view
        iStream.open(std::string(filename), std::ios_base::in);
        if (iStream.fail())
        {
            m_errors.add("Failed opening file " + std::string(filename) + "!");
            return nullptr;
        }

        std::string source(std::istreambuf_iterator<char>(iStream), std::istreambuf_iterator<char>{});
        return m_apiObjects.createLuaScript(m_luaState, source, filename, scriptName, m_errors);
    }

    LuaScript* LogicEngineImpl::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        m_errors.clear();
        return m_apiObjects.createLuaScript(m_luaState, source, "", scriptName, m_errors);
    }

    RamsesNodeBinding* LogicEngineImpl::createRamsesNodeBinding(ramses::Node& ramsesNode, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesNodeBinding(ramsesNode, name);
    }

    RamsesAppearanceBinding* LogicEngineImpl::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesAppearanceBinding(ramsesAppearance, name);
    }

    RamsesCameraBinding* LogicEngineImpl::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        m_errors.clear();
        return m_apiObjects.createRamsesCameraBinding(ramsesCamera, name);
    }

    bool LogicEngineImpl::destroy(LogicNode& logicNode)
    {
        m_errors.clear();
        return m_apiObjects.destroy(logicNode, m_errors);
    }

    bool LogicEngineImpl::isLinked(const LogicNode& logicNode) const
    {
        return m_apiObjects.getLogicNodeDependencies().isLinked(logicNode.m_impl);
    }

    void LogicEngineImpl::updateLinksRecursive(Property& inputProperty)
    {
        // TODO Violin consider providing property iterator - seems it's needed quite often, would make access more convenient also for users
        const auto inputCount = inputProperty.getChildCount();
        for (size_t i = 0; i < inputCount; ++i)
        {
            Property& child = *inputProperty.getChild(i);

            if (TypeUtils::CanHaveChildren(child.getType()))
            {
                updateLinksRecursive(child);
            }
            else
            {
                const PropertyImpl* output = m_apiObjects.getLogicNodeDependencies().getLinkedOutput(*child.m_impl);
                if (nullptr != output)
                {
                    child.m_impl->setValue(output->getValue(), true);
                }
            }
        }
    }

    bool LogicEngineImpl::update(bool disableDirtyTracking)
    {
        m_errors.clear();
        LOG_DEBUG("Begin update");

        const std::optional<NodeVector> sortedNodes = m_apiObjects.getLogicNodeDependencies().getTopologicallySortedNodes();
        if (!sortedNodes)
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling update()!");
            return false;
        }

        for (LogicNodeImpl* logicNode : *sortedNodes)
        {
            if (!updateLogicNodeInternal(*logicNode, disableDirtyTracking))
            {
                return false;
            }
        }

        return true;
    }

    bool LogicEngineImpl::updateLogicNodeInternal(LogicNodeImpl& node, bool disableDirtyTracking)
    {
        updateLinksRecursive(*node.getInputs());
        if (disableDirtyTracking || node.isDirty())
        {
            LOG_DEBUG("Updating LogicNode '{}'", node.getName());
            const std::optional<LogicNodeRuntimeError> potentialError = node.update();
            if (potentialError)
            {
                m_errors.add(potentialError->message, *m_apiObjects.getApiObject(node));
                return false;
            }
        }
        else
        {
            LOG_DEBUG("Skip update of LogicNode '{}' because no input has changed since the last update", node.getName());
        }
        node.setDirty(false);
        return true;
    }

    const std::vector<ErrorData>& LogicEngineImpl::getErrors() const
    {
        return m_errors.getErrors();
    }

    bool LogicEngineImpl::CheckLogicVersionFromFile(const rlogic_serialization::Version& version)
    {
        // TODO Violin/Tobias/Sven should discuss to what do we couple the serialization
        // compatibility check and when do we mandate version bump
        // For now, to be safe, any non-patch version change results in incompatibility
        return
            version.v_major() == g_PROJECT_VERSION_MAJOR &&
            version.v_minor() == g_PROJECT_VERSION_MINOR;
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
            m_errors.add(fmt::format("Failed to load file '{}'", filename));
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
                m_errors.add(fmt::format("{} contains corrupted data!", dataSourceDescription));
                return false;
            }
        }

        const auto* logicEngine = rlogic_serialization::GetLogicEngine(byteData);

        if (nullptr == logicEngine)
        {
            m_errors.add(fmt::format("{} doesn't contain logic engine data with readable version specifiers", dataSourceDescription));
            return false;
        }

        const auto& ramsesVersion = *logicEngine->ramsesVersion();
        if (!CheckRamsesVersionFromFile(ramsesVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected Ramses version {}.x.x but found {}",
                dataSourceDescription, ramses::GetRamsesVersion().major,
                ramsesVersion.v_string()->string_view()));
            return false;
        }

        const auto& rlogicVersion = *logicEngine->rlogicVersion();
        if (!CheckLogicVersionFromFile(rlogicVersion))
        {
            m_errors.add(fmt::format("Version mismatch while loading {}! Expected version {}.{}.x but found {}",
                dataSourceDescription, g_PROJECT_VERSION_MAJOR, g_PROJECT_VERSION_MINOR,
                rlogicVersion.v_string()->string_view()));
            return false;
        }

        if (nullptr == logicEngine->apiObjects())
        {
            m_errors.add(fmt::format("Fatal error while loading {}: doesn't contain API objects!", dataSourceDescription));
            return false;
        }

        RamsesObjectResolver ramsesResolver(m_errors, scene);

        // TODO Violin also use fresh Lua environment, so that we don't pollute current one when loading failed
        std::optional<ApiObjects> deserializedObjects = ApiObjects::Deserialize(m_luaState, *logicEngine->apiObjects(), ramsesResolver, dataSourceDescription, m_errors);

        if (!deserializedObjects)
        {
            return false;
        }

        // No errors -> move data into member
        m_apiObjects = std::move(*deserializedObjects);

        return true;
    }

    bool LogicEngineImpl::saveToFile(std::string_view filename)
    {
        m_errors.clear();

        if (!m_apiObjects.checkBindingsReferToSameRamsesScene(m_errors))
        {
            m_errors.add("Can't save a logic engine to file while it has references to more than one Ramses scene!");
            return false;
        }

        // Refuse save() if logic graph has loops
        if (!m_apiObjects.getLogicNodeDependencies().getTopologicallySortedNodes())
        {
            m_errors.add("Failed to sort logic nodes based on links between their properties. Create a loop-free link graph before calling saveToFile()!");
            return false;
        }

        if (m_apiObjects.bindingsDirty())
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
            builder.CreateString(g_PROJECT_VERSION));
        builder.Finish(ramsesLogicVersionOffset);

        const auto logicEngine = rlogic_serialization::CreateLogicEngine(builder,
            ramsesVersionOffset,
            ramsesLogicVersionOffset,
            ApiObjects::Serialize(m_apiObjects, builder));
        builder.Finish(logicEngine);

        if (!FileUtils::SaveBinary(std::string(filename), builder.GetBufferPointer(), builder.GetSize()))
        {
            m_errors.add(fmt::format("Failed to save content to path '{}'!", filename));
            return false;
        }

        return true;
    }

    bool LogicEngineImpl::link(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects.getLogicNodeDependencies().link(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    bool LogicEngineImpl::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        m_errors.clear();

        return m_apiObjects.getLogicNodeDependencies().unlink(*sourceProperty.m_impl, *targetProperty.m_impl, m_errors);
    }

    ApiObjects& LogicEngineImpl::getApiObjects()
    {
        return m_apiObjects;
    }

    const ApiObjects& LogicEngineImpl::getApiObjects() const
    {
        return m_apiObjects;
    }
}
