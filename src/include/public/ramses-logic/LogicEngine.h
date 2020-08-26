//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/LuaScript.h"

#include <vector>
#include <string>
#include <string_view>

namespace rlogic::internal
{
    class LogicEngineImpl;
}

namespace rlogic
{
    class LuaScript;
    class RamsesNodeBinding;

    /**
    * Entry object for the whole logic engine. It is used to create
    * objects for the logic graph and update the graph
    */
    class LogicEngine
    {
    public:
        /**
        * Constructor of LogicEngine.
        */
        RLOGIC_API LogicEngine() noexcept;

        /**
        * Destructor of LogicEngine
        */
        RLOGIC_API ~LogicEngine() noexcept;

        /**
         * Creates a new Lua script from an existing source file. Refer to the LuaScript class documentation
         * for requirements that Lua scripts must fulfill in order to be created.
         *
         * @param filename to load the script source from
         * @param scriptName name to assign to the script once it's created
         * @return a std::unique_ptr<LuaScript> instance or nullptr if
         * something went wrong during creation. In that case, use getErrors() to obtain compilation errors.
         */
        // TODO discuss if we mandate name, or accept empty too
        RLOGIC_API LuaScript* createLuaScriptFromFile(std::string_view filename, std::string_view scriptName = "");

        /**
         * Creates a new Lua script from a source string. Refer to the LuaScript class documentation
         * for requirements that Lua scripts must fulfill in order to be created.
         *
         * @param source to create the script from
         * @param scriptName name to assign to the script once it's created
         * @return a std::unique_ptr<LuaScript> instance or nullptr if
         * something went wrong during creation. In that case, use getErrors() to obtain compilation errors.
         */
        RLOGIC_API LuaScript* createLuaScriptFromSource(std::string_view source, std::string_view scriptName = "");

        /**
        * Destroys a Lua script. TODO update documentation once links have been implemented
        *
        * @param luaScript script instance to destroy
        * @return true if script destroyed, false otherwise. Call getErrors() for error details upon failure.
        */
        RLOGIC_API bool destroy(LuaScript& luaScript);

        /**
         * Creates a new RamsesNodeBinding. TODO update documentation once the function also needs a real ramses Node
         *
         * @param name of the new RamsesNodeBinding.
         * @return a std::unique_ptr<RamsesNodeBinding> instance or nullptr if
         * something went wrong during creation. In that case, use getErrors() to obtain errors.
         */
        RLOGIC_API RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);

        /**
         * Destroys a RamsesNodeBinding. TODO update documentation once links have been implemented
         *
         * @param ramsesNodeBinding instance to destroy
         * @return true if ramsesNodeBinding destroyed, false otherwise. Call getErrors() for error details upon failure.
         */
        RLOGIC_API bool destroy(RamsesNodeBinding& ramsesNodeBinding);

        /**
         * Updates all relevant LogicNodes which are known to the LogicEngine
         * @return true if the update was successful, false otherwise
         * In case of an error, get detailed error information with "LogicEngine::getErrors"
         */
        RLOGIC_API bool update();

        // TODO updade docs here after execute() has been implemented!
        /**
         * If an internal error occurs e.g. during loading or execution of a Lua script, a
         * detailed error description can be retrieved with this method. Errors get wiped at the beginning of the
         * next execute() call. New errors are always appended at the end of the vector.
         *
         * @return a std::vector<std::string> with detailed error information
         */
        RLOGIC_API const std::vector<std::string>& getErrors() const;

        /**
         * Deserializes the whole LogicEngine from the given file.
         * During deserialization the current state of the LogicEngine will be overwritten.
         * @param filename to read the LogicEngine from
         * @return true if deserialization was successful, false otherwise. To get more detailed
         * error information use LogicEngine::getErrors.
         */
        RLOGIC_API bool loadFromFile(std::string_view filename);

        /**
         * Writes the whole LogicEngine to a binary file with the given filename.
         * @param filename to write the LogicEngine to.
         * @return true if saving was successful, false otherwise. To get more detailed
         * error information use LogicEngine::getErrors
         */
        RLOGIC_API bool saveToFile(std::string_view filename) const;

        /**
        * Copy Constructor of LogicEngine is deleted because logic engines hold named resources and are not supposed to be copied
        *
        * @param other logic engine to copy from
        */
        LogicEngine(const LogicEngine& other) = delete;

        /**
        * Move Constructor of LogicEngine is deleted because because logic engine is not supposed to be moved
        *
        * @param other logic engine to move from
        */
        // TODO consider making LogicEngine move-able, nothing speaks against it
        LogicEngine(LogicEngine&& other) = delete;

        /**
        * Assignment operator of LogicEngine is deleted because logic engines hold named resources and are not supposed to be copied
        *
        * @param other logic engine to assign from
        */
        LogicEngine& operator=(const LogicEngine& other) = delete;

        /**
        * Move assignment operator of LogicEngine is deleted because logic engine is not supposed to be moved
        *
        * @param other logic engine to move from
        */
        // TODO consider making LogicEngine move-able, nothing speaks against it
        LogicEngine& operator=(LogicEngine&& other) = delete;

        /**
        * Implementation detail of LogicEngine
        */
        std::unique_ptr<internal::LogicEngineImpl> m_impl;
    };
}
