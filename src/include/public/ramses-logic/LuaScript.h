//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/LogicNode.h"

#include <string>
#include <memory>

namespace rlogic::internal
{
    class LuaScriptImpl;
}

namespace rlogic
{
    class Property;

    using LuaPrintFunction = std::function<void(std::string_view scriptName, std::string_view message)>;

    /**
    * The LuaScript class is the cornerstone of RAMSES Logic as it encapsulates
    * a Lua script and the associated with it Lua environment. LuaScript instances are created
    * by the LogicEngine class.
    *
    * A LuaScript can be created from Lua source code which must fulfill following requirements:
    *
    * \rst
    * .. note:: TODO discuss whether we want to introduce further checks - e.g. must have at least one input and one output
    * \endrst
    *
    * - valid Lua 5.1 syntax
    * - contains two global functions - interface() and run() with no parameters and no return values
    * - declares its inputs and outputs in the interface() function, and its logic in the run() function
    * - the interface() function declares zero or more inputs and outputs to the IN and OUT global symbols
    * - inputs and outputs are declared like this:
    *   \code{.lua}
    *       function interface()
    *           IN.input_name = TYPE
    *           OUT.output_name = TYPE
    *       end
    *   \endcode
    * - TYPE is one of [INT|FLOAT|BOOL|STRING|VEC2F|VEC3F|VEC4F|VEC2I|VEC3I|VEC4I]
    * - Each property must have a name (string) - other types like number, bool etc. are not supported as keys
    * - TYPE can be also a Lua table with nested properties, obeying the same rules as above
    * - the run() function only accesses the IN and OUT global symbols and the properties defined by it
    *
    * Violating any of these requirements will result in errors, which can be obtained either by calling
    * LogicEngine::getErrors() (for compile-time errors) or LuaScript::getErrors() for runtime errors).
    * The LuaScript object encapsulates a Lua environment (see official Lua docs) which strips all
    * global table entries after the script is loaded to the Lua state, and leaves only the run()
    * function.
    *
    * See also the full documentation at https://genivi.github.io/ramses-logic for more details on Lua and
    * its interaction with C++.
    */
    class LuaScript : public LogicNode
    {
    public:

        /**
        * Returns the filename provided when the script was created
        *
        * @return the filename of the script
        */
        RLOGIC_API std::string_view getFilename() const;

        /**
         * Overrides the lua print function with a custom function. Each time "print" is used
         * inside a lua script, the function will be called. Because the lua "print" function allows
         * an arbitrary amount of parameters, the function is called for each provided parameter.
         * @param luaPrintFunction to use for printing
         */
        RLOGIC_API void overrideLuaPrint(LuaPrintFunction luaPrintFunction);

        /**
        * Constructor of LuaScript. User is not supposed to call this - script are created by other factory classes
        *
        * @param impl implementation details of the script
        */
        explicit LuaScript(std::unique_ptr<internal::LuaScriptImpl> impl) noexcept;

        /**
        * Destructor of LuaScript
        */
        ~LuaScript() noexcept;

        /**
        * Copy Constructor of LuaScript is deleted because scripts are not supposed to be copied
        *
        * @param other script to copy from
        */
        LuaScript(const LuaScript& other) = delete;

        /**
        * Move Constructor of LuaScript is deleted because scripts are not supposed to be moved
        *
        * @param other script to move from
        */
        LuaScript(LuaScript&& other) = delete;

        /**
        * Assignment operator of LuaScript is deleted because scripts are not supposed to be copied
        *
        * @param other script to assign from
        */
        LuaScript& operator=(const LuaScript& other) = delete;

        /**
        * Move assignment operator of LuaScript is deleted because scripts are not supposed to be moved
        *
        * @param other script to move from
        */
        LuaScript& operator=(LuaScript&& other) = delete;

        /**
        * Implementation detail of LuaScript
        */
        std::unique_ptr<internal::LuaScriptImpl> m_script;
    };
}
