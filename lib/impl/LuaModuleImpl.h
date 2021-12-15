//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "impl/LogicObjectImpl.h"
#include "internals/LuaCompilationUtils.h"
#include "internals/SolWrapper.h"
#include <string>

namespace rlogic_serialization
{
    struct LuaModule;
}

namespace flatbuffers
{
    template<typename T> struct Offset;
    class FlatBufferBuilder;
}

namespace rlogic
{
    class LuaModule;
}

namespace rlogic::internal
{
    class ErrorReporting;
    class SolState;
    class DeserializationMap;
    class SerializationMap;

    class LuaModuleImpl : public LogicObjectImpl
    {
    public:
        LuaModuleImpl(LuaCompiledModule module, std::string_view name, uint64_t id);

        [[nodiscard]] std::string_view getSourceCode() const;
        [[nodiscard]] const sol::table& getModule() const;
        [[nodiscard]] const ModuleMapping& getDependencies() const;

        [[nodiscard]] static flatbuffers::Offset<rlogic_serialization::LuaModule> Serialize(
            const LuaModuleImpl& module,
            flatbuffers::FlatBufferBuilder& builder,
            SerializationMap& serializationMap);

        [[nodiscard]] static std::unique_ptr<LuaModuleImpl> Deserialize(
            SolState& solState,
            const rlogic_serialization::LuaModule& module,
            ErrorReporting& errorReporting,
            DeserializationMap& deserializationMap);

    private:
        std::string m_sourceCode;
        sol::table m_module;
        ModuleMapping m_dependencies;
        StandardModules m_stdModules;
    };
}
