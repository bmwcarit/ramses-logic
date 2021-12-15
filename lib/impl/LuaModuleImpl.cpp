//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "impl/LuaModuleImpl.h"

#include "ramses-logic/LuaModule.h"

#include "internals/LuaCompilationUtils.h"
#include "generated/LuaModuleGen.h"
#include "flatbuffers/flatbuffers.h"
#include "internals/SolState.h"
#include "internals/ErrorReporting.h"
#include "internals/DeserializationMap.h"
#include "internals/SerializationMap.h"
#include "internals/FileFormatVersions.h"
#include <fmt/format.h>

namespace rlogic::internal
{
    LuaModuleImpl::LuaModuleImpl(LuaCompiledModule module, std::string_view name, uint64_t id)
        : LogicObjectImpl(name, id)
        , m_sourceCode{ std::move(module.source.sourceCode) }
        , m_module{ std::move(module.moduleTable) }
        , m_dependencies{std::move(module.source.userModules)}
        , m_stdModules {std::move(module.source.stdModules)}
    {
        assert(m_module != sol::nil);
    }

    std::string_view LuaModuleImpl::getSourceCode() const
    {
        return m_sourceCode;
    }

    const sol::table& LuaModuleImpl::getModule() const
    {
        return m_module;
    }

    flatbuffers::Offset<rlogic_serialization::LuaModule> LuaModuleImpl::Serialize(const LuaModuleImpl& module, flatbuffers::FlatBufferBuilder& builder, SerializationMap& serializationMap)
    {
        std::vector<flatbuffers::Offset<rlogic_serialization::LuaModuleUsage>> modulesFB;
        modulesFB.reserve(module.m_dependencies.size());
        for (const auto& dependency : module.m_dependencies)
        {
            modulesFB.push_back(
                rlogic_serialization::CreateLuaModuleUsage(builder,
                    builder.CreateString(dependency.first),
                    serializationMap.resolveLuaModuleOffset(*dependency.second)));
        }

        std::vector<uint8_t> stdModules;
        stdModules.reserve(module.m_stdModules.size());
        for (const EStandardModule stdModule : module.m_stdModules)
        {
            stdModules.push_back(static_cast<uint8_t>(stdModule));
        }

        return rlogic_serialization::CreateLuaModule(builder,
            builder.CreateString(module.getName()),
            module.getId(),
            builder.CreateString(module.getSourceCode()),
            builder.CreateVector(modulesFB),
            builder.CreateVector(stdModules)
        );
    }

    std::unique_ptr<LuaModuleImpl> LuaModuleImpl::Deserialize(
        SolState& solState,
        const rlogic_serialization::LuaModule& module,
        ErrorReporting& errorReporting,
        DeserializationMap& deserializationMap)
    {
        if (!module.name())
        {
            errorReporting.add("Fatal error during loading of LuaModule from serialized data: missing name!", nullptr);
            return nullptr;
        }

        if (module.id() == 0u)
        {
            errorReporting.add("Fatal error during loading of LuaModule from serialized data: missing id!", nullptr);
            return nullptr;
        }

        if (!module.source())
        {
            errorReporting.add("Fatal error during loading of LuaModule from serialized data: missing source code!", nullptr);
            return nullptr;
        }

        if (!module.dependencies())
        {
            errorReporting.add("Fatal error during loading of LuaModule from serialized data: missing dependencies!", nullptr);
            return nullptr;
        }

        const std::string_view name = module.name()->string_view();
        std::string source = module.source()->str();

        StandardModules stdModules;
        stdModules.reserve(module.standardModules()->size());
        for (const uint8_t stdModule : *module.standardModules())
        {
            stdModules.push_back(static_cast<EStandardModule>(stdModule));
        }

        ModuleMapping modulesUsed;
        modulesUsed.reserve(module.dependencies()->size());
        for (const auto* mod : *module.dependencies())
        {
            if (!mod->name() || !mod->module_())
            {
                errorReporting.add(fmt::format("Fatal error during loading of LuaModule '{}' module data: missing name or module!", name), nullptr);
                return nullptr;
            }
            const LuaModule& moduleUsed = deserializationMap.resolveLuaModule(*mod->module_());
            modulesUsed.emplace(mod->name()->str(), &moduleUsed);
        }

        std::optional<LuaCompiledModule> compiledModule = LuaCompilationUtils::CompileModule(solState, modulesUsed, stdModules, source, name, errorReporting);
        if (!compiledModule)
        {
            errorReporting.add(fmt::format("Fatal error during loading of LuaModule '{}' from serialized data: failed parsing Lua module source code.", name), nullptr);
            return nullptr;
        }

        return std::make_unique<LuaModuleImpl>(
            std::move(*compiledModule),
            name, module.id());
    }

    const ModuleMapping& LuaModuleImpl::getDependencies() const
    {
        return m_dependencies;
    }
}
