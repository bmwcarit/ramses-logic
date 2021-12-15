//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "LogicEngineTest_Base.h"

namespace rlogic
{
    class ALuaScript : public ALogicEngine
    {
    protected:
        std::string_view m_minimalScript = R"(
            function interface()
            end

            function run()
            end
        )";

        std::string_view m_minimalScriptWithInputs = R"(
            function interface()
                IN.speed = INT
                IN.speed2 = INT64
                IN.temp = FLOAT
                IN.name = STRING
                IN.enabled = BOOL
                IN.vec2f = VEC2F
                IN.vec3f = VEC3F
                IN.vec4f = VEC4F
                IN.vec2i = VEC2I
                IN.vec3i = VEC3I
                IN.vec4i = VEC4I
            end

            function run()
            end
        )";

        std::string_view m_minimalScriptWithOutputs = R"(
            function interface()
                OUT.speed = INT
                OUT.speed2 = INT64
                OUT.temp = FLOAT
                OUT.name = STRING
                OUT.enabled = BOOL
                OUT.vec2f = VEC2F
                OUT.vec3f = VEC3F
                OUT.vec4f = VEC4F
                OUT.vec2i = VEC2I
                OUT.vec3i = VEC3I
                OUT.vec4i = VEC4I
            end

            function run()
            end
        )";

        // Convenience type for testing
        struct LuaTestError
        {
            std::string errorCode;
            std::string expectedErrorMessage;
        };
    };
}
