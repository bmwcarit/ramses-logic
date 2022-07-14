//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "LogicViewer.h"
#include "LogicViewerLuaTypes.h"
#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/RamsesRenderPassBinding.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaInterface.h"
#include "ramses-logic/Property.h"
#include "ramses-logic/AnchorPoint.h"
#include "fmt/format.h"
#include "internals/SolHelper.h"
#include <iostream>

namespace rlogic
{
    namespace
    {
        // NOLINTNEXTLINE(performance-unnecessary-value-param) The signature is forced by SOL. Therefore we have to disable this warning.
        int solExceptionHandler(lua_State* L, sol::optional<const std::exception&> maybe_exception, sol::string_view description)
        {
            if (maybe_exception)
            {
                return sol::stack::push(L, description);
            }
            return sol::stack::top(L);
        }

        template<class T>
        void registerNodeListType(sol::state& sol, const char* name) {
            sol.new_usertype<NodeListWrapper<T>>(name,
                                                 sol::no_constructor,
                                                 sol::meta_function::index,
                                                 &NodeListWrapper<T>::get,
                                                 sol::meta_function::call,
                                                 &NodeListWrapper<T>::iterator,
                                                 sol::meta_function::to_string,
                                                 [=]() { return name; });
            std::string iteratorName = name;
            iteratorName += "Iterator";
            sol.new_usertype<NodeListIterator<T>>(iteratorName, sol::no_constructor, sol::meta_function::call, &NodeListIterator<T>::call);
        }
    } // namespace

    struct LogicWrapper
    {
        explicit LogicWrapper(LogicEngine& logicEngine, sol::state& sol)
            : views(sol.create_table())
            , interfaces(logicEngine)
            , scripts(logicEngine)
            , animations(logicEngine)
            , timers(logicEngine)
            , nodeBindings(logicEngine)
            , appearanceBindings(logicEngine)
            , cameraBindings(logicEngine)
            , renderPassBindings(logicEngine)
            , anchorPoints(logicEngine)
        {
        }

        sol::table views;

        NodeListWrapper<LuaInterface> interfaces;
        NodeListWrapper<LuaScript> scripts;
        NodeListWrapper<AnimationNode> animations;
        NodeListWrapper<TimerNode> timers;
        NodeListWrapper<RamsesNodeBinding> nodeBindings;
        NodeListWrapper<RamsesAppearanceBinding> appearanceBindings;
        NodeListWrapper<RamsesCameraBinding> cameraBindings;
        NodeListWrapper<RamsesRenderPassBinding> renderPassBindings;
        NodeListWrapper<AnchorPoint> anchorPoints;
    };

    const char* const LogicViewer::ltnModule     = "rlogic";
    const char* const LogicViewer::ltnScript     = "scripts";
    const char* const LogicViewer::ltnInterface  = "interfaces";
    const char* const LogicViewer::ltnAnimation  = "animationNodes";
    const char* const LogicViewer::ltnTimer      = "timerNodes";
    const char* const LogicViewer::ltnNode       = "nodeBindings";
    const char* const LogicViewer::ltnAppearance = "appearanceBindings";
    const char* const LogicViewer::ltnCamera     = "cameraBindings";
    const char* const LogicViewer::ltnRenderPass = "renderPassBindings";
    const char* const LogicViewer::ltnAnchorPoint = "anchorPoints";
    const char* const LogicViewer::ltnScreenshot = "screenshot";
    const char* const LogicViewer::ltnViews      = "views";
    const char* const LogicViewer::ltnLink       = "link";
    const char* const LogicViewer::ltnUnlink     = "unlink";
    const char* const LogicViewer::ltnUpdate     = "update";
    const char* const LogicViewer::ltnIN         = "IN";
    const char* const LogicViewer::ltnOUT        = "OUT";

    const char* const LogicViewer::ltnPropertyValue   = "value";
    const char* const LogicViewer::ltnViewUpdate      = "update";
    const char* const LogicViewer::ltnViewInputs      = "inputs";
    const char* const LogicViewer::ltnViewName        = "name";
    const char* const LogicViewer::ltnViewDescription = "description";

    LogicViewer::LogicViewer(EFeatureLevel engineFeatureLevel, ScreenshotFunc screenshotFunc)
        : m_logicEngine{ engineFeatureLevel }
        , m_screenshotFunc(std::move(screenshotFunc))
    {
        m_startTime = std::chrono::steady_clock::now();
    }

    bool LogicViewer::loadRamsesLogic(const std::string& filename, ramses::Scene* scene)
    {
        m_logicFilename = filename;
        return m_logicEngine.loadFromFile(filename, scene);
    }

    Result LogicViewer::loadLuaFile(const std::string& filename)
    {
        m_result = Result();
        m_sol = sol::state();
        m_sol.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::table, sol::lib::debug);
        m_sol.set_exception_handler(&solExceptionHandler);
        registerNodeListType<LuaInterface>(m_sol, "Interfaces");
        registerNodeListType<LuaScript>(m_sol, "LuaScripts");
        registerNodeListType<AnimationNode>(m_sol, "AnimationNodes");
        registerNodeListType<TimerNode>(m_sol, "TimerNodes");
        registerNodeListType<RamsesNodeBinding>(m_sol, "NodeBindings");
        registerNodeListType<RamsesAppearanceBinding>(m_sol, "AppearanceBindings");
        registerNodeListType<RamsesCameraBinding>(m_sol, "CameraBindings");
        registerNodeListType<RamsesRenderPassBinding>(m_sol, "RenderPassBindings");
        registerNodeListType<AnchorPoint>(m_sol, "AnchorPoints");
        m_sol.new_usertype<LogicNodeWrapper>("LogicNode", sol::meta_function::index, &LogicNodeWrapper::get, sol::meta_function::to_string, &LogicNodeWrapper::toString);
        m_sol.new_usertype<PropertyWrapper>("LogicProperty",
                                            ltnPropertyValue,
                                            sol::property(&PropertyWrapper::getValue, &PropertyWrapper::setValue),
                                            sol::meta_function::index,
                                            &PropertyWrapper::get,
                                            sol::meta_function::to_string,
                                            &PropertyWrapper::toString);
        m_sol.new_usertype<ConstPropertyWrapper>("ConstLogicProperty",
                                                 ltnPropertyValue,
                                                 sol::readonly_property(&ConstPropertyWrapper::getValue),
                                                 sol::meta_function::index,
                                                 &ConstPropertyWrapper::get,
                                                 sol::meta_function::to_string,
                                                 &ConstPropertyWrapper::toString);
        m_sol.new_usertype<LogicWrapper>(
            "RamsesLogic",
            sol::no_constructor,
            ltnInterface,
            sol::readonly(&LogicWrapper::interfaces),
            ltnScript,
            sol::readonly(&LogicWrapper::scripts),
            ltnAnimation,
            sol::readonly(&LogicWrapper::animations),
            ltnTimer,
            sol::readonly(&LogicWrapper::timers),
            ltnNode,
            sol::readonly(&LogicWrapper::nodeBindings),
            ltnAppearance,
            sol::readonly(&LogicWrapper::appearanceBindings),
            ltnCamera,
            sol::readonly(&LogicWrapper::cameraBindings),
            ltnRenderPass,
            sol::readonly(&LogicWrapper::renderPassBindings),
            ltnAnchorPoint,
            sol::readonly(&LogicWrapper::anchorPoints),
            ltnViews,
            &LogicWrapper::views,
            ltnScreenshot,
            [&](const std::string& screenshotFile) {
                updateEngine();
                return m_screenshotFunc(screenshotFile);
            },
            ltnUpdate,
            [&]() { updateEngine(); },
            ltnLink,
            [&](const ConstPropertyWrapper& src, const PropertyWrapper& target) { return m_logicEngine.link(src.m_property, target.m_property); },
            ltnUnlink,
            [&](const ConstPropertyWrapper& src, const PropertyWrapper& target) { return m_logicEngine.unlink(src.m_property, target.m_property); });

        m_sol[ltnModule] = LogicWrapper(m_logicEngine, m_sol);

        m_luaFilename  = filename;
        auto loadResult = m_sol.load_file(filename);
        if (!loadResult.valid())
        {
            sol::error err = loadResult;
            std::cerr << err.what() << std::endl;
            m_result = Result(err.what());
        }
        else
        {
            sol::protected_function mainFunction = loadResult;
            auto mainResult = mainFunction();
            if (!mainResult.valid())
            {
                sol::error err = mainResult;
                std::cerr << err.what() << std::endl;
                m_result = Result(err.what());
            }
        }
        return m_result;
    }

    Result LogicViewer::call(const std::string& functionName)
    {
        sol::protected_function protectedFunction = m_sol[functionName];
        auto result = protectedFunction();
        if (!result.valid())
        {
            sol::error err = result;
            m_result = Result(err.what());
        }
        return m_result;
    }

    Result LogicViewer::update()
    {
        updateEngine();
        // don't update if there's already an error
        if (m_result.ok())
        {
            sol::optional<sol::table> view = m_sol[ltnModule][ltnViews][m_view];
            if (view)
            {
                const auto elapsed   = std::chrono::steady_clock::now() - m_startTime;
                const auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
                sol::optional<sol::protected_function> func = (*view)[ltnViewUpdate];
                if (func)
                {
                    auto result = (*func)(millisecs);
                    if (!result.valid())
                    {
                        sol::error err = result;
                        std::cerr << err.what() << std::endl;
                        m_result = Result(err.what());
                        return m_result;
                    }
                }
                else
                {
                    m_result = Result("update() function is missing for current view");
                    return m_result;
                }
            }
        }
        return Result();
    }

    size_t LogicViewer::getViewCount() const
    {
        sol::optional<sol::table> tbl = m_sol[ltnModule][ltnViews];
        return tbl ? tbl->size() : 0U;
    }

    void LogicViewer::setCurrentView(size_t viewId)
    {
        if (viewId >= 1U && viewId <= getViewCount())
            m_view = viewId;
    }

    LogicViewer::View LogicViewer::getView(size_t viewId) const
    {
        sol::optional<sol::table> tbl = m_sol[ltnModule][ltnViews][viewId];
        return View(std::move(tbl));
    }

    void LogicViewer::updateEngine()
    {
        m_logicEngine.update();
        if (m_updateReportEnabled)
        {
            m_updateReportSummary.add(m_logicEngine.getLastUpdateReport());
        }
    }
}

