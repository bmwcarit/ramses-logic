//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicViewerGui.h"
#include "LogicViewer.h"
#include "ramses-client-api/Scene.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/Property.h"
#include "internals/StdFilesystemWrapper.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "imgui_internal.h" // for ImGuiSettingsHandler
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

namespace rlogic
{
    namespace
    {
        const char* EnumToString(ERotationType t)
        {
            switch (t)
            {
            case ERotationType::Euler_ZYX:
                return "Euler_ZYX";
            case ERotationType::Euler_YZX:
                return "Euler_YZX";
            case ERotationType::Euler_ZXY:
                return "Euler_ZXY";
            case ERotationType::Euler_XZY:
                return "Euler_XZY";
            case ERotationType::Euler_YXZ:
                return "Euler_YXZ";
            case ERotationType::Euler_XYZ:
                return "Euler_XYZ";
            case ERotationType::Quaternion:
                return "Quaternion";
            }
            return "";
        }

        const char* EnumToString(EInterpolationType t)
        {
            switch (t)
            {
            case EInterpolationType::Step:
                return "Step";
            case EInterpolationType::Linear:
                return "Linear";
            case EInterpolationType::Cubic:
                return "Cubic";
            case EInterpolationType::Linear_Quaternions:
                return "Linear_Quaternions";
            case EInterpolationType::Cubic_Quaternions:
                return "Cubic_Quaternions";
            }
            return "";
        }

        bool IniReadFlag(const char* line, const char* fmt, int* flag)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, cert-err34-c) no suitable replacement
            return (sscanf(line, fmt, flag) == 1);
        }

        void LogText(const std::string& text)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
            ImGui::LogText("%s", text.c_str());
        }

        bool TreeNode(const void* ptr_id, const std::string_view& text)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
            return ImGui::TreeNode(ptr_id, "%s", text.data());
        }

        bool TreeNode(const void* ptr_id, const std::string& text)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
            return ImGui::TreeNode(ptr_id, "%s", text.c_str());
        }
    }

    LogicViewerGui::LogicViewerGui(rlogic::LogicViewer& viewer)
        : m_viewer(viewer)
        , m_logicEngine(viewer.getEngine())
    {
        auto* ctx = ImGui::GetCurrentContext();
        ImGuiSettingsHandler ini_handler;
        ini_handler.TypeName = "LogicViewerGui";
        ini_handler.TypeHash = ImHashStr("LogicViewerGui");
        ini_handler.ReadOpenFn = IniReadOpen;
        ini_handler.ReadLineFn = IniReadLine;
        ini_handler.WriteAllFn = IniWriteAll;
        ini_handler.UserData   = this;
        ctx->SettingsHandlers.push_back(ini_handler);

        ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
        m_persistentSettings = m_settings;

        // filename proposal if there is no lua file found at startup
        fs::path p = m_viewer.getLogicFilename();
        p.replace_extension("lua");
        m_filename = p.string();
    }

    void LogicViewerGui::draw()
    {
        if (m_settings != m_persistentSettings)
        {
            ImGui::MarkIniSettingsDirty();
            m_persistentSettings = m_settings;
        }

        if (ImGui::IsKeyPressed(ramses::EKeyCode_Left))
        {
            m_viewer.setCurrentView(m_viewer.getCurrentView() - 1);
        }
        else if (ImGui::IsKeyPressed(ramses::EKeyCode_Right))
        {
            m_viewer.setCurrentView(m_viewer.getCurrentView() + 1);
        }
        else if (ImGui::IsKeyPressed(ramses::EKeyCode_F11))
        {
            m_settings.showWindow = !m_settings.showWindow;
        }
        else if (ImGui::IsKeyPressed(ramses::EKeyCode_F5))
        {
            loadLuaFile(m_viewer.getLuaFilename());
        }
        else if (ImGui::IsKeyPressed(ramses::EKeyCode_C) && ImGui::GetIO().KeyCtrl)
        {
            copyScriptInputs();
        }
        else
        {
        }

        drawGlobalContextMenu();
        drawSceneTexture();
        drawErrorPopup();

        if (m_settings.showWindow)
        {
            // ImGui::ShowDemoWindow();
            drawWindow();
        }
    }

    void LogicViewerGui::openErrorPopup(const std::string& message)
    {
        // ImGui::OpenPopup("Error") does not work in all cases (The calculated ID seems to be context related)
        // The popup will be opened in LogicViewerGui::draw() instead
        m_lastErrorMessage = message;
    }

    void LogicViewerGui::setSceneTexture(ramses::TextureSampler* sampler, uint32_t width, uint32_t height)
    {
        m_sampler       = sampler;
        m_samplerSize.x = static_cast<float>(width);
        m_samplerSize.y = static_cast<float>(height);
    }

    void LogicViewerGui::drawMenuItemShowWindow()
    {
        ImGui::MenuItem("Show Logic Viewer Window", "F11", &m_settings.showWindow);
    }

    void LogicViewerGui::drawMenuItemReload()
    {
        if (ImGui::MenuItem("Reload configuration", "F5"))
        {
            loadLuaFile(m_viewer.getLuaFilename());
        }
    }

    void LogicViewerGui::drawMenuItemCopy()
    {
        if (ImGui::MenuItem("Copy script inputs", "Ctrl+C"))
        {
            copyScriptInputs();
        }
    }

    void LogicViewerGui::loadLuaFile(const std::string& filename)
    {
        auto result = m_viewer.loadLuaFile(filename);
        if (!result.ok())
        {
            openErrorPopup(result.getMessage());
        }
    }

    template <class T>
    void LogicViewerGui::copyInputs(const std::string_view& ns, Collection<T> collection)
    {
        PathVector path;
        path.push_back(LogicViewer::ltnModule);
        path.push_back(ns);
        ImGui::LogToClipboard();
        for (auto* node : collection)
        {
            logInputs(node, path);
        }
        ImGui::LogFinish();
    }

    void LogicViewerGui::copyScriptInputs()
    {
        copyInputs(LogicViewer::ltnScript, m_logicEngine.scripts());
    }

    void LogicViewerGui::drawGlobalContextMenu()
    {
        if (ImGui::BeginPopupContextVoid("GlobalContextMenu"))
        {
            drawMenuItemShowWindow();
            drawMenuItemReload();
            drawMenuItemCopy();
            if (ImGui::MenuItem("Next view", "Arrow Right", nullptr, (m_viewer.getCurrentView() < m_viewer.getViewCount())))
                m_viewer.setCurrentView(m_viewer.getCurrentView() + 1);
            if (ImGui::MenuItem("Previous view", "Arrow Left", nullptr, m_viewer.getCurrentView() > 1))
                m_viewer.setCurrentView(m_viewer.getCurrentView() - 1);
            ImGui::EndPopup();
        }
    }

    void LogicViewerGui::drawSceneTexture()
    {
        if (m_sampler)
        {
            ImGui::GetBackgroundDrawList()->AddImage(m_sampler, ImVec2(0, 0), m_samplerSize, ImVec2(0, 1), ImVec2(1, 0));
        }
    }

    void LogicViewerGui::drawErrorPopup()
    {
        if (!m_lastErrorMessage.empty())
            ImGui::OpenPopup("Error");

        if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(m_lastErrorMessage.c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                m_lastErrorMessage.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy Message", ImVec2(120, 0)))
            {
                ImGui::LogToClipboard();
                LogText(m_lastErrorMessage);
                ImGui::LogFinish();
            }
            ImGui::EndPopup();
        }
    }

    void LogicViewerGui::drawWindow()
    {
        if (!ImGui::Begin("Logic Viewer", &m_settings.showWindow, ImGuiWindowFlags_MenuBar))
        {
            ImGui::End();
            return;
        }

        drawMenuBar();
        drawCurrentView();

        if (m_settings.showScripts)
        {
            drawScripts();
        }

        if (m_settings.showAnimationNodes)
        {
            drawAnimationNodes();
        }

        if (m_settings.showDataArrays && ImGui::CollapsingHeader("Data Arrays"))
        {
            for (auto* obj : m_logicEngine.dataArrays())
            {
                DrawDataArray(obj);
            }
        }

        if (m_settings.showRamsesBindings)
        {
            drawAppearanceBindings();
            drawNodeBindings();
            drawCameraBindings();
        }

        ImGui::End();
    }

    void LogicViewerGui::drawMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                drawMenuItemReload();
                drawMenuItemCopy();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Settings"))
            {
                drawMenuItemShowWindow();
                ImGui::Separator();
                ImGui::MenuItem("Show Scripts", nullptr, &m_settings.showScripts);
                ImGui::MenuItem("Show Animation Nodes", nullptr, &m_settings.showAnimationNodes);
                ImGui::MenuItem("Show Data Arrays", nullptr, &m_settings.showDataArrays);
                ImGui::MenuItem("Show Ramses Bindings", nullptr, &m_settings.showRamsesBindings);
                ImGui::Separator();
                ImGui::MenuItem("Show Linked Inputs", nullptr, &m_settings.showLinkedInputs);
                ImGui::MenuItem("Show Outputs", nullptr, &m_settings.showOutputs);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    void LogicViewerGui::drawCurrentView()
    {
        const auto  viewCount = static_cast<int>(m_viewer.getViewCount());
        const auto& status    = m_viewer.getLastResult();
        if (!status.ok())
        {
            ImGui::TextUnformatted(fmt::format("Error occurred in {}", m_viewer.getLuaFilename()).c_str());
            ImGui::TextUnformatted(status.getMessage().c_str());
        }
        else if (m_viewer.getLuaFilename().empty())
        {
            drawSaveDefaultLuaFile();
        }
        else if (viewCount > 0)
        {
            auto       viewId = static_cast<int>(m_viewer.getCurrentView());
            const auto view   = m_viewer.getView(viewId);
            ImGui::TextUnformatted(!view.name().empty() ? view.name().c_str() : "<no name>");
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputInt("##View", &viewId))
                m_viewer.setCurrentView(viewId);
            ImGui::SameLine();
            ImGui::TextUnformatted(fmt::format("of {}", viewCount).c_str());

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
            ImGui::TextWrapped("%s", view.description().c_str());

            for (size_t i = 0U; i < view.getInputCount(); ++i)
            {
                auto* prop = view.getInput(i);
                if (prop)
                    drawProperty(view.getInput(i), i);
            }
        }
        else
        {
            ImGui::TextUnformatted("no views defined in configuration file");
        }
    }

    void LogicViewerGui::drawScripts()
    {
        const bool openScripts = ImGui::CollapsingHeader("Scripts");
        if (ImGui::BeginPopupContextItem("ScriptsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Script inputs"))
            {
                copyScriptInputs();
            }
            ImGui::EndPopup();
        }
        if (openScripts)
        {
            for (auto* script : m_logicEngine.scripts())
            {
                const bool open = TreeNode(script, script->getName());
                drawNodeContextMenu(script, LogicViewer::ltnScript);
                if (open)
                {
                    drawNode(script);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawAnimationNodes()
    {
        const bool openAnimationNodes = ImGui::CollapsingHeader("Animation Nodes");
        if (ImGui::BeginPopupContextItem("AnimationNodesContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Animation Node inputs"))
            {
                copyInputs(LogicViewer::ltnAnimation, m_logicEngine.animationNodes());
            }
            ImGui::EndPopup();
        }
        if (openAnimationNodes)
        {
            for (auto* obj : m_logicEngine.animationNodes())
            {
                const bool open = TreeNode(obj, obj->getName());
                drawNodeContextMenu(obj, LogicViewer::ltnAnimation);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Duration: {}", obj->getDuration()).c_str());
                    if (ImGui::TreeNode("Channels"))
                    {
                        for (auto& ch : obj->getChannels())
                        {
                            if (TreeNode(&ch, ch.name))
                            {
                                ImGui::TextUnformatted(fmt::format("InterpolationType: {}", EnumToString(ch.interpolationType)).c_str());
                                DrawDataArray(ch.keyframes, "Keyframes");
                                DrawDataArray(ch.tangentsIn, "TangentsIn");
                                DrawDataArray(ch.tangentsOut, "TangentsOut");
                                DrawDataArray(ch.timeStamps, "TimeStamps");
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawNodeBindings()
    {
        const bool openBindings = ImGui::CollapsingHeader("Node Bindings");
        if (ImGui::BeginPopupContextItem("NodeBindingsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Node Binding inputs"))
            {
                copyInputs(LogicViewer::ltnNode, m_logicEngine.ramsesNodeBindings());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.ramsesNodeBindings())
            {
                const bool open = TreeNode(obj, obj->getName());
                drawNodeContextMenu(obj, LogicViewer::ltnNode);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses Node: {}", obj->getRamsesNode().getName()).c_str());
                    ImGui::TextUnformatted(fmt::format("Rotation Mode: {}", EnumToString(obj->getRotationType())).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawCameraBindings()
    {
        const bool openBindings = ImGui::CollapsingHeader("Camera Bindings");
        if (ImGui::BeginPopupContextItem("CameraBindingsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Camera Binding inputs"))
            {
                copyInputs(LogicViewer::ltnCamera, m_logicEngine.ramsesCameraBindings());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.ramsesCameraBindings())
            {
                const bool open = TreeNode(obj, obj->getName());
                drawNodeContextMenu(obj, LogicViewer::ltnCamera);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses Camera: {}", obj->getRamsesCamera().getName()).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawAppearanceBindings()
    {
        const bool openBindings = ImGui::CollapsingHeader("Appearance Bindings");
        if (ImGui::BeginPopupContextItem("AppearanceBindingsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Appearance Binding inputs"))
            {
                copyInputs(LogicViewer::ltnAppearance, m_logicEngine.ramsesAppearanceBindings());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.ramsesAppearanceBindings())
            {
                const bool open = TreeNode(obj, obj->getName());
                drawNodeContextMenu(obj, LogicViewer::ltnAppearance);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses Appearance: {}", obj->getRamsesAppearance().getName()).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawSaveDefaultLuaFile()
    {
        ImGui::TextUnformatted("No lua configuration file found.");
        ImGui::InputText("##filename", &m_filename);
        ImGui::SameLine();
        if (ImGui::Button("Save default"))
        {
            fs::path luafile(m_filename);
            if (fs::exists(luafile))
            {
                ImGui::OpenPopup("Append?");
            }
            else if (!luafile.empty())
            {
                saveDefaultLuaFile();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            fs::path luafile(m_filename);
            if (fs::exists(luafile))
            {
                loadLuaFile(m_filename);
            }
            else if (!luafile.empty())
            {
                m_lastErrorMessage = "File does not exist: " + m_filename;
            }
        }

        if (ImGui::BeginPopupModal("Append?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(fmt::format("File exists:\n{}\nAppend default lua configuration?", m_filename).c_str());
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                saveDefaultLuaFile();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }


    void LogicViewerGui::drawNodeContextMenu(rlogic::LogicNode* obj, const std::string_view& ns)
    {
        if (ImGui::BeginPopupContextItem(obj->getName().data()))
        {
            if (ImGui::MenuItem(fmt::format("Copy {} inputs", obj->getName()).c_str()))
            {
                PathVector path;
                path.push_back(LogicViewer::ltnModule);
                path.push_back(ns);
                ImGui::LogToClipboard();
                logInputs(obj, path);
                ImGui::LogFinish();
            }
            ImGui::EndPopup();
        }
    }

    void LogicViewerGui::drawNode(rlogic::LogicNode* obj)
    {
        auto* in = obj->getInputs();
        const auto* out = obj->getOutputs();
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        if (TreeNode(in, in->getName()))
        {
            for (size_t i = 0; i < in->getChildCount(); ++i)
            {
                drawProperty(in->getChild(i), i);
            }
            ImGui::TreePop();
        }
        if (out != nullptr && m_settings.showOutputs)
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            if (TreeNode(out, out->getName()))
            {
                for (size_t i = 0; i < out->getChildCount(); ++i)
                {
                    drawOutProperty(out->getChild(i), i);
                }
                ImGui::TreePop();
            }
        }
    }

    void LogicViewerGui::drawProperty(rlogic::Property* prop, size_t index)
    {
        const bool  isLinked = prop->isLinked();
        if (isLinked && !m_settings.showLinkedInputs)
            return;

        std::string strName = prop->getName().data();
        if (prop->getName().empty())
            strName = fmt::format("[{}]", index);
        const char* name = strName.c_str();

        switch (prop->getType())
        {
        case rlogic::EPropertyType::Int32: {
            auto value = prop->get<int32_t>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            }
            else if (ImGui::DragInt(name, &value, 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Float: {
            auto value = prop->get<float>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            }
            else if (ImGui::DragFloat(name, &value, 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec2f: {
            auto value = prop->get<rlogic::vec2f>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {})", name, value[0], value[1]).c_str());
            }
            else if (ImGui::DragFloat2(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec3f: {
            auto value = prop->get<rlogic::vec3f>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {})", name, value[0], value[1], value[2]).c_str());
            }
            else if (ImGui::DragFloat3(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec4f: {
            auto value = prop->get<rlogic::vec4f>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {}, {})", name, value[0], value[1], value[2], value[3]).c_str());
            }
            else if (ImGui::DragFloat4(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec2i: {
            auto value = prop->get<rlogic::vec2i>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {})", name, value[0], value[1]).c_str());
            }
            else if (ImGui::DragInt2(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec3i: {
            auto value = prop->get<rlogic::vec3i>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {})", name, value[0], value[1], value[2]).c_str());
            }
            else if (ImGui::DragInt3(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Vec4i: {
            auto value = prop->get<rlogic::vec4i>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {}, {})", name, value[0], value[1], value[2], value[3]).c_str());
            }
            else if (ImGui::DragInt4(name, value.data(), 0.1f))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Struct:
            if (TreeNode(prop, fmt::format("Struct {}", name)))
            {
                for (size_t i = 0U; i < prop->getChildCount(); ++i)
                {
                    auto* child = prop->getChild(i);
                    drawProperty(child, i);
                }
                ImGui::TreePop();
            }
            break;
        case rlogic::EPropertyType::Bool: {
            auto value = prop->get<bool>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: {}", name, value ? "true" : "false").c_str());
            }
            else if (ImGui::Checkbox(name, &value))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::String: {
            auto value = prop->get<std::string>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            }
            else if (ImGui::InputText(name, &value))
            {
                prop->set(value);
            }
            break;
        }
        case rlogic::EPropertyType::Array:
            if (TreeNode(prop, fmt::format("Array {}", name)))
            {
                for (size_t i = 0U; i < prop->getChildCount(); ++i)
                {
                    auto* child = prop->getChild(i);
                    drawProperty(child, i);
                }
                ImGui::TreePop();
            }
            break;
        }
    }

    void LogicViewerGui::drawOutProperty(const rlogic::Property* prop, size_t index)
    {
        std::string strName = prop->getName().data();
        if (prop->getName().empty())
            strName = fmt::format("[{}]", index);
        const char* name = strName.c_str();

        switch (prop->getType())
        {
        case rlogic::EPropertyType::Int32: {
            auto value = prop->get<int32_t>().value();
            ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            break;
        }
        case rlogic::EPropertyType::Float: {
            auto value = prop->get<float>().value();
            ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec2f: {
            auto value = prop->get<rlogic::vec2f>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {})", name, value[0], value[1]).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec3f: {
            auto value = prop->get<rlogic::vec3f>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {})", name, value[0], value[1], value[2]).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec4f: {
            auto value = prop->get<rlogic::vec4f>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {}, {})", name, value[0], value[1], value[2], value[3]).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec2i: {
            auto value = prop->get<rlogic::vec2i>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {})", name, value[0], value[1]).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec3i: {
            auto value = prop->get<rlogic::vec3i>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {})", name, value[0], value[1], value[2]).c_str());
            break;
        }
        case rlogic::EPropertyType::Vec4i: {
            auto value = prop->get<rlogic::vec4i>().value();
            ImGui::TextUnformatted(fmt::format("{}: ({}, {}, {}, {})", name, value[0], value[1], value[2], value[3]).c_str());
            break;
        }
        case rlogic::EPropertyType::Struct:
            if (TreeNode(prop, fmt::format("Struct {}", name)))
            {
                for (size_t i = 0U; i < prop->getChildCount(); ++i)
                {
                    const auto* child = prop->getChild(i);
                    drawOutProperty(child, i);
                }
                ImGui::TreePop();
            }
            break;
        case rlogic::EPropertyType::String: {
            auto value = prop->get<std::string>().value();
            ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            break;
        }
        case rlogic::EPropertyType::Bool: {
            auto value = prop->get<bool>().value();
            ImGui::TextUnformatted(fmt::format("{}: {}", name, value ? "true" : "false").c_str());
            break;
        }
        case rlogic::EPropertyType::Array:
            if (TreeNode(prop, fmt::format("Array {}", name)))
            {
                for (size_t i = 0U; i < prop->getChildCount(); ++i)
                {
                    const auto* child = prop->getChild(i);
                    drawOutProperty(child, i);
                }
                ImGui::TreePop();
            }
            break;
        }
    }

    void LogicViewerGui::DrawDataArray(const rlogic::DataArray* obj, std::string_view context)
    {
        if (obj != nullptr)
        {
            if (!context.empty())
            {
                ImGui::TextUnformatted(
                    fmt::format("{}: Name:{} Type:{}[{}]", context.data(), obj->getName(), GetLuaPrimitiveTypeName(obj->getDataType()), obj->getNumElements()).c_str());
            }
            else
            {
                ImGui::TextUnformatted(fmt::format("Name:{} Type:{}[{}]", obj->getName(), GetLuaPrimitiveTypeName(obj->getDataType()), obj->getNumElements()).c_str());
            }
        }
    }

    void LogicViewerGui::logInputs(rlogic::LogicNode* obj, PathVector& path)
    {
        path.push_back(obj->getName());
        logProperty(obj->getInputs(), path);
        path.pop_back();
    }

    void LogicViewerGui::logProperty(rlogic::Property* prop, PathVector& path)
    {
        if (prop->isLinked())
            return;

        path.push_back(prop->getName());

        auto strPath = fmt::format("{}[\"{}\"].value", path.front(), fmt::join(path.begin() + 1, path.end(), "\"][\""));

        switch (prop->getType())
        {
        case rlogic::EPropertyType::Int32:
            LogText(fmt::format("{} = {}\n", strPath, prop->get<int32_t>().value()));
            break;
        case rlogic::EPropertyType::Float:
            LogText(fmt::format("{} = {}\n", strPath, prop->get<float>().value()));
            break;
        case rlogic::EPropertyType::Vec2f: {
            auto val = prop->get<rlogic::vec2f>().value();
            LogText(fmt::format("{} = {{ {}, {} }}\n", strPath, val[0], val[1]));
            break;
        }
        case rlogic::EPropertyType::Vec3f: {
            auto val = prop->get<rlogic::vec3f>().value();
            LogText(fmt::format("{} = {{ {}, {}, {} }}\n", strPath, val[0], val[1], val[2]));
            break;
        }
        case rlogic::EPropertyType::Vec4f: {
            auto val = prop->get<rlogic::vec4f>().value();
            LogText(fmt::format("{} = {{ {}, {}, {}, {} }}\n", strPath, val[0], val[1], val[2], val[3]));
            break;
        }
        case rlogic::EPropertyType::Vec2i: {
            auto val = prop->get<rlogic::vec2i>().value();
            LogText(fmt::format("{} = {{ {}, {} }}\n", strPath, val[0], val[1]));
            break;
        }
        case rlogic::EPropertyType::Vec3i: {
            auto val = prop->get<rlogic::vec3i>().value();
            LogText(fmt::format("{} = {{ {}, {}, {} }}\n", strPath, val[0], val[1], val[2]));
            break;
        }
        case rlogic::EPropertyType::Vec4i: {
            auto val = prop->get<rlogic::vec4i>().value();
            LogText(fmt::format("{} = {{ {}, {}, {}, {} }}\n", strPath, val[0], val[1], val[2], val[3]));
            break;
        }
        case rlogic::EPropertyType::Struct:
            for (size_t i = 0U; i < prop->getChildCount(); ++i)
            {
                logProperty(prop->getChild(i), path);
            }
            break;
        case rlogic::EPropertyType::Bool: {
            LogText(fmt::format("{} = {}\n", strPath, prop->get<bool>().value()));
            break;
        }
        case rlogic::EPropertyType::String:
            LogText(fmt::format("{} = {}\n", strPath, prop->get<std::string>().value()));
            break;
        case rlogic::EPropertyType::Array:
            break;
        }
        path.pop_back();
    }

    void* LogicViewerGui::IniReadOpen(ImGuiContext* /*context*/, ImGuiSettingsHandler* handler, const char* /*name*/)
    {
        return handler->UserData;
    }

    void LogicViewerGui::IniReadLine(ImGuiContext* /*context*/, ImGuiSettingsHandler* handler, void* /*entry*/, const char* line)
    {
        auto* gui = static_cast<LogicViewerGui*>(handler->UserData);
        int flag = 0;
        if (IniReadFlag(line, "ShowWindow=%d", &flag))
        {
            gui->m_settings.showWindow = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowScripts=%d", &flag))
        {
            gui->m_settings.showScripts = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowAnimationNodes=%d", &flag))
        {
            gui->m_settings.showAnimationNodes = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowDataArrays=%d", &flag))
        {
            gui->m_settings.showDataArrays = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowRamsesBindings=%d", &flag))
        {
            gui->m_settings.showRamsesBindings = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowLinkedInputs=%d", &flag))
        {
            gui->m_settings.showLinkedInputs = (flag != 0);
        }
        else if (IniReadFlag(line, "ShowOutputs=%d", &flag))
        {
            gui->m_settings.showOutputs = (flag != 0);
        }
    }

    void LogicViewerGui::IniWriteAll(ImGuiContext* /*context*/, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
    {
        auto* gui = static_cast<LogicViewerGui*>(handler->UserData);
        buf->reserve(buf->size() + 200);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("[%s][Settings]\n", handler->TypeName);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowWindow=%d\n", gui->m_settings.showWindow ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowScripts=%d\n", gui->m_settings.showScripts ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowAnimationNodes=%d\n", gui->m_settings.showAnimationNodes ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowDataArrays=%d\n", gui->m_settings.showDataArrays ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowRamsesBindings=%d\n", gui->m_settings.showRamsesBindings ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowLinkedInputs=%d\n", gui->m_settings.showLinkedInputs ? 1 : 0);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
        buf->appendf("ShowOutputs=%d\n", gui->m_settings.showOutputs ? 1 : 0);
        buf->append("\n");
    }

    void LogicViewerGui::saveDefaultLuaFile()
    {
        ImGui::LogToFile(-1, m_filename.c_str());
        LogText("function default()\n");

        PathVector path;
        std::string name = std::string("    ") + LogicViewer::ltnModule + "." + LogicViewer::ltnScript;
        path.push_back(name);
        for (auto* script : m_logicEngine.scripts())
        {
            logInputs(script, path);
        }
        LogText("end\n\n");
        const char* code = R"(
defaultView = {
    name = "Default",
    description = "",
    update = function(time_ms)
        default()
    end
}

rlogic.views = {defaultView}

-- sample test function for automated image base tests
-- can be executed by command line parameter --exec=test_default
function test_default()
    -- modify properties
    default()
    -- stores a screenshot (relative to the working directory)
    rlogic.screenshot("test_default.png")
end

)";
        LogText(code);
        ImGui::LogFinish();
        loadLuaFile(m_filename);
    }
}

