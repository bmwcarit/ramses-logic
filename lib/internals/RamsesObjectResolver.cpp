//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "internals/RamsesObjectResolver.h"

#include "internals/ErrorReporting.h"

#include "ramses-client-api/SceneObject.h"
#include "ramses-client-api/Scene.h"
#include "ramses-client-api/Node.h"
#include "ramses-client-api/Appearance.h"
#include "ramses-client-api/Camera.h"
#include "ramses-utils.h"

#include "fmt/format.h"

namespace rlogic::internal
{
    RamsesObjectResolver::RamsesObjectResolver(ErrorReporting& errorReporting, ramses::Scene* scene)
        : m_errors(errorReporting)
        , m_scene(scene)
    {
    }

    ramses::SceneObject* RamsesObjectResolver::findRamsesSceneObjectInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const
    {
        if (nullptr == m_scene)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized Ramses Logic object '{}' points to a Ramses object (id: {}), but no Ramses scene was provided to resolve the Ramses object!",
                            logicNodeName,
                            objectId.getValue()
                    ));
            return nullptr;
        }

        ramses::SceneObject* sceneObject = m_scene->findObjectById(objectId);

        if (nullptr == sceneObject)
        {
            m_errors.add(
                fmt::format("Fatal error during loading from file! Serialized Ramses Logic object '{}' points to a Ramses object (id: {}) which couldn't be found in the provided scene!",
                            logicNodeName,
                            objectId.getValue()));
            return nullptr;
        }

        return sceneObject;
    }

    ramses::Node* RamsesObjectResolver::findRamsesNodeInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNodeName, objectId);

        if (nullptr == sceneObject)
        {
            return nullptr;
        }

        auto* ramsesNode = ramses::RamsesUtils::TryConvert<ramses::Node>(*sceneObject);

        if (nullptr == ramsesNode)
        {
            m_errors.add("Fatal error during loading from file! Node binding points to a Ramses scene object which is not of type 'Node'!");
            return nullptr;
        }

        return ramsesNode;
    }

    ramses::Appearance* RamsesObjectResolver::findRamsesAppearanceInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNodeName, objectId);

        if (nullptr == sceneObject)
        {
            return nullptr;
        }

        auto* ramsesAppearance = ramses::RamsesUtils::TryConvert<ramses::Appearance>(*sceneObject);

        if (nullptr == ramsesAppearance)
        {
            m_errors.add("Fatal error during loading from file! Appearance binding points to a Ramses scene object which is not of type 'Appearance'!");
            return nullptr;
        }

        return ramsesAppearance;
    }

    ramses::Camera* RamsesObjectResolver::findRamsesCameraInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const
    {
        ramses::SceneObject* sceneObject = findRamsesSceneObjectInScene(logicNodeName, objectId);

        if (nullptr == sceneObject)
        {
            return nullptr;
        }

        auto* ramsesCamera = ramses::RamsesUtils::TryConvert<ramses::Camera>(*sceneObject);

        if (nullptr == ramsesCamera)
        {
            m_errors.add("Fatal error during loading from file! Camera binding points to a Ramses scene object which is not of type 'Camera'!");
            return nullptr;
        }

        return ramsesCamera;
    }
}
