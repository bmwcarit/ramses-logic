//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/RamsesBindingImpl.h"

#include "ramses-client-api/SceneObject.h"

#include "generated/RamsesReferenceGen.h"

namespace rlogic::internal
{
    RamsesBindingImpl::RamsesBindingImpl(std::string_view name, uint64_t id) noexcept
        : LogicNodeImpl(name, id)
    {
    }

    flatbuffers::Offset<rlogic_serialization::RamsesReference> RamsesBindingImpl::SerializeRamsesReference(const ramses::SceneObject& object, flatbuffers::FlatBufferBuilder& builder)
    {
        const ramses::sceneObjectId_t ramsesObjectId = object.getSceneObjectId();
        const ramses::ERamsesObjectType ramsesObjectType = object.getType();

        auto ramsesRef = rlogic_serialization::CreateRamsesReference(builder,
            ramsesObjectId.getValue(),
            static_cast<uint32_t>(ramsesObjectType)
        );
        builder.Finish(ramsesRef);

        return ramsesRef;
    }
}
