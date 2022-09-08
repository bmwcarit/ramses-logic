//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicViewerGui.h"
#include "LogicViewer.h"
#include "LogicViewerSettings.h"
#include "ramses-client-api/Scene.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesRenderPassBinding.h"
#include "ramses-logic/RamsesRenderGroupBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/AnchorPoint.h"
#include "internals/StdFilesystemWrapper.h"
#include "fmt/format.h"

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include "imgui_internal.h" // for ImGuiSettingsHandler
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

template <> struct fmt::formatter<std::chrono::microseconds>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext> constexpr auto format(const std::chrono::microseconds value, FormatContext& ctx)
    {
        const auto c = value.count();
        if (c == 0)
            return fmt::format_to(ctx.out(), "0");
        return fmt::format_to(ctx.out(), "{}.{:03}", c / 1000, c % 1000);
    }
};


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

        const char* TypeName(const LogicNode* node)
        {
            const char* name = "Unknown";
            if (node->as<LuaInterface>() != nullptr)
            {
                name = "LuaInterface";
            }
            else if (node->as<LuaScript>() != nullptr)
            {
                name = "LuaScript";
            }
            else if (node->as<AnimationNode>() != nullptr)
            {
                name = "Animation";
            }
            else if (node->as<RamsesNodeBinding>() != nullptr)
            {
                name = "NodeBinding";
            }
            else if (node->as<RamsesAppearanceBinding>() != nullptr)
            {
                name = "AppearanceBinding";
            }
            else if (node->as<RamsesCameraBinding>() != nullptr)
            {
                name = "CameraBinding";
            }
            else if (node->as<RamsesRenderPassBinding>() != nullptr)
            {
                name = "RenderPassBinding";
            }
            else if (node->as<RamsesRenderGroupBinding>() != nullptr)
            {
                name = "RenderGroupBinding";
            }
            else if (node->as<TimerNode>() != nullptr)
            {
                name = "Timer";
            }
            else if (node->as<AnchorPoint>() != nullptr)
            {
                name = "AnchorPoint";
            }
            return name;
        }

        template <typename... Args>
        void HelpMarker(const char* desc, Args&& ... args)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg) 3rd party interface
            ImGui::TextDisabled("(?)");
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(fmt::format(desc, args...).c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    }

    LogicViewerGui::LogicViewerGui(rlogic::LogicViewer& viewer, LogicViewerSettings& settings, std::string luafile)
        : m_settings(settings)
        , m_viewer(viewer)
        , m_logicEngine(viewer.getEngine())
        , m_filename(std::move(luafile))
    {
        m_viewer.enableUpdateReport(m_settings.showUpdateReport, m_updateReportInterval);
    }

    void LogicViewerGui::draw()
    {
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
            ImGui::MarkIniSettingsDirty();
        }
        else if (ImGui::IsKeyPressed(ramses::EKeyCode_F5))
        {
            reloadConfiguration();
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

    void LogicViewerGui::setRendererInfo(ramses::RamsesRenderer& renderer, ramses::displayId_t displayId, ramses::displayBufferId_t displayBufferId, const std::array<float, 4>& initialClearColor)
    {
        m_renderer = &renderer;
        m_displayId = displayId;
        m_displayBufferId = displayBufferId;
        m_clearColor = initialClearColor;
    }

    void LogicViewerGui::drawMenuItemShowWindow()
    {
        if (ImGui::MenuItem("Show Logic Viewer Window", "F11", &m_settings.showWindow))
        {
            ImGui::MarkIniSettingsDirty();
        }
    }

    void LogicViewerGui::drawMenuItemReload()
    {
        if (ImGui::MenuItem("Reload configuration", "F5"))
        {
            reloadConfiguration();
        }
    }

    void LogicViewerGui::drawMenuItemCopy()
    {
        if (ImGui::MenuItem("Copy script inputs", "Ctrl+C"))
        {
            copyScriptInputs();
        }
    }

    void LogicViewerGui::reloadConfiguration()
    {
        if (fs::exists(m_filename))
        {
            loadLuaFile(m_filename);
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
        copyInputs(LogicViewer::ltnScript, m_logicEngine.getCollection<LuaScript>());
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
        if (!ImGui::Begin(fmt::format("Logic Viewer (FeatureLevel 0{})", m_logicEngine.getFeatureLevel()).c_str(), &m_settings.showWindow, ImGuiWindowFlags_MenuBar))
        {
            ImGui::End();
            return;
        }

        drawMenuBar();
        drawCurrentView();

        if (m_settings.showInterfaces)
        {
            drawInterfaces();
        }

        if (m_settings.showScripts)
        {
            drawScripts();
        }

        if (m_settings.showAnimationNodes)
        {
            drawAnimationNodes();
        }

        if (m_settings.showTimerNodes)
        {
            drawTimerNodes();
        }

        if (m_settings.showDataArrays && ImGui::CollapsingHeader("Data Arrays"))
        {
            for (auto* obj : m_logicEngine.getCollection<DataArray>())
            {
                DrawDataArray(obj);
            }
        }

        if (m_settings.showRamsesBindings)
        {
            drawAppearanceBindings();
            drawNodeBindings();
            drawCameraBindings();
            drawRenderPassBindings();
            drawRenderGroupBindings();
            drawAnchorPoints();
        }

        if (m_settings.showUpdateReport)
        {
            drawUpdateReport();
        }

        if (m_settings.showDisplaySettings && m_renderer)
        {
            drawDisplaySettings();
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
                bool changed = ImGui::MenuItem("Show Interfaces", nullptr, &m_settings.showInterfaces);
                changed = ImGui::MenuItem("Show Scripts", nullptr, &m_settings.showScripts) || changed;
                changed = ImGui::MenuItem("Show Animation Nodes", nullptr, &m_settings.showAnimationNodes) || changed;
                changed = ImGui::MenuItem("Show Timer Nodes", nullptr, &m_settings.showTimerNodes) || changed;
                changed = ImGui::MenuItem("Show Data Arrays", nullptr, &m_settings.showDataArrays) || changed;
                changed = ImGui::MenuItem("Show Ramses Bindings", nullptr, &m_settings.showRamsesBindings) || changed;
                if (ImGui::MenuItem("Show Update Report", nullptr, &m_settings.showUpdateReport))
                {
                    m_viewer.enableUpdateReport(m_settings.showUpdateReport, m_updateReportInterval);
                    ImGui::MarkIniSettingsDirty();
                }
                ImGui::Separator();
                changed = ImGui::MenuItem("Show Linked Inputs", nullptr, &m_settings.showLinkedInputs) || changed;
                changed = ImGui::MenuItem("Show Outputs", nullptr, &m_settings.showOutputs) || changed;
                ImGui::Separator();
                changed = ImGui::MenuItem("Lua: prefer identifiers (scripts.foo)", nullptr, &m_settings.luaPreferIdentifiers) || changed;
                changed = ImGui::MenuItem("Lua: prefer object ids (scripts[1])", nullptr, &m_settings.luaPreferObjectIds) || changed;
                ImGui::Separator();
                changed = ImGui::MenuItem("Show Display Settings", nullptr, &m_settings.showDisplaySettings) || changed;
                ImGui::EndMenu();

                if (changed)
                {
                    ImGui::MarkIniSettingsDirty();
                }
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

        if (m_settings.showUpdateReport)
        {
            ImGui::Separator();
            ImGui::TextUnformatted(fmt::format("Average Update Time: {} ms", m_viewer.getUpdateReport().getTotalTime().average).c_str());
            ImGui::SameLine();
            HelpMarker("Time it took to update the whole logic nodes network (LogicEngine::update()).");
        }
    }

    bool LogicViewerGui::DrawTreeNode(rlogic::LogicObject* obj)
    {
        return TreeNode(obj, fmt::format("[{}]: {}", obj->getId(), obj->getName()));
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
            for (auto* script : m_logicEngine.getCollection<LuaScript>())
            {
                const bool open = DrawTreeNode(script);
                drawNodeContextMenu(script, LogicViewer::ltnScript);
                if (open)
                {
                    drawNode(script);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawInterfaces()
    {
        const bool openInterfaces = ImGui::CollapsingHeader("Interfaces");
        if (ImGui::BeginPopupContextItem("InterfacesContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Interface inputs"))
            {
                copyInputs(LogicViewer::ltnInterface, m_logicEngine.getCollection<LuaInterface>());
            }
            ImGui::EndPopup();
        }
        if (openInterfaces)
        {
            for (auto* script : m_logicEngine.getCollection<LuaInterface>())
            {
                const bool open = DrawTreeNode(script);
                drawNodeContextMenu(script, LogicViewer::ltnInterface);
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
                copyInputs(LogicViewer::ltnAnimation, m_logicEngine.getCollection<AnimationNode>());
            }
            ImGui::EndPopup();
        }
        if (openAnimationNodes)
        {
            for (auto* obj : m_logicEngine.getCollection<AnimationNode>())
            {
                const bool open = DrawTreeNode(obj);
                drawNodeContextMenu(obj, LogicViewer::ltnAnimation);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Duration: {}", *obj->getOutputs()->getChild("duration")->get<float>()).c_str());
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

    void LogicViewerGui::drawTimerNodes()
    {
        const bool openTimerNodes = ImGui::CollapsingHeader("Timer Nodes");
        if (ImGui::BeginPopupContextItem("TimerNodesContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Timer Node inputs"))
            {
                copyInputs(LogicViewer::ltnTimer, m_logicEngine.getCollection<TimerNode>());
            }
            ImGui::EndPopup();
        }
        if (openTimerNodes)
        {
            for (auto* obj : m_logicEngine.getCollection<TimerNode>())
            {
                const bool open = DrawTreeNode(obj);
                drawNodeContextMenu(obj, LogicViewer::ltnTimer);
                if (open)
                {
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
                copyInputs(LogicViewer::ltnNode, m_logicEngine.getCollection<RamsesNodeBinding>());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.getCollection<RamsesNodeBinding>())
            {
                const bool open = DrawTreeNode(obj);
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
                copyInputs(LogicViewer::ltnCamera, m_logicEngine.getCollection<RamsesCameraBinding>());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.getCollection<RamsesCameraBinding>())
            {
                const bool open = DrawTreeNode(obj);
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

    void LogicViewerGui::drawRenderPassBindings()
    {
        const bool openBindings = ImGui::CollapsingHeader("RenderPass Bindings");
        if (ImGui::BeginPopupContextItem("RenderPassBindingsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all RenderPass Binding inputs"))
            {
                copyInputs(LogicViewer::ltnRenderPass, m_logicEngine.getCollection<RamsesRenderPassBinding>());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.getCollection<RamsesRenderPassBinding>())
            {
                const bool open = DrawTreeNode(obj);
                drawNodeContextMenu(obj, LogicViewer::ltnRenderPass);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses RenderPass: {}", obj->getRamsesRenderPass().getName()).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawRenderGroupBindings()
    {
        const bool openBindings = ImGui::CollapsingHeader("RenderGroup Bindings");
        if (ImGui::BeginPopupContextItem("RenderGroupBindingsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all RenderGroup Binding inputs"))
            {
                copyInputs(LogicViewer::ltnRenderGroup, m_logicEngine.getCollection<RamsesRenderGroupBinding>());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.getCollection<RamsesRenderGroupBinding>())
            {
                const bool open = DrawTreeNode(obj);
                drawNodeContextMenu(obj, LogicViewer::ltnRenderGroup);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses RenderGroup: {}", obj->getRamsesRenderGroup().getName()).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawAnchorPoints()
    {
        const bool openAnchors = ImGui::CollapsingHeader("Anchor Points");
        if (ImGui::BeginPopupContextItem("AnchorPointsContextMenu"))
        {
            if (ImGui::MenuItem("Copy all Anchor Point inputs"))
            {
                copyInputs(LogicViewer::ltnAnchorPoint, m_logicEngine.getCollection<AnchorPoint>());
            }
            ImGui::EndPopup();
        }
        if (openAnchors)
        {
            for (auto* obj : m_logicEngine.getCollection<AnchorPoint>())
            {
                const bool open = DrawTreeNode(obj);
                drawNodeContextMenu(obj, LogicViewer::ltnAnchorPoint);
                if (open)
                {
                    ImGui::TextUnformatted(fmt::format("Ramses Node: {}", obj->getRamsesNode().getName()).c_str());
                    ImGui::TextUnformatted(fmt::format("Ramses Camera: {}", obj->getRamsesCamera().getName()).c_str());
                    drawNode(obj);
                    ImGui::TreePop();
                }
            }
        }
    }

    void LogicViewerGui::drawUpdateReport()
    {
        const bool open = ImGui::CollapsingHeader("Update Report");
        if (open)
        {
            auto interval = static_cast<int>(m_updateReportInterval);
            bool refresh = m_viewer.isUpdateReportEnabled();
            if (ImGui::Checkbox("Auto Refresh", &refresh))
            {
                m_viewer.enableUpdateReport(refresh, m_updateReportInterval);
            }
            ImGui::SetNextItemWidth(100);
            if (ImGui::DragInt("Refresh Interval", &interval, 0.5f, 1, 1000, "%d Frames"))
            {
                m_updateReportInterval = static_cast<size_t>(interval);
                m_viewer.enableUpdateReport(refresh, m_updateReportInterval);
            }
            const auto& report = m_viewer.getUpdateReport();
            const auto& executed = report.getNodesExecuted();
            const auto& skipped  = report.getNodesSkippedExecution();
            const auto longest = report.getTotalTime().maxValue;

            ImGui::Separator();
            ImGui::TextUnformatted("Summary:");
            ImGui::SameLine();
            HelpMarker("Timing data is collected and summarized for {} frames.\n'min', 'max', 'avg' show the minimum, maximum, and average value for the measured interval.", m_updateReportInterval);
            ImGui::Indent();

            const auto& updateTime = report.getTotalTime();
            ImGui::TextUnformatted(fmt::format("Total Update Time  (ms): max:{} min:{} avg:{}", updateTime.maxValue, updateTime.minValue, updateTime.average).c_str());
            ImGui::SameLine();
            HelpMarker("Time it took to update the whole logic nodes network (LogicEngine::update()).");

            const auto& sortTime = report.getSortTime();
            ImGui::TextUnformatted(fmt::format("Topology Sort Time (ms): max:{} min:{} avg:{}", sortTime.maxValue, sortTime.minValue, sortTime.average).c_str());
            ImGui::SameLine();
            HelpMarker("Time it took to sort logic nodes by their topology during update (see rlogic::LogicEngineReport::getTopologySortExecutionTime()");

            const auto& links = report.getLinkActivations();
            ImGui::TextUnformatted(fmt::format("Activated Links: max:{} min:{} avg:{}", links.maxValue, links.minValue, links.average).c_str());
            ImGui::SameLine();
            HelpMarker("Number of input properties that had been updated by an output property (see rlogic::LogicEngineReport::getTotalLinkActivations()).");
            ImGui::Unindent();

            ImGui::TextUnformatted(fmt::format("Details for the longest update ({} ms):", longest).c_str());
            if (TreeNode("Executed", fmt::format("Executed Nodes ({}):", executed.size())))
            {
                for (auto& timedNode : executed)
                {
                    auto* node = timedNode.first;
                    const auto percentage = (longest.count() > 0u) ? (100u * timedNode.second / longest) : 0u;
                    if (TreeNode(node, fmt::format("{}[{}]: {} [time:{} ms, {}%]", TypeName(node), node->getId(), node->getName(), timedNode.second, percentage)))
                    {
                        drawNode(node);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            if (TreeNode("Skipped", fmt::format("Skipped Nodes ({}):", skipped.size())))
            {
                for (auto& node : skipped)
                {
                    if (TreeNode(node, fmt::format("{}[{}]: {}", TypeName(node), node->getId(), node->getName())))
                    {
                        drawNode(node);
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
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
                copyInputs(LogicViewer::ltnAppearance, m_logicEngine.getCollection<RamsesAppearanceBinding>());
            }
            ImGui::EndPopup();
        }
        if (openBindings)
        {
            for (auto* obj : m_logicEngine.getCollection<RamsesAppearanceBinding>())
            {
                const bool open = DrawTreeNode(obj);
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

    void LogicViewerGui::drawDisplaySettings()
    {
        assert(m_renderer);
        const bool openDisplaySettings = ImGui::CollapsingHeader("Display Settings");
        if (openDisplaySettings)
        {
            if (ImGui::DragFloat4("Clear color", m_clearColor.data(), 0.1f, 0.f, 1.f))
            {
                m_renderer->setDisplayBufferClearColor(m_displayId, m_displayBufferId, m_clearColor[0], m_clearColor[1], m_clearColor[2], m_clearColor[3]);
                m_renderer->flush();
            }

            float fps = m_renderer->getMaximumFramerate();
            if (ImGui::DragFloat("Maximum FPS", &fps, 1.f, 1.f, 1000.f))
            {
                m_renderer->setMaximumFramerate(fps);
                m_renderer->flush();
            }

            if (ImGui::Checkbox("Skip rendering of unmodified buffers", &m_skipUnmodifiedBuffers))
            {
                m_renderer->setSkippingOfUnmodifiedBuffers(m_skipUnmodifiedBuffers);
                m_renderer->flush();
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
                ImGui::OpenPopup("Overwrite?");
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

        if (ImGui::BeginPopupModal("Overwrite?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(fmt::format("File exists:\n{}\nOverwrite default lua configuration?", m_filename).c_str());
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
        if (in != nullptr)
        {
            if (TreeNode(in, std::string_view("Inputs")))
            {
                for (size_t i = 0; i < in->getChildCount(); ++i)
                {
                    drawProperty(in->getChild(i), i);
                }
                ImGui::TreePop();
            }
        }
        if (out != nullptr && m_settings.showOutputs)
        {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            if (TreeNode(out, std::string_view("Outputs")))
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
        const bool isLinked = prop->hasIncomingLink();
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
        case rlogic::EPropertyType::Int64: {
            auto value = prop->get<int64_t>().value();
            if (isLinked)
            {
                ImGui::TextUnformatted(fmt::format("{}: {}", name, value).c_str());
            }
            else if (ImGui::DragScalar(name, ImGuiDataType_S64, &value, 0.1f, nullptr, nullptr))
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
        case rlogic::EPropertyType::Int64: {
            auto value = prop->get<int64_t>().value();
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

    void LogicViewerGui::logInputs(rlogic::LogicNode* obj, const PathVector& path)
    {
        const auto joinedPath = fmt::format("{}", fmt::join(path.begin(), path.end(), "."));
        std::string prefix;
        if ((m_settings.luaPreferObjectIds) || obj->getName().empty())
        {
            prefix = fmt::format("{}[{}]", joinedPath, obj->getId());
        }
        else if (m_settings.luaPreferIdentifiers)
        {
            prefix = fmt::format("{}.{}", joinedPath, obj->getName());
        }
        else
        {
            prefix = fmt::format("{}[\"{}\"]", joinedPath, obj->getName());
        }
        PathVector propertyPath;
        propertyPath.push_back(LogicViewer::ltnIN);
        auto prop = obj->getInputs();
        for (size_t i = 0U; i < prop->getChildCount(); ++i)
        {
            logProperty(prop->getChild(i), prefix, propertyPath);
        }
    }

    void LogicViewerGui::logProperty(rlogic::Property* prop, const std::string& prefix, PathVector& path)
    {
        if (prop->hasIncomingLink())
            return;

        path.push_back(prop->getName());

        std::string strPath;
        if (m_settings.luaPreferIdentifiers)
        {
            strPath = fmt::format("{}.{}.value", prefix, fmt::join(path.begin(), path.end(), "."));
        }
        else
        {
            strPath = fmt::format("{}[\"{}\"].value", prefix, fmt::join(path.begin(), path.end(), "\"][\""));
        }

        switch (prop->getType())
        {
        case rlogic::EPropertyType::Int32:
            LogText(fmt::format("{} = {}\n", strPath, prop->get<int32_t>().value()));
            break;
        case rlogic::EPropertyType::Int64:
            LogText(fmt::format("{} = {}\n", strPath, prop->get<int64_t>().value()));
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
                logProperty(prop->getChild(i), prefix, path);
            }
            break;
        case rlogic::EPropertyType::Bool: {
            LogText(fmt::format("{} = {}\n", strPath, prop->get<bool>().value()));
            break;
        }
        case rlogic::EPropertyType::String:
            LogText(fmt::format("{} = '{}'\n", strPath, prop->get<std::string>().value()));
            break;
        case rlogic::EPropertyType::Array:
            break;
        }
        path.pop_back();
    }

    template<class T>
    void LogicViewerGui::logAllInputs(std::string_view headline, std::string_view ltn)
    {
        PathVector path;
        const std::string indent = "    ";
        std::string name = indent + LogicViewer::ltnModule + "." + ltn.data();
        path.push_back(name);
        LogText(indent + headline.data());
        for (auto* node : m_logicEngine.getCollection<T>())
        {
            logInputs(node, path);
        }
    }

    void LogicViewerGui::saveDefaultLuaFile()
    {
        std::error_code ec;
        fs::remove(m_filename, ec);
        if (ec)
        {
            m_lastErrorMessage = ec.message();
            return;
        }
        ImGui::LogToFile(-1, m_filename.c_str());
        LogText("function default()\n");

        logAllInputs<LuaInterface>("--Interfaces\n", LogicViewer::ltnInterface);
        logAllInputs<LuaScript>("--Scripts\n", LogicViewer::ltnScript);
        logAllInputs<RamsesNodeBinding>("--Node bindings\n", LogicViewer::ltnNode);
        logAllInputs<RamsesAppearanceBinding>("--Appearance bindings\n", LogicViewer::ltnAppearance);
        logAllInputs<RamsesCameraBinding>("--Camera bindings\n", LogicViewer::ltnCamera);
        logAllInputs<RamsesRenderPassBinding>("--RenderPass bindings\n", LogicViewer::ltnRenderPass);
        logAllInputs<RamsesRenderGroupBinding>("--RenderGroup bindings\n", LogicViewer::ltnRenderGroup);
        logAllInputs<AnchorPoint>("--Anchor points\n", LogicViewer::ltnRenderPass);

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

