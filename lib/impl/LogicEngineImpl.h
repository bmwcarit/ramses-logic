//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/ERotationType.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/LogicEngineReport.h"
#include "internals/LogicNodeDependencies.h"
#include "internals/ErrorReporting.h"
#include "internals/UpdateReport.h"

#include "ramses-framework-api/RamsesFrameworkTypes.h"

#include <memory>
#include <vector>
#include <string>
#include <string_view>

namespace ramses
{
    class Scene;
    class SceneObject;
    class Node;
    class Appearance;
    class Camera;
}

namespace rlogic
{
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;
    class RamsesCameraBinding;
    class DataArray;
    class AnimationNode;
    class TimerNode;
    class LuaScript;
    class LuaModule;
    class LogicNode;
    class Property;
}

namespace rlogic_serialization
{
    struct Version;
}

namespace rlogic::internal
{
    class LuaConfigImpl;
    class LogicNodeImpl;
    class RamsesBindingImpl;
    class ApiObjects;

    class LogicEngineImpl
    {
    public:
        // Move-able (noexcept); Not copy-able

        // can't be noexcept anymore because constructor of std::unordered_map can throw
        LogicEngineImpl();
        ~LogicEngineImpl() noexcept;

        LogicEngineImpl(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl& operator=(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl(const LogicEngineImpl& other)                = delete;
        LogicEngineImpl& operator=(const LogicEngineImpl& other) = delete;

        // Public API
        LuaScript* createLuaScript(std::string_view source, const LuaConfigImpl& config, std::string_view scriptName);
        LuaModule* createLuaModule(std::string_view source, const LuaConfigImpl& config, std::string_view moduleName);
        bool extractLuaDependencies(std::string_view source, const std::function<void(const std::string&)>& callbackFunc);
        RamsesNodeBinding* createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name);
        RamsesAppearanceBinding* createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name);
        RamsesCameraBinding* createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name);
        template <typename T>
        DataArray* createDataArray(const std::vector<T>& data, std::string_view name);
        AnimationNode* createAnimationNode(const AnimationChannels& channels, std::string_view name);
        TimerNode* createTimerNode(std::string_view name);

        bool destroy(LogicObject& object);

        bool update();
        [[nodiscard]] const std::vector<ErrorData>& getErrors() const;

        bool loadFromFile(std::string_view filename, ramses::Scene* scene, bool enableMemoryVerification);
        bool loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* scene, bool enableMemoryVerification);
        bool saveToFile(std::string_view filename);

        bool link(const Property& sourceProperty, const Property& targetProperty);
        bool unlink(const Property& sourceProperty, const Property& targetProperty);

        [[nodiscard]] bool isLinked(const LogicNode& logicNode) const;

        [[nodiscard]] ApiObjects& getApiObjects();

        // for benchmarking purposes only
        void disableTrackingDirtyNodes();

        void enableUpdateReport(bool enable);
        [[nodiscard]] LogicEngineReport getLastUpdateReport() const;

    private:
        size_t activateLinksRecursive(PropertyImpl& output);

        bool checkLogicVersionFromFile(std::string_view dataSourceDescription, uint32_t fileVersion);
        static bool CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion);

        [[nodiscard]] bool updateNodes(const NodeVector& nodes);

        [[nodiscard]] bool loadFromByteData(const void* byteData, size_t byteSize, ramses::Scene* scene, bool enableMemoryVerification, const std::string& dataSourceDescription);

        std::unique_ptr<ApiObjects> m_apiObjects;
        ErrorReporting m_errors;
        bool m_nodeDirtyMechanismEnabled = true;

        bool m_updateReportEnabled = false;
        UpdateReport m_updateReport;
    };
}
