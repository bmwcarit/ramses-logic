//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/LuaScript.h"
#include "ramses-logic/RamsesNodeBinding.h"
#include "ramses-logic/RamsesAppearanceBinding.h"
#include "ramses-logic/RamsesCameraBinding.h"

#include "impl/LogicEngineImpl.h"

#include <string>

namespace rlogic
{
    LogicEngine::LogicEngine() noexcept
        : m_impl(std::make_unique<internal::LogicEngineImpl>())
    {
    }

    LogicEngine::~LogicEngine() noexcept = default;

    LuaScript* LogicEngine::createLuaScriptFromFile(std::string_view filename, std::string_view scriptName)
    {
        return m_impl->createLuaScriptFromFile(filename, scriptName);
    }

    Collection<LuaScript> LogicEngine::scripts() const
    {
        return Collection<LuaScript>(m_impl->getApiObjects().getScripts());
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

    LuaScript* LogicEngine::createLuaScriptFromSource(std::string_view source, std::string_view scriptName)
    {
        return m_impl->createLuaScriptFromSource(source, scriptName);
    }

    RamsesNodeBinding* LogicEngine::createRamsesNodeBinding(ramses::Node& ramsesNode, std::string_view name)
    {
        return m_impl->createRamsesNodeBinding(ramsesNode, name);
    }

    bool LogicEngine::destroy(LogicNode& logicNode)
    {
        return m_impl->destroy(logicNode);
    }

    RamsesAppearanceBinding* LogicEngine::createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name)
    {
        return m_impl->createRamsesAppearanceBinding(ramsesAppearance, name);
    }

    RamsesCameraBinding* LogicEngine::createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name)
    {
        return m_impl->createRamsesCameraBinding(ramsesCamera, name);
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
}
