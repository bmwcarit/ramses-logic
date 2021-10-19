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

    Collection<LuaScript> LogicEngine::scripts() const
    {
        return Collection<LuaScript>(m_impl->getApiObjects().getScripts());
    }

    Collection<LuaModule> LogicEngine::luaModules() const
    {
        return Collection<LuaModule>(m_impl->getApiObjects().getLuaModules());
    }

    Collection<RamsesNodeBinding> LogicEngine::ramsesNodeBindings() const
    {
        return Collection<RamsesNodeBinding>(m_impl->getApiObjects().getNodeBindings());
    }

    Collection<RamsesAppearanceBinding> LogicEngine::ramsesAppearanceBindings() const
    {
        return Collection<RamsesAppearanceBinding>(m_impl->getApiObjects().getAppearanceBindings());
    }

    Collection<RamsesCameraBinding> LogicEngine::ramsesCameraBindings() const
    {
        return Collection<RamsesCameraBinding>(m_impl->getApiObjects().getCameraBindings());
    }

    Collection<DataArray> LogicEngine::dataArrays() const
    {
        return Collection<DataArray>(m_impl->getApiObjects().getDataArrays());
    }

    Collection<rlogic::AnimationNode> LogicEngine::animationNodes() const
    {
        return Collection<AnimationNode>(m_impl->getApiObjects().getAnimationNodes());
    }

    template <typename T>
    const T* findObject(const std::vector<std::unique_ptr<T>>& container, std::string_view name)
    {
        const auto it = std::find_if(container.cbegin(), container.cend(), [name](const auto& o) {
            return o->getName() == name; });

        return (it == container.cend() ? nullptr : it->get());
    }

    template <typename T>
    T* findObject(std::vector<std::unique_ptr<T>>& container, std::string_view name)
    {
        const auto it = std::find_if(container.begin(), container.end(), [name](const auto& o) {
            return o->getName() == name; });

        return (it == container.end() ? nullptr : it->get());
    }

    const LuaScript* LogicEngine::findScript(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getScripts(), name);
    }
    LuaScript* LogicEngine::findScript(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getScripts(), name);
    }

    const LuaModule* LogicEngine::findLuaModule(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getLuaModules(), name);
    }
    LuaModule* LogicEngine::findLuaModule(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getLuaModules(), name);
    }

    const RamsesNodeBinding* LogicEngine::findNodeBinding(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getNodeBindings(), name);
    }
    RamsesNodeBinding* LogicEngine::findNodeBinding(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getNodeBindings(), name);
    }

    const RamsesAppearanceBinding* LogicEngine::findAppearanceBinding(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getAppearanceBindings(), name);
    }
    RamsesAppearanceBinding* LogicEngine::findAppearanceBinding(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getAppearanceBindings(), name);
    }

    const RamsesCameraBinding* LogicEngine::findCameraBinding(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getCameraBindings(), name);
    }
    RamsesCameraBinding* LogicEngine::findCameraBinding(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getCameraBindings(), name);
    }

    const DataArray* LogicEngine::findDataArray(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getDataArrays(), name);
    }
    DataArray* LogicEngine::findDataArray(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getDataArrays(), name);
    }

    const AnimationNode* LogicEngine::findAnimationNode(std::string_view name) const
    {
        return findObject(m_impl->getApiObjects().getAnimationNodes(), name);
    }
    AnimationNode* LogicEngine::findAnimationNode(std::string_view name)
    {
        return findObject(m_impl->getApiObjects().getAnimationNodes(), name);
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

    const std::vector<ErrorData>& LogicEngine::getErrors() const
    {
        return m_impl->getErrors();
    }

    bool LogicEngine::update()
    {
        return m_impl->update();
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

    bool LogicEngine::unlink(const Property& sourceProperty, const Property& targetProperty)
    {
        return m_impl->unlink(sourceProperty, targetProperty);
    }

    bool LogicEngine::isLinked(const LogicNode& logicNode) const
    {
        return m_impl->isLinked(logicNode);
    }

    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<float>(const std::vector<float>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec2f>(const std::vector<vec2f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec3f>(const std::vector<vec3f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec4f>(const std::vector<vec4f>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<int32_t>(const std::vector<int32_t>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec2i>(const std::vector<vec2i>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec3i>(const std::vector<vec3i>&, std::string_view);
    template RLOGIC_API DataArray* LogicEngine::createDataArrayInternal<vec4i>(const std::vector<vec4i>&, std::string_view);
}
