//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include <cassert>
#include <iostream>

/**
 * This example shows how to deal with runtime errors in Lua scripts.
 */
int main()
{
    rlogic::LogicEngine logicEngine;

    /**
     * This script contains a runtime error, i.e. from Lua point of view this is
     * valid syntax, but the type check of the Logic engine will fire a runtime
     * error for trying to assign a string to a VEC4F property
     */
    rlogic::LuaScript* script = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            OUT.vec4f = VEC4F
        end

        function run()
            OUT.vec4f = "this is not a table with 4 floats and will trigger a runtime error!"
        end
    )", "FaultyScript");

    // Script is successfully created, as it is syntactically correct
    assert(script != nullptr);

    /**
     * Update the logic engine including our script
     * Because there is a runtime error in the script, the execution will return "false"
     */
    assert(!logicEngine.update());

    // To get further information about the issue, fetch errors from LogicEngine
    auto errors = logicEngine.getErrors();
    assert(!errors.empty());

    // Print out the error information
    for (const auto& error : errors)
    {
        /**
         * The stack trace is coming from the Lua VM and has limited information on the error. See the
         * docs at https://genivi.github.io/ramses-logic/api.html#additional-lua-syntax-specifics for more information
         */
        std::cout << error << std::endl;
    }

    logicEngine.destroy(*script);

    return 0;
}
