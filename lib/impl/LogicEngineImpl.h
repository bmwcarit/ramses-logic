//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolState.h"

#include "internals/LogicNodeDependencies.h"
#include "internals/ErrorReporting.h"
#include "internals/ApiObjects.h"

#include "ramses-framework-api/RamsesFrameworkTypes.h"

#include <optional>
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
    class LuaScript;
    class LogicNode;
    class Property;
}

namespace rlogic_serialization
{
    struct Version;
}

namespace rlogic::internal
{
    class LogicNodeImpl;
    class RamsesBindingImpl;

    class LogicEngineImpl
    {
    public:
        // Move-able (noexcept); Not copy-able

        // can't be noexcept anymore because constructor of std::unordered_map can throw
        LogicEngineImpl() = default;
        ~LogicEngineImpl() noexcept = default;

        LogicEngineImpl(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl& operator=(LogicEngineImpl&& other) noexcept = default;
        LogicEngineImpl(const LogicEngineImpl& other)                = delete;
        LogicEngineImpl& operator=(const LogicEngineImpl& other) = delete;

        // Public API
        LuaScript* createLuaScriptFromFile(std::string_view filename, std::string_view scriptName);
        LuaScript* createLuaScriptFromSource(std::string_view source, std::string_view scriptName);
        RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);
        RamsesAppearanceBinding* createRamsesAppearanceBinding(std::string_view name);
        RamsesCameraBinding* createRamsesCameraBinding(std::string_view name);

        bool destroy(LogicNode& logicNode);

        bool                            update(bool disableDirtyTracking = false);
        const std::vector<ErrorData>&   getErrors() const;

        bool loadFromFile(std::string_view filename, ramses::Scene* scene, bool enableMemoryVerification);
        bool loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* scene, bool enableMemoryVerification);
        bool saveToFile(std::string_view filename);

        bool link(const Property& sourceProperty, const Property& targetProperty);
        bool unlink(const Property& sourceProperty, const Property& targetProperty);

        [[nodiscard]] bool isLinked(const LogicNode& logicNode) const;

        // Internally used
        [[nodiscard]] bool isDirty() const;
        [[nodiscard]] bool bindingsDirty() const;

        [[nodiscard]] ApiObjects& getApiObjects();
        [[nodiscard]] const ApiObjects& getApiObjects() const;

    private:
        SolState m_luaState;
        ApiObjects m_apiObjects;
        ErrorReporting m_errors;

        void updateLinksRecursive(Property& inputProperty);

        static bool CheckLogicVersionFromFile(const rlogic_serialization::Version& version);
        static bool CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion);

        [[nodiscard]] bool updateLogicNodeInternal(LogicNodeImpl& node, bool disableDirtyTracking);

        [[nodiscard]] bool loadFromByteData(const void* byteData, size_t byteSize, ramses::Scene* scene, bool enableMemoryVerification, const std::string& dataSourceDescription);
    };
}
