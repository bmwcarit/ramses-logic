//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#include "ramses-logic/LogicEngine.h"
#include "ramses-logic/Property.h"
#include <iostream>

/**
 * This example demonstrates how to override the default Lua print() function
 */
int main()
{
    rlogic::LogicEngine logicEngine;

    // Create a simple script which prints a debug message
    rlogic::LuaScript* script = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.debug_message = STRING
        end

        function run()
            print(IN.debug_message)
        end
    )", "MyScript");

    // Override Lua's print() function so that we can get the result in C++
    script->overrideLuaPrint(
        [](std::string_view scriptName, std::string_view message)
        {
            std::cout << "From C++: script '" << scriptName << "' printed message '" << message << "'!\n";
        }
    );

    // Set debug message text
    script->getInputs()->getChild("debug_message")->set<std::string>("hello!");

    // update() will execute the script and call the custom lambda function we passed above
    logicEngine.update();

    // To delete the script we need to call the destroy method on LogEngine
    logicEngine.destroy(*script);

    return 0;
}
