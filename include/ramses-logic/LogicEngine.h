//  -------------------------------------------------------------------------
//  Copyright (C) 2020 BMW AG
//  -------------------------------------------------------------------------
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at https://mozilla.org/MPL/2.0/.
//  -------------------------------------------------------------------------

#pragma once

#include "ramses-logic/APIExport.h"
#include "ramses-logic/Collection.h"
#include "ramses-logic/ErrorData.h"
#include "ramses-logic/LuaConfig.h"
#include "ramses-logic/EPropertyType.h"
#include "ramses-logic/AnimationTypes.h"
#include "ramses-logic/ERotationType.h"

#include <vector>
#include <string_view>

namespace ramses
{
    class Scene;
    class Node;
    class Appearance;
    class Camera;
}

namespace rlogic::internal
{
    class LogicEngineImpl;
}

namespace rlogic
{
    class LogicNode;
    class LuaScript;
    class LuaModule;
    class Property;
    class RamsesNodeBinding;
    class RamsesAppearanceBinding;
    class RamsesCameraBinding;
    class DataArray;
    class AnimationNode;

    /**
    * Central object which creates and manages the lifecycle and execution
    * of scripts, bindings, and all other objects supported by the Ramses Logic library.
    * All objects created by this class' methods must be destroyed with #destroy!
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
         * Returns an iterable #rlogic::Collection of all #rlogic::LogicObject instances created by this #LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::LogicObject instances created by this #LogicEngine
         */
        [[nodiscard]] RLOGIC_API Collection<LogicObject> logicObjects() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::LuaScript instances created by this #LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::LuaScript instances created by this #LogicEngine
         */
        [[nodiscard]] RLOGIC_API Collection<LuaScript> scripts() const;

        /**
        * Returns an iterable #rlogic::Collection of all #rlogic::LuaModule instances created by this #LogicEngine.
        *
        * @return an iterable #rlogic::Collection with all #rlogic::LuaModule instances created by this #LogicEngine
        */
        [[nodiscard]] RLOGIC_API Collection<LuaModule> luaModules() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::RamsesNodeBinding instances created by this #LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::RamsesNodeBinding instances created by this #LogicEngine
         */
        [[nodiscard]] RLOGIC_API Collection<RamsesNodeBinding> ramsesNodeBindings() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::RamsesAppearanceBinding instances created by this LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::RamsesAppearanceBinding created by this #LogicEngine
         */
        [[nodiscard]] RLOGIC_API Collection<RamsesAppearanceBinding> ramsesAppearanceBindings() const;

        /**
         * Returns an iterable #rlogic::Collection of all #rlogic::RamsesCameraBinding instances created by this LogicEngine.
         *
         * @return an iterable #rlogic::Collection with all #rlogic::RamsesCameraBinding created by this #LogicEngine
         */
        [[nodiscard]] RLOGIC_API Collection<RamsesCameraBinding> ramsesCameraBindings() const;

        /**
        * Returns an iterable #rlogic::Collection of all #rlogic::DataArray instances created by this LogicEngine.
        *
        * @return an iterable #rlogic::Collection with all #rlogic::DataArray created by this #LogicEngine
        */
        [[nodiscard]] RLOGIC_API Collection<DataArray> dataArrays() const;

        /**
        * Returns an iterable #rlogic::Collection of all #rlogic::AnimationNode instances created by this LogicEngine.
        *
        * @return an iterable #rlogic::Collection with all #rlogic::AnimationNode created by this #LogicEngine
        */
        [[nodiscard]] RLOGIC_API Collection<AnimationNode> animationNodes() const;

        /**
         * Returns a pointer to the first occurrence of an object with a given \p name regardless of its type.
         * To convert the object to a concrete type (e.g. LuaScript) use #rlogic::LogicObject::as<Type>() e.g.:
         *   auto myLuaScript = logicEngine.findLogicObject("myLuaScript")->as<LuaScript>());
         * Be aware that this function behaves as \c dynamic_cast and will return nullptr (without error) if
         * given type doesn't match the objects type. This can later lead to crash if ignored.
         *
         * @param name the name of the logic object to search for
         * @return a pointer to the logic object, or nullptr if none was found
         */
        [[nodiscard]] RLOGIC_API const LogicObject* findLogicObject(std::string_view name) const;
        /// @copydoc findLogicObject(std::string_view) const
        [[nodiscard]] RLOGIC_API LogicObject* findLogicObject(std::string_view name);

        /**
         * Returns a pointer to the first occurrence of a script with a given \p name if such exists, and nullptr otherwise.
         *
         * @param name the name of the script to search for
         * @return a pointer to the script, or nullptr if none was found
         */
        [[nodiscard]] RLOGIC_API const LuaScript* findScript(std::string_view name) const;
        /// @copydoc findScript(std::string_view) const
        [[nodiscard]] RLOGIC_API LuaScript* findScript(std::string_view name);

        /**
        * Returns a pointer to the first occurrence of a #rlogic::LuaModule with a given \p name if such exists, and nullptr otherwise.
        *
        * @param name the name of the module to search for
        * @return a pointer to the script, or nullptr if none was found
        */
        [[nodiscard]] RLOGIC_API const LuaModule* findLuaModule(std::string_view name) const;
        /// @copydoc findScript(std::string_view) const
        [[nodiscard]] RLOGIC_API LuaModule* findLuaModule(std::string_view name);

        /**
         * Returns a pointer to the first occurrence of a node binding with a given \p name if such exists, and nullptr otherwise.
         *
         * @param name the name of the node binding to search for
         * @return a pointer to the node binding, or nullptr if none was found
         */
        [[nodiscard]] RLOGIC_API const RamsesNodeBinding* findNodeBinding(std::string_view name) const;
        /// @copydoc findNodeBinding(std::string_view) const
        [[nodiscard]] RLOGIC_API RamsesNodeBinding* findNodeBinding(std::string_view name);

        /**
         * Returns a pointer to the first occurrence of an appearance binding with a given \p name if such exists, and nullptr otherwise.
         *
         * @param name the name of the appearance binding to search for
         * @return a pointer to the appearance binding, or nullptr if none was found
         */
        [[nodiscard]] RLOGIC_API const RamsesAppearanceBinding* findAppearanceBinding(std::string_view name) const;
        /// @copydoc findAppearanceBinding(std::string_view) const
        [[nodiscard]] RLOGIC_API RamsesAppearanceBinding* findAppearanceBinding(std::string_view name);

        /**
         * Returns a pointer to the first occurrence of a camera binding with a given \p name if such exists, and nullptr otherwise.
         *
         * @param name the name of the camera binding to search for
         * @return a pointer to the camera binding, or nullptr if none was found
         */
        [[nodiscard]] RLOGIC_API const RamsesCameraBinding* findCameraBinding(std::string_view name) const;
        /// @copydoc findCameraBinding(std::string_view) const
        [[nodiscard]] RLOGIC_API RamsesCameraBinding* findCameraBinding(std::string_view name);

        /**
        * Returns a pointer to the first occurrence of a #rlogic::DataArray with a given \p name if such exists, and nullptr otherwise.
        *
        * @param name the name of the #rlogic::DataArray to search for
        * @return a pointer to the #rlogic::DataArray, or nullptr if none was found
        */
        [[nodiscard]] RLOGIC_API const DataArray* findDataArray(std::string_view name) const;
        /// @copydoc findDataArray(std::string_view) const
        [[nodiscard]] RLOGIC_API DataArray* findDataArray(std::string_view name);

        /**
        * Returns a pointer to the first occurrence of an #rlogic::AnimationNode with a given \p name if such exists, and nullptr otherwise.
        *
        * @param name the name of the #rlogic::AnimationNode to search for
        * @return a pointer to the #rlogic::AnimationNode, or nullptr if none was found
        */
        [[nodiscard]] RLOGIC_API const AnimationNode* findAnimationNode(std::string_view name) const;
        /// @copydoc findAnimationNode(std::string_view) const
        [[nodiscard]] RLOGIC_API AnimationNode* findAnimationNode(std::string_view name);

        /**
        * Creates a new Lua script from a source string. Refer to the #rlogic::LuaScript class
        * for requirements which Lua scripts must fulfill in order to be added to the #LogicEngine.
        * You can optionally provide Lua module dependencies via the \p config, they will be accessible
        * under their configured alias name for use by the script. The provided module dependencies
        * must exactly match the declared dependencies in source code (see #extractLuaDependencies).
        *
        * Attention! This method clears all previous errors! See also docs of #getErrors()
        *
        * @param source the Lua source code
        * @param config configuration options, e.g. for module dependencies
        * @param scriptName name to assign to the script once it's created
        * @return a pointer to the created object or nullptr if
        * something went wrong during creation. In that case, use #getErrors() to obtain errors.
        * The script can be destroyed by calling the #destroy method
        */
        RLOGIC_API LuaScript* createLuaScript(
            std::string_view source,
            const LuaConfig& config = {},
            std::string_view scriptName = "");

        /**
         * Creates a new #rlogic::LuaModule from Lua source code.
         * LuaModules can be used to share code and data constants across scripts or
         * other modules. See also #createLuaScript and #rlogic::LuaConfig for details.
         * You can optionally provide Lua module dependencies via the \p config, they will be accessible
         * under their configured alias name for use by the module. The provided module dependencies
         * must exactly match the declared dependencies in source code (see #extractLuaDependencies).
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param source module source code
         * @param config configuration options, e.g. for module dependencies
         * @param moduleName name to assign to the module once it's created
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         * The script can be destroyed by calling the #destroy method
         */
        RLOGIC_API LuaModule* createLuaModule(
            std::string_view source,
            const LuaConfig& config = {},
            std::string_view moduleName = "");

        /**
         * Extracts dependencies from a Lua script or module source code so that the corresponding
         * modules can be provided when creating #rlogic::LuaScript or #rlogic::LuaModule.
         *
         * Any #rlogic::LuaScript or #rlogic::LuaModule which has a module dependency,
         * i.e. it requires another #rlogic::LuaModule for it to work, must explicitly declare these
         * dependencies directly in their source code by calling function 'modules' in global space
         * and pass list of module names it depends on, for example:
         *   \code{.lua}
         *       modules("foo", "bar")
         *       function interface()
         *         OUT.x = foo.myType()
         *       end
         *       function run()
         *         OUT.x = bar.doSth()
         *       end
         *   \endcode
         * The 'modules' function does not affect any other part of the source code in any way,
         * it is used only for the purpose of explicit declaration and extraction of its dependencies.
         *
         * Please note that script runtime errors are ignored during extraction. In case a runtime
         * error prevents the 'modules' function to be called, this method will still succeed
         * but will not extract any modules, i.e. will not call \c callbackFunc. It is therefore
         * highly recommended to put the modules declaration always at the beginning of every script
         * before any other code so it will get executed even if there is runtime error later in the code.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param source source code of module or script to parse for dependencies
         * @param callbackFunc function callback will be called for each dependency found
         * @return \c true if extraction succeeded (also if no dependencies found) or \c false if
         * something went wrong. In that case, use #getErrors() to obtain errors.
         */
        RLOGIC_API bool extractLuaDependencies(
            std::string_view source,
            const std::function<void(const std::string&)>& callbackFunc);

        /**
         * Creates a new #rlogic::RamsesNodeBinding which can be used to set the properties of a Ramses Node object.
         * The initial values of the binding's properties are loaded from the \p ramsesNode. Rotation values are
         * taken over from the \p ramsesNode only if the conventions are compatible (see \ref rlogic::ERotationType).
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param ramsesNode the ramses::Node object to control with the binding.
         * @param rotationType the type of rotation to use (will affect the 'rotation' property semantics and type).
         * @param name a name for the new #rlogic::RamsesNodeBinding.
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         * The binding can be destroyed by calling the #destroy method
         */
        RLOGIC_API RamsesNodeBinding* createRamsesNodeBinding(ramses::Node& ramsesNode, ERotationType rotationType = ERotationType::Euler_XYZ, std::string_view name = "");

        /**
         * Creates a new #rlogic::RamsesAppearanceBinding which can be used to set the properties of a Ramses Appearance object.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param ramsesAppearance the ramses::Appearance object to control with the binding.
         * @param name a name for the the new #rlogic::RamsesAppearanceBinding.
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         * The binding can be destroyed by calling the #destroy method
         */
        RLOGIC_API RamsesAppearanceBinding* createRamsesAppearanceBinding(ramses::Appearance& ramsesAppearance, std::string_view name = "");

        /**
         * Creates a new #rlogic::RamsesCameraBinding which can be used to set the properties of a Ramses Camera object.
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param ramsesCamera the ramses::Camera object to control with the binding.
         * @param name a name for the the new #rlogic::RamsesCameraBinding.
         * @return a pointer to the created object or nullptr if
         * something went wrong during creation. In that case, use #getErrors() to obtain errors.
         * The binding can be destroyed by calling the #destroy method
         */
        RLOGIC_API RamsesCameraBinding* createRamsesCameraBinding(ramses::Camera& ramsesCamera, std::string_view name ="");

        /**
        * Creates a new #rlogic::DataArray to store data which can be used with animations.
        * Provided data must not be empty otherwise creation will fail.
        * See #rlogic::CanPropertyTypeBeStoredInDataArray and #rlogic::PropertyTypeToEnum
        * to determine supported types that can be used to create a #rlogic::DataArray.
        *
        * Attention! This method clears all previous errors! See also docs of #getErrors()
        *
        * @param data source data to move into #rlogic::DataArray, must not be empty.
        * @param name a name for the the new #rlogic::DataArray.
        * @return a pointer to the created object or nullptr if
        * something went wrong during creation. In that case, use #getErrors() to obtain errors.
        */
        template <typename T>
        DataArray* createDataArray(const std::vector<T>& data, std::string_view name ="");

        /**
        * Creates a new #rlogic::AnimationNode for animating properties.
        * Refer to #rlogic::AnimationNode for more information about its use.
        * There must be at least one channel provided, please see #rlogic::AnimationChannel
        * requirements for all the data.
        *
        * Attention! This method clears all previous errors! See also docs of #getErrors()
        *
        * @param channels list of animation channels to be animated with this animation node.
        * @param name a name for the the new #rlogic::AnimationNode.
        * @return a pointer to the created object or nullptr if
        * something went wrong during creation. In that case, use #getErrors() to obtain errors.
        */
        RLOGIC_API AnimationNode* createAnimationNode(const AnimationChannels& channels, std::string_view name = "");

        /**
         * Updates all #rlogic::LogicNode's which were created by this #LogicEngine instance.
         * The order in which #rlogic::LogicNode's are executed is determined by the links created
         * between them (see #link and #unlink). #rlogic::LogicNode's which don't have any links
         * between then are executed in arbitrary order, but the order is always the same between two
         * invocations of #update without any calls to #link or #unlink between them.
         * As an optimization #rlogic::LogicNode's are only updated, if at least one input of a #rlogic::LogicNode
         * has changed since the last call to #update. If the links between logic nodes create a loop,
         * this method will fail with an error and will not execute any of the logic nodes.
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
         * before node B. A single output property (\p sourceProperty) can be linked to any number of input
         * properties (\p targetProperty), but any input property can have at most one link to an output property
         * (links are directional and support a 1-to-N relationships).
         *
         * The #link() method will fail when:
         * - \p sourceProperty and \p targetProperty belong to the same #rlogic::LogicNode
         * - \p sourceProperty is not an output (see #rlogic::LogicNode::getOutputs())
         * - \p targetProperty is not an input (see #rlogic::LogicNode::getInputs())
         * - either \p sourceProperty or \p targetProperty is not a primitive property (you have to link sub-properties
         *   of structs and arrays individually)
         *
         * Creating link loops will cause the next call to #update() to fail with an error. Loops
         * are directional, it is OK to have A->B, A->C and B->C, but is not OK to have
         * A->B->C->A.
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
         * Checks if an input or output of a given LogicNode is linked to another LogicNode
         * @param logicNode the node to check for linkage.
         * @return true if the given LogicNode is linked to any other LogicNode, false otherwise.
         */
        [[nodiscard]] RLOGIC_API bool isLinked(const LogicNode& logicNode) const;

        /**
         * Returns the list of all errors which occurred during the last API call to a #LogicEngine method
         * or any other method of its subclasses (scripts, bindings etc). Note that errors get wiped by all
         * mutable methods of the #LogicEngine.
         *
         * This method can be used in two different ways:
         * - To debug the correct usage of the Logic Engine API (e.g. by wrapping all API calls with a check
         *   for their return value and using this method to find out the cause of the error)
         * - To check for runtime errors of scripts which come from a dynamic source, e.g. by calling the
         *   method after an unsuccessful call to #update() with a faulty script
         *
         * @return a list of errors
         */
        [[nodiscard]] RLOGIC_API const std::vector<ErrorData>& getErrors() const;

        /**
        * Destroys an instance of an object created with #LogicEngine.
        * All objects created using #LogicEngine derive from a base class #rlogic::LogicObject
        * and can be destroyed using this method.
        *
        * In case of a #rlogic::LogicNode and its derived classes, if any links are connected to this #rlogic::LogicNode,
        * they will be destroyed too. Note that after this call, the execution order of #rlogic::LogicNode may change! See the
        * docs of #link and #unlink for more information.
        *
        * In case of a #rlogic::DataArray, destroy will fail if it is used in any #rlogic::AnimationNode's #rlogic::AnimationChannel.
        *
        * In case of a #rlogic::LuaModule, destroy will fail if it is used in any #rlogic::LuaScript.
        *
        * Attention! This method clears all previous errors! See also docs of #getErrors()
        *
        * @param object the object instance to destroy
        * @return true if object destroyed, false otherwise. Call #getErrors() for error details upon failure.
        */
        RLOGIC_API bool destroy(LogicObject& object);

        /**
         * Writes the whole #LogicEngine and all of its objects to a binary file with the given filename. The RAMSES scene
         * potentially referenced by #rlogic::RamsesBinding objects is not saved - that is left to the application.
         * #LogicEngine saves the references to those object, and restores them after loading.
         * Thus, deleting Ramses objects which are being referenced from within the #LogicEngine
         * will result in errors if the Logic Engine is loaded from the file again. Note that it is not sufficient
         * to have objects with the same name, they have to be the exact same objects as during saving!
         * For more in-depth information regarding saving and loading, refer to the online documentation at
         * https://ramses-logic.readthedocs.io/en/latest/api.html#saving-loading-from-file
         *
         * Note: The method reports error and aborts if the #rlogic::RamsesBinding objects reference more than one
         * Ramses scene (this is acceptable during runtime, but not for saving to file).
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param filename path to file to save the data (relative or absolute). The file will be created or overwritten if it exists!
         * @return true if saving was successful, false otherwise. To get more detailed
         * error information use #getErrors()
         */
        RLOGIC_API bool saveToFile(std::string_view filename);

        /**
         * Loads the whole LogicEngine data from the given file. See also #saveToFile().
         * After loading, the previous state of the #LogicEngine will be overwritten with the
         * contents loaded from the file, i.e. all previously created objects (scripts, bindings, etc.)
         * will be deleted and pointers to them will be invalid. The (optionally) provided ramsesScene
         * will be used to resolve potential #rlogic::RamsesBinding objects which point to Ramses objects.
         * You can provide a nullptr if you know for sure that the
         * #LogicEngine loaded from the file has no #rlogic::RamsesBinding objects which point to a Ramses scene object.
         * Otherwise, the call to #loadFromFile will fail with an error. In case of errors, the #LogicEngine
         * may be left in an inconsistent state.
         * For more in-depth information regarding saving and loading, refer to the online documentation at
         * https://ramses-logic.readthedocs.io/en/latest/api.html#saving-loading-from-file
         *
         * Attention! This method clears all previous errors! See also docs of #getErrors()
         *
         * @param filename path to file from which to load content (relative or absolute)
         * @param ramsesScene pointer to the Ramses Scene which holds the objects referenced in the Ramses Logic file
         * @param enableMemoryVerification flag to enable memory verifier (a flatbuffers feature which checks bounds and ranges).
         *        Disable this only if the file comes from a trusted source and performance is paramount.
         * @return true if deserialization was successful, false otherwise. To get more detailed
         * error information use #getErrors()
         */
        RLOGIC_API bool loadFromFile(std::string_view filename, ramses::Scene* ramsesScene = nullptr, bool enableMemoryVerification = true);

        /**
        * Loads the whole LogicEngine data from the given memory buffer. This method is equivalent to
        * #loadFromFile but allows to have the file-opening
        * logic done by the user and only pass the data as a buffer. The logic engine only reads the
        * data, does not take ownership of it and does not modify it. The memory can be freed or
        * modified after the call returns, the #LogicEngine keeps no references to it.
        *
        * @param rawBuffer pointer to the raw data in memory
        * @param bufferSize size of the data (bytes)
        * @param ramsesScene pointer to the Ramses Scene which holds the objects referenced in the Ramses Logic file
        * @param enableMemoryVerification flag to enable memory verifier (a flatbuffers feature which checks bounds and ranges).
        *        Disable this only if the file comes from a trusted source and performance is paramount.
        * @return true if deserialization was successful, false otherwise. To get more detailed
        * error information use #getErrors()
        */
        RLOGIC_API bool loadFromBuffer(const void* rawBuffer, size_t bufferSize, ramses::Scene* ramsesScene = nullptr, bool enableMemoryVerification = true);

        /**
        * Copy Constructor of LogicEngine is deleted because logic engines hold named resources and are not supposed to be copied
        *
        * @param other logic engine to copy from
        */
        LogicEngine(const LogicEngine& other) = delete;

        /**
        * Move Constructor of LogicEngine
        *
        * @param other logic engine to move from
        */
        RLOGIC_API LogicEngine(LogicEngine&& other) noexcept;

        /**
        * Assignment operator of LogicEngine is deleted because logic engines hold named resources and are not supposed to be copied
        *
        * @param other logic engine to assign from
        */
        LogicEngine& operator=(const LogicEngine& other) = delete;

        /**
        * Move assignment operator of LogicEngine
        *
        * @param other logic engine to move from
        */
        RLOGIC_API LogicEngine& operator=(LogicEngine&& other) noexcept;

        /**
        * Implementation detail of LogicEngine
        */
        std::unique_ptr<internal::LogicEngineImpl> m_impl;

    private:
        /**
        * Internal implementation of #createDataArray
        *
        * @param data source data
        * @param name name
        * @return a pointer to the created object or nullptr on error
        */
        template <typename T>
        RLOGIC_API DataArray* createDataArrayInternal(const std::vector<T>& data, std::string_view name);
    };

    template <typename T>
    DataArray* LogicEngine::createDataArray(const std::vector<T>& data, std::string_view name)
    {
        static_assert(IsPrimitiveProperty<T>::value && CanPropertyTypeBeStoredInDataArray(PropertyTypeToEnum<T>::TYPE),
            "Unsupported data type, see createDataArray API doc to see supported types.");
        return createDataArrayInternal<T>(data, name);
    }
}
