//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"

#include "ramses-logic/LuaScript.h"
#include "ramses-logic/LuaModule.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"
#include "ramses-logic/DataArray.h"
#include "ramses-logic/AnimationNode.h"
#include "ramses-logic/TimerNode.h"

#include "impl/LogicEngineImpl.h"
#include "impl/LuaConfigImpl.h"
#include "internals/ApiObjects.h"

#include <string>

namespace rlogic
{
    LogicEngine::LogicEngine() noexcept
        : m_impl(std::make_unique<internal::LogicEngineImpl>())
    {
    }

    LogicEngine::~LogicEngine() noexcept = default;

    LogicEngine::LogicEngine(LogicEngine&& other) noexcept = default;

    LogicEngine& LogicEngine::operator=(LogicEngine&& other) noexcept = default;

    template <typename T>
    Collection<T> LogicEngine::getLogicObjectsInternal() const
    {
        return Collection<T>(m_impl->getApiObjects().getApiObjectContainer<T>());
    }

    template <typename T>
    const T* findObject(const internal::ApiObjects& apiObjects, std::string_view name)
    {
        const auto& container = apiObjects.getApiObjectContainer<T>();
        const auto it = std::find_if(container.cbegin(), container.cend(), [name](const auto& o) {
            return o->getName() == name; });

        return (it == container.cend() ? nullptr : *it);
    }

    template <typename T>
    const T* LogicEngine::findLogicObjectInternal(std::string_view name) const
    {
        auto& container = m_impl->getApiObjects().getApiObjectContainer<T>();
        const auto it = std::find_if(container.begin(), container.end(), [name](const auto& o) {
            return o->getName() == name; });

        return (it == container.end() ? nullptr : *it);
    }

    template <typename T>
    T* LogicEngine::findLogicObjectInternal(std::string_view name)
    {
        auto& container = m_impl->getApiObjects().getApiObjectContainer<T>();
        const auto it = std::find_if(container.begin(), container.end(), [name](const auto& o) {
            return o->getName() == name; });

        return (it == container.end() ? nullptr : *it);
    }

    const LogicObject* LogicEngine::findLogicObjectById(uint64_t id) const
    {
        return m_impl->getApiObjects().getApiObjectById(id);
    }

    LogicObject* LogicEngine::findLogicObjectById(uint64_t id)
    {
        return m_impl->getApiObjects().getApiObjectById(id);
    }

    LuaScript* LogicEngine::createLuaScript(std::string_view source, const LuaConfig& config, std::string_view scriptName)
    {
        return m_impl->createLuaScript(source, *config.m_impl, scriptName);
    }

    LuaModule* LogicEngine::createLuaModule(std::string_view source, const LuaConfig& config, std::string_view moduleName)
    {
        return m_impl->createLuaModule(source, *config.m_impl, moduleName);
    }

    bool LogicEngine::extractLuaDependencies(std::string_view source, const std::function<void(const std::string&)>& callbackFunc)
    {
        return m_impl->extractLuaDependencies(source, callbackFunc);
    }

    RamsesNodeBinding* LogicEngine::createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType /* = ERotationType::Euler_XYZ*/, std::string_view name)
    {
        return m_impl->createRamsesNodeBinding(ramsesNode, rotationType, name);
    }

    bool LogicEngine::destroy(LogicObject& object)
    {
        return m_impl->destroy(object);
    }

    RamsesAppearanceBinding* LogicEngine::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        return m_impl->createRamsesAppearanceBinding(ramsesAppearance, name);
    }

    RamsesCameraBinding* LogicEngine::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        return m_impl->createRamsesCameraBinding(ramsesCamera, name);
    }

    template <typename T>
    DataArray* LogicEngine::createDataArrayInternal(const std::vector<T>& data, std::string_view name)
    {
        static_assert(IsPrimitiveProperty<T>::value && CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE));
        return m_impl->createDataArray(data, name);
    }

    AnimationNode* LogicEngine::createAnimationNode(const AnimationChannels& channels, std::string_view name)
    {
        return m_impl->createAnimationNode(channels, name);
    }

    TimerNode* LogicEngine::createTimerNode(std::string_view name)
    {
        return m_impl->createTimerNode(name);
    }

    const std::vector<ErrorData>& LogicEngine::getErrors() const
    {
        return m_impl->getErrors();
    }

    bool LogicEngine::update()
    {
        return m_impl->update();
    }

    void LogicEngine::enableUpdateReport(bool enable)
    {
        m_impl->enableUpdateReport(enable);
    }

    LogicEngineReport LogicEngine::getLastUpdateReport() const
    {
        return m_impl->getLastUpdateReport();
    }

    void LogicEngine::setStatisticsLoggingRate(size_t loggingRate)
    {
        m_impl->setStatisticsLoggingRate(loggingRate);
    }

    void LogicEngine::setStatisticsLogLevel(ELogMessageType logLevel)
    {
        m_impl->setStatisticsLogLevel(logLevel);
    }

    bool LogicEngine::loadFromFile(std::string_view filename, ramses::Scene* ramsesScene /* = nullptr*/, bool enableMemoryVerification /* = true */)
    {
        return m_impl->loadFromFile(filename, ramsesScene, enableMemoryVerification);
    }

    bool LogicEngine::loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* ramsesScene /* = nullptr*/, bool enableMemoryVerification /* = true */)
    {
        return m_impl->loadFromBuffer(rawBuffer, bufferSize, ramsesScene, enableMemoryVerification);
    }

    bool LogicEngine::saveToFile(std::string_view filename)
    {
        return m_impl->saveToFile(filename);
    }

    bool LogicEngine::link(const Property& sourceProperty, const Property& targetProperty)
    {
        return m_impl->link(sourceProperty, targetProperty);
    }

    bool LogicEngine::linkWeak(const Property& sourceProperty, const Property& targetProperty)
    {
        return m_impl->linkWeak(sourceProperty, targetProperty);
    }

    bool LogicEngine::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        return m_impl->unlink(sourceProperty, targetProperty);
    }

    bool LogicEngine::isLinked(const LogicNode& logicNode) const
    {
        return m_impl->isLinked(logicNode);
    }

    template RLOGIC_API Collection<LogicObject>             LogicEngine::getLogicObjectsInternal<LogicObject>() const;
    template RLOGIC_API Collection<LuaScript>               LogicEngine::getLogicObjectsInternal<LuaScript>() const;
    template RLOGIC_API Collection<LuaModule>               LogicEngine::getLogicObjectsInternal<LuaModule>() const;
    template RLOGIC_API Collection<RamsesNodeBinding>       LogicEngine::getLogicObjectsInternal<RamsesNodeBinding>() const;
    template RLOGIC_API Collection<RamsesAppearanceBinding> LogicEngine::getLogicObjectsInternal<RamsesAppearanceBinding>() const;
    template RLOGIC_API Collection<RamsesCameraBinding>     LogicEngine::getLogicObjectsInternal<RamsesCameraBinding>() const;
    template RLOGIC_API Collection<DataArray>               LogicEngine::getLogicObjectsInternal<DataArray>() const;
    template RLOGIC_API Collection<AnimationNode>           LogicEngine::getLogicObjectsInternal<AnimationNode>() const;
    template RLOGIC_API Collection<TimerNode>               LogicEngine::getLogicObjectsInternal<TimerNode>() const;

    template RLOGIC_API const LogicObject*             LogicEngine::findLogicObjectInternal<LogicObject>(std::string_view) const;
    template RLOGIC_API const LuaScript*               LogicEngine::findLogicObjectInternal<LuaScript>(std::string_view) const;
    template RLOGIC_API const LuaModule*               LogicEngine::findLogicObjectInternal<LuaModule>(std::string_view) const;
    template RLOGIC_API const RamsesNodeBinding*       LogicEngine::findLogicObjectInternal<RamsesNodeBinding>(std::string_view) const;
    template RLOGIC_API const RamsesAppearanceBinding* LogicEngine::findLogicObjectInternal<RamsesAppearanceBinding>(std::string_view) const;
    template RLOGIC_API const RamsesCameraBinding*     LogicEngine::findLogicObjectInternal<RamsesCameraBinding>(std::string_view) const;
    template RLOGIC_API const DataArray*               LogicEngine::findLogicObjectInternal<DataArray>(std::string_view) const;
    template RLOGIC_API const AnimationNode*           LogicEngine::findLogicObjectInternal<AnimationNode>(std::string_view) const;
    template RLOGIC_API const TimerNode*               LogicEngine::findLogicObjectInternal<TimerNode>(std::string_view) const;

    template RLOGIC_API LogicObject*             LogicEngine::findLogicObjectInternal<LogicObject>(std::string_view);
    template RLOGIC_API LuaScript*               LogicEngine::findLogicObjectInternal<LuaScript>(std::string_view);
    template RLOGIC_API LuaModule*               LogicEngine::findLogicObjectInternal<LuaModule>(std::string_view);
    template RLOGIC_API RamsesNodeBinding*       LogicEngine::findLogicObjectInternal<RamsesNodeBinding>(std::string_view);
    template RLOGIC_API RamsesAppearanceBinding* LogicEngine::findLogicObjectInternal<RamsesAppearanceBinding>(std::string_view);
    template RLOGIC_API RamsesCameraBinding*     LogicEngine::findLogicObjectInternal<RamsesCameraBinding>(std::string_view);
    template RLOGIC_API DataArray*               LogicEngine::findLogicObjectInternal<DataArray>(std::string_view);
    template RLOGIC_API AnimationNode*           LogicEngine::findLogicObjectInternal<AnimationNode>(std::string_view);
    template RLOGIC_API TimerNode*               LogicEngine::findLogicObjectInternal<TimerNode>(std::string_view);

    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<float>(const std::vector<float>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec2f>(const std::vector<vec2f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec3f>(const std::vector<vec3f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec4f>(const std::vector<vec4f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<int32_t>(const std::vector<int32_t>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec2i>(const std::vector<vec2i>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec3i>(const std::vector<vec3i>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec4i>(const std::vector<vec4i>&, std::string_view);
}
