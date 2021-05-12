//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-framework-api/RamsesFrameworkTypes.h"

#include <string_view>

namespace ramses
{
    class Scene;
    class Node;
    class Appearance;
    class Camera;
}

namespace rlogic::internal
{
    class IRamsesObjectResolver
    {
    public:
        virtual ~IRamsesObjectResolver() = default;

        [[nodiscard]] virtual ramses::Node* findRamsesNodeInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const = 0;
        [[nodiscard]] virtual ramses::Appearance* findRamsesAppearanceInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const = 0;
        [[nodiscard]] virtual ramses::Camera* findRamsesCameraInScene(std::string_view logicNodeName, ramses::sceneObjectId_t objectId) const = 0;
    };
}
