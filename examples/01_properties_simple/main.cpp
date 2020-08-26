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
 * This example demonstrates basic functionality of the LuaScript class:
 * - creating properties of primitive types
 * - simple debugging by overloading the print() method
 */
int main()
{
    rlogic::LogicEngine logicEngine;

    // Create a simple script which does some simple math and optionally prints a debug message
    rlogic::LuaScript* script = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.param1 = INT
            IN.param2 = FLOAT
            IN.enable_debug = BOOL
            IN.debug_message = STRING

            OUT.result = FLOAT
        end

        function run()
            OUT.result = IN.param1 * IN.param2

            if IN.enable_debug then
                print(IN.debug_message)
            end
        end
    )", "MyScript");

    // Override Lua's print() function so that we can get the result in C++
    script->overrideLuaPrint(
        [](std::string_view scriptName, std::string_view message)
        {
            std::cout << "From C++: script '" << scriptName << "' printed message '" << message << "'!\n";
        }
    );

    /**
     * Query the inputs of the script. The inputs are
     * stored in a "Property" instance and can be used to
     * get the information about available inputs and outputs
     */
    rlogic::Property* inputs = script->getInputs();

    for (size_t i = 0u; i < inputs->getChildCount(); ++i)
    {
        rlogic::Property* input = inputs->getChild(i);
        std::cout << "Input: " << input->getName() << " of type: " << rlogic::GetLuaPrimitiveTypeName(input->getType()) << std::endl;
    }

    // We can do the same with the outputs
    const rlogic::Property* outputs = script->getOutputs();

    for (size_t i = 0u; i < outputs->getChildCount(); ++i)
    {
        const rlogic::Property* output = outputs->getChild(i);
        std::cout << "Output: " << output->getName() << " of type: " << rlogic::GetLuaPrimitiveTypeName(output->getType()) << std::endl;
    }

    // Set some test values to the inputs before executing the script
    inputs->getChild("param1")->set<int32_t>(21);
    inputs->getChild("param2")->set<float>(2.f);
    inputs->getChild("enable_debug")->set<bool>(true);
    inputs->getChild("debug_message")->set<std::string>("hello!");

    // Update the logic engine including our script
    logicEngine.update();

    /**
     * After execution, we can get the calculated outputs
     * The getters of the values are returned as std::optional
     * to ensure the combination of name and type matches an existing input.
     */
    auto result = outputs->getChild("result")->get<float>();
    if (result)
    {
        std::cout << "Calculated result is: " << *result << std::endl;
    }

    // To delete the script we need to call the destroy method on LogEngine
    logicEngine.destroy(*script);

    return 0;
}
