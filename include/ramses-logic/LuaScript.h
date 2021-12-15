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
#include <functional>

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
    * by the #rlogic::LogicEngine class.
    *
    * A LuaScript can be created from Lua source code which must fulfill following requirements:
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
    * - TYPE is one of [INT32|INT64|FLOAT|BOOL|STRING|VEC2F|VEC3F|VEC4F|VEC2I|VEC3I|VEC4I], or...
    * - TYPE can be also a Lua table with nested properties, obeying the same rules as above, or...
    * - TYPE can be an array declaration of the form ARRAY(n, T) where:
    *     - n is a positive integer
    *     - T obeys the same rules as TYPE, except T can not be an ARRAY itself
    *     - T can be a struct, i.e. arrays of structs are supported
    * - Each property must have a name (string) - other types like number, bool etc. are not supported as keys
    * - TYPE can also be defined in a module, see #rlogic::LuaModule for details
    * - the run() function only accesses the IN and OUT global symbols and the properties defined by it
    *
    * Violating any of these requirements will result in errors, which can be obtained by calling
    * #rlogic::LogicEngine::getErrors().
    * The LuaScript object encapsulates a Lua environment (see official Lua docs) which strips all
    * global table entries after the script is loaded to the Lua state, and leaves only the run()
    * function.
    * Note that none of the TYPE labels should be used in user code (outside of run() at least)
    * for other than interface definition purposes, see #rlogic::LuaModule for details.
    *
    * See also the full documentation at https://ramses-logic.readthedocs.io/en/latest/api.html for more details on Lua and
    * its interaction with C++.
    */
    class LuaScript : public LogicNode
    {
    public:

        /**
        * Constructor of LuaScript. User is not supposed to call this - script are created by other factory classes
        *
        * @param impl implementation details of the script
        */
        explicit LuaScript(std::unique_ptr<internal::LuaScriptImpl> impl) noexcept;

        /**
        * Destructor of LuaScript
        */
        ~LuaScript() noexcept override;

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
        internal::LuaScriptImpl& m_script;
    };
}
