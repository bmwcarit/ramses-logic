//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "LogicNodeDependencies.h"

#include <vector>
#include <memory>
#include <string_view>

namespace rlogic
{
    class LogicNode;
    class LuaScript;
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;
    class RamsesCameraBinding;
}

namespace rlogic::internal
{
    class SolState;

    using ScriptsContainer = std::vector<std::unique_ptr<LuaScript>>;
    using NodeBindingsContainer = std::vector<std::unique_ptr<RamsesNodeBinding>>;
    using AppearanceBindingsContainer = std::vector<std::unique_ptr<RamsesAppearanceBinding>>;
    using CameraBindingsContainer = std::vector<std::unique_ptr<RamsesCameraBinding>>;

    class ApiObjects
    {
    public:
        // Moveable, non-copyable, can be constructed from deserialized data
        ApiObjects() = default;
        ApiObjects(ScriptsContainer scripts, NodeBindingsContainer ramsesNodeBindings, AppearanceBindingsContainer ramsesAppearanceBindings,
            CameraBindingsContainer ramsesCameraBindings, LogicNodeDependencies logicNodeDependencies);
        ~ApiObjects() noexcept = default;
        // TODO Violin try to find a way to make the class move-able without exceptions (and also the whole LogicEngine)
        // Currently not possible because MSVC2017 compiler forces copy on move: https://stackoverflow.com/questions/47604029/move-constructors-of-stl-containers-in-msvc-2017-are-not-marked-as-noexcept
        ApiObjects(ApiObjects&& other) = default;
        ApiObjects& operator=(ApiObjects&& other) = default;
        ApiObjects(const ApiObjects& other) = delete;
        ApiObjects& operator=(const ApiObjects& other) = delete;

        // Create/destroy API
        LuaScript* createLuaScript(SolState& solState, std::string_view source, std::string_view filename, std::string_view scriptName, ErrorReporting& errorReporting);
        RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);
        RamsesAppearanceBinding* createRamsesAppearanceBinding(std::string_view name);
        RamsesCameraBinding* createRamsesCameraBinding(std::string_view name);
        bool destroy(LogicNode& logicNode, ErrorReporting& errorReporting);

        // Invariance checks
        [[nodiscard]] bool checkBindingsReferToSameRamsesScene(ErrorReporting& errorReporting) const;

        // Getters
        [[nodiscard]] ScriptsContainer& getScripts();
        [[nodiscard]] const ScriptsContainer& getScripts() const;
        [[nodiscard]] NodeBindingsContainer& getNodeBindings();
        [[nodiscard]] const NodeBindingsContainer& getNodeBindings() const;
        [[nodiscard]] AppearanceBindingsContainer& getAppearanceBindings();
        [[nodiscard]] const AppearanceBindingsContainer& getAppearanceBindings() const;
        [[nodiscard]] CameraBindingsContainer& getCameraBindings();
        [[nodiscard]] const CameraBindingsContainer& getCameraBindings() const;

        [[nodiscard]] const LogicNodeDependencies& getLogicNodeDependencies() const;
        [[nodiscard]] LogicNodeDependencies& getLogicNodeDependencies();

        [[nodiscard]] LogicNode* getApiObject(LogicNodeImpl& impl) const;

        // Strictly for testing purposes (inverse mappings require extra attention and test coverage)
        [[nodiscard]] const std::unordered_map<LogicNodeImpl*, LogicNode*>& getReverseImplMapping() const;

    private:
        // Handle internal data structures and mappings
        void registerLogicNode(LogicNode& logicNode);
        void unregisterLogicNode(LogicNode& logicNode);

        // Type-specific destruction logic
        [[nodiscard]] bool destroyInternal(RamsesNodeBinding& ramsesNodeBinding, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(LuaScript& luaScript, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding, ErrorReporting& errorReporting);
        [[nodiscard]] bool destroyInternal(RamsesCameraBinding& ramsesCameraBinding, ErrorReporting& errorReporting);

        ScriptsContainer                    m_scripts;
        NodeBindingsContainer               m_ramsesNodeBindings;
        AppearanceBindingsContainer         m_ramsesAppearanceBindings;
        CameraBindingsContainer             m_ramsesCameraBindings;
        LogicNodeDependencies               m_logicNodeDependencies;

        std::unordered_map<LogicNodeImpl*, LogicNode*> m_reverseImplMapping;
    };
}
