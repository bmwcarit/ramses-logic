//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ImguiClientHelper.h"
#include "ramses-logic/Collection.h"

struct ImGuiSettingsHandler;

namespace ramses
{
    class Scene;
}

namespace rlogic
{
    class LogicEngine;
    class Property;
    class LogicObject;
    class LogicNode;
    class LogicViewer;
    class DataArray;

    class LogicViewerGui
    {
    public:
        explicit LogicViewerGui(rlogic::LogicViewer& viewer);
        void draw();
        void openErrorPopup(const std::string& message);
        void setSceneTexture(ramses::TextureSampler* sampler, uint32_t width, uint32_t height);

    private:
        void drawMenuItemShowWindow();
        void drawMenuItemReload();
        void drawMenuItemCopy();

        void drawGlobalContextMenu();
        void drawSceneTexture();
        void drawErrorPopup();
        void drawWindow();
        void drawMenuBar();
        void drawCurrentView();
        void drawInterfaces();
        void drawScripts();
        void drawAnimationNodes();
        void drawTimerNodes();
        void drawNodeBindings();
        void drawCameraBindings();
        void drawUpdateReport();
        void drawAppearanceBindings();
        void drawSaveDefaultLuaFile();

        static bool DrawTreeNode(rlogic::LogicObject* obj);

        void drawNodeContextMenu(rlogic::LogicNode* obj, const std::string_view& ns);
        void drawNode(rlogic::LogicNode* obj);
        void drawProperty(rlogic::Property* prop, size_t index);
        void drawOutProperty(const rlogic::Property* prop, size_t index);
        static void DrawDataArray(const rlogic::DataArray* obj, std::string_view context = std::string_view());

        void loadLuaFile(const std::string& filename);

        /**
         * copies all node inputs to clipboard (lua syntax)
         */
        template <class T>
        void copyInputs(const std::string_view& ns, Collection<T> collection);
        void copyScriptInputs();

        using PathVector = std::vector<std::string_view>;
        void logInputs(rlogic::LogicNode* obj, const PathVector& path);
        void logProperty(rlogic::Property* prop, const std::string& prefix, PathVector& path);

        static void* IniReadOpen(ImGuiContext* context, ImGuiSettingsHandler* handler, const char* name);
        static void IniReadLine(ImGuiContext* context, ImGuiSettingsHandler* handler, void* entry, const char* line);
        static void IniWriteAll(ImGuiContext* context, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);

        /**
         * saves a simple default lua configuration that can be used as a starting point
         */
        void saveDefaultLuaFile();

        struct Settings
        {
            bool showWindow         = true;
            bool showOutputs        = true;
            bool showLinkedInputs   = true;
            bool showInterfaces     = true;
            bool showScripts        = false;
            bool showAnimationNodes = false;
            bool showTimerNodes     = false;
            bool showDataArrays     = false;
            bool showRamsesBindings = false;
            bool showUpdateReport   = true;

            bool luaPreferObjectIds   = false;
            bool luaPreferIdentifiers = false;

            bool operator==(const Settings& rhs) const {
                return showWindow == rhs.showWindow && showOutputs == rhs.showOutputs && showLinkedInputs == rhs.showLinkedInputs && showInterfaces == rhs.showInterfaces &&
                       showScripts == rhs.showScripts && showAnimationNodes == rhs.showAnimationNodes && showTimerNodes == rhs.showTimerNodes &&
                       showDataArrays == rhs.showDataArrays && showRamsesBindings == rhs.showRamsesBindings && showUpdateReport == rhs.showUpdateReport &&
                       luaPreferObjectIds == rhs.luaPreferObjectIds && luaPreferIdentifiers == rhs.luaPreferIdentifiers;
            }

            bool operator!=(const Settings& rhs) const {
                return !operator==(rhs);
            }
        };

        rlogic::LogicViewer&       m_viewer;
        rlogic::LogicEngine&       m_logicEngine;

        Settings m_settings;
        Settings m_persistentSettings;

        std::string m_lastErrorMessage;
        std::string m_filename;
        size_t m_updateReportInterval = 60u;

        ramses::TextureSampler* m_sampler = nullptr;
        ImVec2 m_samplerSize;
    };
}

