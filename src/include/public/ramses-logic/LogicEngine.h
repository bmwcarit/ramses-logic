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
#include "ramses-logic/Collection.h"

#include <vector>
#include <string>
#include <string_view>

namespace ramses
{
    class Scene;
}

namespace rlogic::internal
{
    class LogicEngineImpl;
}

namespace rlogic
{
    class LuaScript;
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;

    /**
    * Central object which creates and manages the lifecycle and execution
    * of scripts, bindings, and all other objects supported by the Ramses Logic library.
    *
    * - Use the create[Type] methods to create various objects, use #destroy() to delete them.
    * - Use #link and #unlink to connect data properties between these objects
    * - use #update() to trigger the execution of all objects
    */
    class LogicEngine
    {
    public:
        /**
        * Constructor of #LogicEngine.
        */
        RLOGIC_API LogicEngine() noexcept;

        /**
        * Destructor of #LogicEngine
        */
        RLOGIC_API ~LogicEngine() noexcept;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::LuaScript instances created by this #LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::LuaScript instances created by this #LogicEngine
         */
        RLOGIC_API Collection<LuaScript> scripts() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::RamsesNodeBinding instances created by this #LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::RamsesNodeBinding instances created by this #LogicEngine
         */
        RLOGIC_API Collection<RamsesNodeBinding> ramsesNodeBindings() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::RamsesAppearanceBinding instances created by this LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::RamsesAppearanceBinding created by this #LogicEngine
         */
        RLOGIC_API Collection<RamsesAppearanceBinding> ramsesAppearanceBindings() const;

        /**
         * Creates a new #rlogic::LuaScript from an existing Lua source file. Refer to the #rlogic::LuaScript class documentation
         * for requirements that Lua scripts must fulfill in order to be added to the #LogicEngine.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param filename path to file from which to load the script source code
         * @param scriptName name to assign to the script once it's created
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         */
        // TODO Violin discuss if we mandate name, or accept empty too
        RLOGIC_API LuaScript* createLuaScriptFromFile(std::string_view filename, std::string_view scriptName = "");

        /**
         * Creates a new Lua script from a source string. Refer to the #rlogic::LuaScript class
         * for requirements which Lua scripts must fulfill in order to be added to the #LogicEngine.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param source the Lua source code
         * @param scriptName name to assign to the script once it's created
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         */
        RLOGIC_API LuaScript* createLuaScriptFromSource(std::string_view source, std::string_view scriptName = "");

        /**
        * Destroys a #rlogic::LogicNode instance. If any links are connected to this #rlogic::LogicNode, they will
        * be destroyed too. Note that after this call, the execution order of #rlogic::LogicNode may change! See the
        * docs of #link and #unlink for more information.
        *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
        * @param logicNode the logic node instance to destroy
        * @return true if logicNode destroyed, false otherwise. Call #getErrors() for error details upon failure.
        */
        RLOGIC_API bool destroy(LogicNode& logicNode);

        /**
         * Creates a new #rlogic::RamsesNodeBinding which can be used to set the properties of a Ramses Node object.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param name a name for the new #rlogic::RamsesNodeBinding.
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         */
        RLOGIC_API RamsesNodeBinding* createRamsesNodeBinding(std::string_view name);

        /**
         * Creates a new #rlogic::RamsesAppearanceBinding which can be used to set the properties of a Ramses Appearance object.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param name a name for the the new #rlogic::RamsesAppearanceBinding.
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         */
        RLOGIC_API RamsesAppearanceBinding* createRamsesAppearanceBinding(std::string_view name);

        /**
         * Updates all #rlogic::LogicNode's which were created by this #LogicEngine instance.
         * The order in which #rlogic::LogicNode's are executed is determined by the links created
         * between them (see #link and #unlink). #rlogic::LogicNode's which don't have any links
         * between then are executed in arbitrary order, but the order is always the same between two
         * invocations of #update without any calls to #link or #unlink between them.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @return true if the update was successful, false otherwise
         * In case of an error, use #getErrors() to obtain errors.
         */
        RLOGIC_API bool update();

        /**
         * Links a property of a #rlogic::LogicNode to another #rlogic::Property of another #rlogic::LogicNode.
         * After linking, calls to #update will propagate the value of \p sourceProperty to
         * the \p targetProperty. Creating links influences the order in which scripts
         * are executed - if node A provides data to node B, then node A will be executed
         * before node B.
         *
         * The #link() method will result in undefined behavior when creating links:
         * - which are created between properties of the same #rlogic::LogicNode
         * - for which the \p sourceProperty is not an output (see #rlogic::LogicNode::getOutputs())
         * - which create a 'link cycle', i.e. a set of #rlogic::LogicNode's which are connected in a cyclic fashion
         *
         * After calling #link, the value of the \p targetProperty will not change until the next call to #update. Creating
         * and destroying links generally has no effect until #update is called.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param sourceProperty the output property which will provide data for \p targetProperty
         * @param targetProperty the target property which will receive its value from \p sourceProperty
         * @return true if linking was successful, false otherwise. To get more detailed
         * error information use #getErrors()
         */
        // TODO Violin/Sven fix above docs once we have implemented cycle detection
        RLOGIC_API bool link(const Property& sourceProperty, const Property& targetProperty);

        /**
         * Unlinks two properties which were linked with #link. After a link is destroyed,
         * calls to #update will no longer propagate the output value from the \p sourceProperty to
         * the input value of the \p targetProperty. The value of the \p targetProperty will remain as it was after the last call to #update -
         * it will **not** be restored to a default value or to any value which was set manually with calls to #rlogic::Property::set().
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param sourceProperty the output property which is currently linked to \p targetProperty
         * @param targetProperty the property which will no longer receive the value from \p sourceProperty
         * @return true if unlinking was successful, false otherwise. To get more detailed
         * error information use #getErrors().
         */
        RLOGIC_API bool unlink(const Property& sourceProperty, const Property& targetProperty);

        /**
         * If an internal error occurs e.g. during loading or execution of a #rlogic::LuaScript, a
         * detailed error description can be retrieved with this method. The list of errors is reset before
         * each #LogicEngine method is called, and contains potential errors created by that method after it returns.
         *
         * @return a list of (potentially multi-line) strings with detailed error information. For Lua errors, an extra stack
         * trace is contained in the error string. Error strings contain new line characters and spaces, and in the case of Lua errors
         * it also contains the error stack trace captured by Lua.
         */
        RLOGIC_API const std::vector<std::string>& getErrors() const;

        /**
         * Writes the whole #LogicEngine and all of its objects to a binary file with the given filename. The RAMSES scene
         * potentially referenced by #rlogic::RamsesBinding objects is not saved - that is left to the application.
         * #LogicEngine saves the references to those object, and restores them after loading.
         * Thus, deleting Ramses objects which are being referenced from within the #LogicEngine
         * will result in errors if the Logic Engine is loaded from the file again. Note that it is not sufficient
         * to have objects with the same name, they have to be the exact same objects as during saving!
         * For more in-depth information regarding saving and loading, refer to the online documentation at
         * https://genivi.github.io/ramses-logic/api.html#serialization-deserialization
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param filename path to file to save the data (relative or absolute). The file will be created or overwritten if it exists!
         * @return true if saving was successful, false otherwise. To get more detailed
         * error information use #getErrors()
         */
        RLOGIC_API bool saveToFile(std::string_view filename);

        /**
         * Loads the whole LogicEngine from the given file. See also #saveToFile().
         * After loading, the previous state of the #LogicEngine will be overwritten with the
         * contents loaded from the file, i.e. all previously created objects (scripts, bindings, etc.)
         * will be deleted and pointers to them will be invalid. The (optionally) provided ramsesScene
         * will be used to resolve potential #rlogic::RamsesBinding objects which point to Ramses objects.
         * You can provide a nullptr if you know for sure that the
         * #LogicEngine loaded from the file has no #rlogic::RamsesBinding objects which point to a Ramses scene object.
         * Otherwise, the call to #loadFromFile will fail with an error. In case of errors, the #LogicEngine
         * may be left in an inconsistent state.
         * For more in-depth information regarding saving and loading, refer to the online documentation at
         * https://genivi.github.io/ramses-logic/api.html#serialization-deserialization
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param filename path to file from which to load content (relative or absolute)
         * @param ramsesScene pointer to the Ramses Scene which holds the objects referenced in the Ramses Logic file
         * @return true if deserialization was successful, false otherwise. To get more detailed
         * error information use #getErrors()
         */
        RLOGIC_API bool loadFromFile(std::string_view filename, ramses::Scene* ramsesScene = nullptr);

        /**
         * Checks if an input or output of a given LogicNode is linked to another LogicNode
         * @param logicNode the node to check for linkage.
         * @return true if the given LogicNode is linked to any other LogicNode, false otherwise.
         */
        RLOGIC_API bool isLinked(const LogicNode& logicNode) const;

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
        // TODO Violin consider making LogicEngine move-able, nothing speaks against it
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
        // TODO Violin consider making LogicEngine move-able, nothing speaks against it
        LogicEngine& operator=(LogicEngine&& other) = delete;

        /**
        * Implementation detail of LogicEngine
        */
        std::unique_ptr<internal::LogicEngineImpl> m_impl;
    };
}
