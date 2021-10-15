//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/ERotationType.h"

#include "impl/LuaConfigImpl.h"

#include "internals/LuaCompilationUtils.h"
#include "internals/SolState.h"
#include "internals/LogicNodeDependencies.h"

#include <vector>
#include <memory>
#include <string_view>

namespace ramses
{
    class Scene;
    class Node;
    class Appearance;
    class Camera;
}

namespace rlogic
{
    class LogicObject;
    class LogicNode;
    class LuaScript;
    class LuaModule;
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;
    class RamsesCameraBinding;
    class DataArray;
    class AnimationNode;
}

namespace rlogic_serialization
{
    struct ApiObjects;
}

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic::internal
{
    class SolState;
    class IRamsesObjectResolver;

    using ScriptsContainer = std::vector<std::unique_ptr<LuaScript>>;
    using LuaModulesContainer = std::vector<std::unique_ptr<LuaModule>>;
    using NodeBindingsContainer = std::vector<std::unique_ptr<RamsesNodeBinding>>;
    using AppearanceBindingsContainer = std::vector<std::unique_ptr<RamsesAppearanceBinding>>;
    using CameraBindingsContainer = std::vector<std::unique_ptr<RamsesCameraBinding>>;
    using DataArrayContainer = std::vector<std::unique_ptr<DataArray>>;
    using AnimationNodesContainer = std::vector<std::unique_ptr<AnimationNode>>;

    class ApiObjects
    {
    public:
        // Not move-able and non-copyable
        ApiObjects();
        ~ApiObjects() noexcept;
        // Not move-able because of the dependency between sol objects and their parent sol state
        // Moving those would require a custom move assignment operator which keeps both sol states alive
        // until the objects have been moved, and only then also moves the sol state - we don't need this
        // complexity because we never move-assign ApiObjects
        ApiObjects(ApiObjects&& other) noexcept = delete;
        ApiObjects& operator=(ApiObjects&& other) noexcept = delete;
        ApiObjects(const ApiObjects& other) = delete;
        ApiObjects& operator=(const ApiObjects& other) = delete;

        // Serialization/Deserialization
        static flatbuffers::Offset<rlogic_serialization::ApiObjects> Serialize(const ApiObjects& apiObjects, flatbuffers::FlatBufferBuilder& builder);
        static std::unique_ptr<ApiObjects> Deserialize(
            const rlogic_serialization::ApiObjects& apiObjects,
            const IRamsesObjectResolver& ramsesResolver,
            const std::string& dataSourceDescription,
            ErrorReporting& errorReporting);

        // Create/destroy API objects
        LuaScript* createLuaScript(
            std::string_view source,
            const LuaConfigImpl& config,
            std::string_view scriptName,
            ErrorReporting& errorReporting);
        LuaModule* createLuaModule(
            std::string_view source,
            const LuaConfigImpl& config,
            std::string_view moduleName,
            ErrorReporting& errorReporting);
        RamsesNodeBinding* createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType, std::string_view name);
        RamsesAppearanceBinding* createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name);
        RamsesCameraBinding* createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name);
        template <typename T>
        DataArray* createDataArray(const std::vector<T>& data, std::string_view name);
        AnimationNode* createAnimationNode(const AnimationChannels& channels, std::string_view name);
        bool destroy(LogicObject& object, ErrorReporting& errorReporting);

        // Invariance checks
        [[nodiscard]] bool checkBindingsReferToSameRamsesScene(ErrorReporting& errorReporting) const;

        // Getters
        [[nodiscard]] ScriptsContainer& getScripts();
        [[nodiscard]] const ScriptsContainer& getScripts() const;
        [[nodiscard]] LuaModulesContainer& getLuaModules();
        [[nodiscard]] const LuaModulesContainer& getLuaModules() const;
        [[nodiscard]] NodeBindingsContainer& getNodeBindings();
        [[nodiscard]] const NodeBindingsContainer& getNodeBindings() const;
        [[nodiscard]] AppearanceBindingsContainer& getAppearanceBindings();
        [[nodiscard]] const AppearanceBindingsContainer& getAppearanceBindings() const;
        [[nodiscard]] CameraBindingsContainer& getCameraBindings();
        [[nodiscard]] const CameraBindingsContainer& getCameraBindings() const;
        [[nodiscard]] DataArrayContainer& getDataArrays();
        [[nodiscard]] const DataArrayContainer& getDataArrays() const;
        [[nodiscard]] AnimationNodesContainer& getAnimationNodes();
        [[nodiscard]] const AnimationNodesContainer& getAnimationNodes() const;

        [[nodiscard]] const LogicNodeDependencies& getLogicNodeDependencies() const;
        [[nodiscard]] LogicNodeDependencies& getLogicNodeDependencies();

        [[nodiscard]] LogicNode* getApiObject(LogicNodeImpl& impl) const;

        // Internally used
        [[nodiscard]] bool isDirty() const;
        [[nodiscard]] bool bindingsDirty() const;

        // Strictly for testing purposes (inverse mappings require extra attention and test coverage)
        [[nodiscard]] const std::unordered_map<LogicNodeImpl*, LogicNode*>& getReverseImplMapping() const;

    private:
        // Handle internal data structures and mappings
        void registerLogicNode(LogicNode& logicNode);
        void unregisterLogicNode(LogicNode& logicNode);

        bool checkLuaModules(
            const ModuleMapping& moduleMapping,
            ErrorReporting& errorReporting);

        // Type-specific destruction logic
        [[nodiscard]] bool destroyInternal(RamsesNodeBinding& ramsesNodeBinding, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(LuaScript& luaScript, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(LuaModule& luaModule, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(RamsesCameraBinding& ramsesCameraBinding, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(AnimationNode& node, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(DataArray& dataArray, ErrorReporting& errorReporting);

        std::unique_ptr<SolState>           m_solState {std::make_unique<SolState>()};

        ScriptsContainer                    m_scripts;
        LuaModulesContainer                 m_luaModules;
        NodeBindingsContainer               m_ramsesNodeBindings;
        AppearanceBindingsContainer         m_ramsesAppearanceBindings;
        CameraBindingsContainer             m_ramsesCameraBindings;
        DataArrayContainer                  m_dataArrays;
        AnimationNodesContainer             m_animationNodes;
        LogicNodeDependencies               m_logicNodeDependencies;
        std::unordered_map<LogicNodeImpl*, LogicNode*> m_reverseImplMapping;
    };
}
