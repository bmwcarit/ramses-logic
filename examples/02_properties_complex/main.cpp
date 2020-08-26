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
#include <cassert>


/**
 * This example demonstrates more complex data structures and possible
 * ways to interact with them: structs, nested structs, vector properties
 */

// Convenience function which pretty-prints the contents of property to stdout
void printStruct(const rlogic::Property* property, size_t identation = 0);

int main()
{
    rlogic::LogicEngine logicEngine;

    // Create a script with property structure containing nested data and vector properties
    rlogic::LuaScript* script = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.struct = {
                vec2i = VEC2I,
                nested = {
                    vec4f = VEC4F
                }
            }

            OUT.struct = {
                vec2i = VEC2I,
                nested = {
                    vec4f = VEC4F
                }
            }
        end

        function run()
            -- can assign whole structs if both sides are of compatible types
            OUT.struct = IN.struct

            -- Can assign a nested struct too
            OUT.struct.nested = IN.struct.nested

            -- This assigns a single vec2i component-wise. Notice the indexing of vec2i - it follows Lua conventions (starts by 1)
            OUT.struct.vec2i = {
                IN.struct.vec2i[1],
                IN.struct.vec2i[2]
            }

            -- This is equivalent to the above statement
            -- Note: you can't assign a single vecXY component - you have to set all of them atomically
            -- When using this notation, you can reorder indices, but ultimately all vecNt types must have
            -- exactly N[2|3|4] components of type t[i|f]
            OUT.struct.vec2i = {
                [2] = IN.struct.vec2i[2],
                [1] = IN.struct.vec2i[1]
            }
        end
    )");

    /**
     * Set some data on the inputs
     * Note that with this notation (using C++ initializer lists) it is possible to accidentally
     * provide fewer entries than the vector expects. This will result in zeroes filling the unspecified slots
     */
    script->getInputs()->getChild("struct")->getChild("vec2i")->set<rlogic::vec2i>({ 1, 2 });
    script->getInputs()->getChild("struct")->getChild("nested")->getChild("vec4f")->set<rlogic::vec4f>({ 1.1f, 1.2f, 1.3f, 1.4f });

    // Update the logic engine including our script
    logicEngine.update();

    // Inspect the results of the script
    printStruct(script->getOutputs());

    logicEngine.destroy(*script);

    return 0;
}


void printStruct(const rlogic::Property* property, size_t identation)
{
    std::cout << std::string(identation * 2, ' ') << "Property: " << property->getName() << " of type: " << GetLuaPrimitiveTypeName(property->getType()) << " with value: ";

    // Here, we only handle the types used in this example. Usually, you'd want to handle all types in a real world application
    if (property->getType() == rlogic::EPropertyType::Vec2i)
    {
        std::cout << "vec2i[" << (*property->get<rlogic::vec2i>())[0] << ", " << (*property->get<rlogic::vec2i>())[1] << "]";
    }
    else if (property->getType() == rlogic::EPropertyType::Vec4f)
    {
        std::cout << "vec4f[" << (*property->get<rlogic::vec4f>())[0] << ", " << (*property->get<rlogic::vec4f>())[1] <<
            ", " << (*property->get<rlogic::vec4f>())[2] << ", " << (*property->get<rlogic::vec4f>())[3] << "]";
    }
    else if (property->getType() == rlogic::EPropertyType::Struct)
    {
        // Structs don't have a value! Trying to call get() will result in error
        std::cout << "None";
    }
    else
    {
        assert(false && "Type not handled in this example!");
    }

    std::cout << std::endl;

    for (size_t i = 0u; i < property->getChildCount(); ++i)
    {
        const rlogic::Property* child = property->getChild(i);
        printStruct(child, identation + 1);
    }
}
