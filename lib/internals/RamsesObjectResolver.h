//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/IRamsesObjectResolver.h"

namespace ramses
{
    class SceneObject;
}

namespace rlogic::internal
{
    class ErrorReporting;

    class RamsesObjectResolver : public IRamsesObjectResolver
    {
    public:
        explicit RamsesObjectResolver(ErrorReporting& errorReporting, ramses::Scene* scene);

        // TODO Violin remove duplications between these methods
        [[nodiscard]] ramses::Node* findRamsesNodeInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const override;
        [[nodiscard]] ramses::Appearance* findRamsesAppearanceInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const override;
        [[nodiscard]] ramses::Camera* findRamsesCameraInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const override;

    private:
        ErrorReporting& m_errors;
        ramses::Scene* m_scene;

        [[nodiscard]] ramses::SceneObject* findRamsesSceneObjectInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const;
    };
}
