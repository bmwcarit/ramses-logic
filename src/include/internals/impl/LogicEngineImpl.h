//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolState.h"

#include "internals/LogicNodeGraph.h"
#include "internals/LogicNodeConnector.h"
#include "internals/ErrorReporting.h"

#include "ramses-framework-api/RamsesFrameworkTypes.h"

#include <optional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace ramses
{
    class Scene;
    class SceneObject;
    class Node;
    class Appearance;
}

namespace rlogic
{
    class RamsesBinding;
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;
    class LuaScript;
    class LogicNode;
    class Property;
}

namespace rlogic_serialization
{
    struct LogicNode;
    struct Version;
}

namespace rlogic::internal
{
    class LogicNodeImpl;

    using ScriptsContainer = std::vector<std::unique_ptr<LuaScript>>;
    using NodeBindingsContainer = std::vector<std::unique_ptr<RamsesNodeBinding>>;
    using AppearanceBindingsContainer = std::vector<std::unique_ptr<RamsesAppearanceBinding>>;

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

        bool destroy(LogicNode& logicNode);


        LuaScript* createLuaScriptFromFile(std::string_view filename, std::string_view scriptName);
        LuaScript* createLuaScriptFromSource(std::string_view source, std::string_view scriptName);

        RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);
        RamsesAppearanceBinding* createRamsesAppearanceBinding(std::string_view name);

        ScriptsContainer& getScripts();
        NodeBindingsContainer& getNodeBindings();
        AppearanceBindingsContainer& getAppearanceBindings();

        bool                            update(bool disableDirtyTracking = false);
        const std::vector<std::string>& getErrors() const;

        bool loadFromFile(std::string_view filename, ramses::Scene* ramsesScene);
        bool saveToFile(std::string_view filename);

        bool link(const Property& sourceProperty, const Property& targetProperty);
        bool unlink(const Property& sourceProperty, const Property& targetProperty);

        bool isLinked(const LogicNode& logicNode) const;

        // For testing only
        const LogicNodeGraph& getLogicNodeGraph() const;
        const LogicNodeConnector& getLogicNodeConnector() const;

    private:
        SolState                            m_luaState;
        ErrorReporting                      m_errors;
        // TODO Sven move the containers to store instances to seperate resource manager
        ScriptsContainer                    m_scripts;
        NodeBindingsContainer               m_ramsesNodeBindings;
        AppearanceBindingsContainer         m_ramsesAppearanceBindings;
        std::vector<RamsesBinding*>         m_ramsesBindings;
        std::unordered_set<LogicNodeImpl*>  m_logicNodes;

        LogicNodeGraph                      m_logicNodeGraph;
        LogicNodeConnector                  m_logicNodeConnector;

        // TODO Violin redesign this; we have multiple places where we add/remove things from disconnectedNodes.
        // It feels like it's tightly coupled with m_logicNodeGraph and maybe worth checking if we can/should move the
        // logic to decide whether a node is disconnected there
        std::unordered_set<LogicNodeImpl*>  m_disconnectedNodes;

        LuaScript* createLuaScriptInternal(std::string_view source, std::string_view filename, std::string_view scriptName);
        void       setupLogicNodeInternal(LogicNode& logicNode);
        bool       destroyInternal(RamsesNodeBinding& ramsesNodeBinding);
        bool       destroyInternal(LuaScript& luaScript);
        bool       destroyInternal(RamsesAppearanceBinding& ramsesAppearanceBinding);

        void updateLinksRecursive(Property& inputProperty);

        ramses::SceneObject* findRamsesSceneObjectInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId);
        std::optional<ramses::Node*> findRamsesNodeInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId);
        std::optional<ramses::Appearance*> findRamsesAppearanceInScene(const rlogic_serialization::LogicNode* logicNode, ramses::Scene* scene, ramses::sceneObjectId_t objectId);

        static bool CheckLogicVersionFromFile(const rlogic_serialization::Version& version);
        static bool CheckRamsesVersionFromFile(const rlogic_serialization::Version& ramsesVersion);

        std::unordered_set<const LogicNodeImpl*> getAllLinkedLogicNodesOfInput(PropertyImpl& property);
        std::unordered_set<const LogicNodeImpl*> getAllLinkedLogicNodesOfOutput(PropertyImpl& property);

        void unlinkAll(LogicNode& logicNode);
    };
}
