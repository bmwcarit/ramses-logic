//  -------------------------------------------------------------------------
//  Copyright (C) 2021 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "internals/SolWrapper.h"
#include <string>
#include <memory>
#include <optional>

namespace rlogic
{
    class Property;
}

namespace rlogic::internal
{
    class SolState;
    class ErrorReporting;

    struct LuaCompiledScript
    {
        // Metadata
        std::string sourceCode;
        std::string fileName;

        // Which Lua/sol environment holds the compiled function
        std::reference_wrapper<SolState> solState;
        // The main function (holding interface() and run() functions)
        sol::protected_function mainFunction;

        // Parsed interface properties
        std::unique_ptr<Property> rootInput;
        std::unique_ptr<Property> rootOutput;
    };

    class LuaCompilationUtils
    {
    public:
        [[nodiscard]] static std::optional<LuaCompiledScript> Compile(
            SolState& solState,
            std::string source,
            std::string_view scriptName,
            std::string filename,
            ErrorReporting& errorReporting);

    private:
        [[nodiscard]] static std::string BuildChunkName(std::string_view scriptName, std::string_view fileName);
    };
}
