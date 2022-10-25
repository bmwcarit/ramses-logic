# master

# Unreleased

# v1.3.0

**ADDED**

* Added new Feature Level 04 with following features:
    * SkinBinding to calculate joint matrices for vertex skinning
    * DataArray can contain arrays of floats as elements
* Added command line argument '--clearColor' to LogicViewer to set background clear color

**FIXED**

* Anchor points with output properties not linked do not fail validation anymore,
  this is to allow use cases where applications read output directly
* Bindings which are used in Anchor points and have no input properties linked do not fail
  validation anymore
* Upgrade ramses from 27.0.125 to 27.0.126 (patchfix)

# v1.2.2

**ADDED**

* Added LogicEngine::loadFromFileDescriptor() to load logic files with an open file descriptor

**FIXED**

* Scripts (and other nodes) with no input or no output properties do not fail validation anymore,
  only nodes with actual properties on each input/output side are checked if they are linked

# v1.2.1

**FIXED**

* Upgrade ramses from 27.0.122 to 27.0.125 (patchfix)

# v1.2.0

**Features**

* Added new Feature Level 03 with following features:
    * Added RamsesRenderGroupBinding and RamsesRenderGroupBindingElements to control render order of elements in a ramses::RenderGroup

**General Changes**

* Upgrade ramses from 27.0.121 to 27.0.122 (patchfix)
* ramses-logic-viewer: added GUI option to change clear the display color (both offscreen or not)

# v1.1.1

**API Changes**

* Added Property::getOwningLogicNode to query which logic node the propery belongs to
* Add new function to get the serialized size of ramses-logic objects
* Added Property::getIncomingLink, Property::getOutogingLinksCount and Property::getOutogingLink to query which properties are linked to it
* Added LogicEngine::getPropertyLinks to collect and retrieve all existing links between properties
* Added static LogicEngine::GetFeatureLevelFromBuffer which can parse a rlogic data buffer and detect its feature level without having to instantiate LogicEngine first.

**General Changes**

* ramses-logic-viewer: added `--exec-lua` cli to execute lua code (after the lua configuration file was executed)
* ramses-logic-viewer: added `--headless` cli to run the viewer without renderer and display (e.g. in CI environments)

# v1.1.0

**General Changes**

* Upgrade ramses from 27.0.119 to 27.0.121 (patchfix)
* LuaInterface will not fail to be created if it does not have a unique name anymore,
  however validation (LogicEngine::validate) will produce warning if two or more interfaces share same name
* ramses-logic-viewer: improved command line interface
    * new `--help` message and error handling
    * renamed `--multi-sample` to `--msaa`
    * added `--write-config` option to save default lua configuration
    * added `--log-level-console` option. Enable error log level by default

**API Changes**

* Added new constructor for LogicEngine where a feature level can be specified, see LogicEngine::LogicEngine(EFeatureLevel) and EFeatureLevel for details
* Added static LogicEngine::GetFeatureLevelFromFile which can parse a rlogic file and detect its feature level without having to instantiate LogicEngine first.

**Bugfixes**

* ramses-logic-viewer: use 1-based indices for accessing children of Array properties in lua configuration (previous versions used 0-based indices)
* ramses-logic-viewer: fixed content of default lua configuration file

**Features**

* Added new Feature Level 02 with following features:
    * Added rlogic::AnchorPoint to help tracking a 3D node in 2D coordinates
    * Added rlogic::RamsesRenderPassBinding allowing control of ramses::RenderPass states from logic nodes
    * Added 'enabled' input property to RamsesNodeBinding to be able to control also 3rd state of Ramses Node visibility mode to turn it Off (in addition to Visible/Invisible).
    * Lua scripts and modules are serialized as pre-compiled binaries instead of source code.
    * Added LogicEngine::createRamsesCameraBindingWithFrustumPlanes which allows also perspective camera binding to have a full set of frustum planes properties

# v1.0.3

**General Changes**

* ramses-logic-viewer: added support for lua interfaces

**API Changes**

* Added rlogic::Property::hasIncomingLink()
* Added rlogic::Property::hasOutgoingLink()

**Bugfixes**

* Remove subtle behavior differences in Lua script between Debug/Release builds
* Fixed API documentation for rlogic::Property::isLinked()
* ramses-logic-viewer: fixed property access in lua configuration file

# v1.0.2

**Bugfixes**

* Fixes more crashes with large interface objects
* Produces CMake error if compiled with Clang++ < 10.0.0 (older versions don't support C++17 fully)

# v1.0.1

**Bugfixes**

* Fixes exotic crash when creating complex nested interfaces
* LuaInterface objects can be retrieved via LogicEngine::findByName, LogicEngine::findLogicObjectById and can be cast using LogicObject::as<T>()
* Fixed assert when logging a script error which contains curly braces conflicting with internal Fmt formatter

# v1.0.0

**General changes**

* Version bump to 1.0!
    * From now on, any API, ABI or content-breaking change will result in major version bumps
    * New APIs or non-breaking features will result in minor version bumps
    * Binary file compatibility is explicitly documented in README.md, as before
* Not backwards compatible
    * Binary files need re-export
* Updated Ramses to 27.0.119
    * Fixes bugs
    * Adds custom logger

**Bugfixes**

* Objects created after loading from binary data/file will also receive unique id's
* Can't overwrite the IN and OUT globals in the interface() function any more
* RamsesLogic::saveToFile produces the same file content when called twice
* Mismatching rotation conventions don't cause warnings any more if the rotation values are (0, 0, 0)
* Modules cost less memory in binary files
* Bindings are not dirty unless their inputs changed. Fixed wrong warning associated to that.
* Reading and writing global variables in modules now reports a verbose error

**API Changes**

* !!BREAKING Lua syntax!! Reworked Lua type system
    * See [LuaScript docs](https://ramses-logic.readthedocs.io/en/latest/classes/LuaScript.html) for details
    * Replaces "TYPENAME" (all uppercase) with "Type:TypeName()" (in camel case), e.g., "INT32" becomes "Type:Int32()"
    * We provide a Python migration script you can use to update existing content (tools/migration/migrate_to_v1_0.py)
    * Int is only available as Int32 or Int64, have to select bit-ness explicitly now (previous INT corresponds to Int32)
    * IN and OUT global varaibles are now passed as params to run() and interface() functions
        * IN and OUT are not reserved keywords anymore and do not carry special semantics
        * Params declared in run() and interface() functions do not have to be called IN and OUT, but they can be. Most importantly
          the order needs to be maintained, i.e., the 1st param represents inputs and the 2nd param represents outputs
        * root properties (returned by LogicNode::getInputs/getOutputs) have empty names now, instead of "IN"/"OUT"
* Possible to create multidimensional arrays, e.g. "IN.matrix3d = Type:Array(3, Type:Array(3, Type:Float()))" will create a 3x3 float matrix
* ErrorData holds an enum to distinguish the type of errors
    * Use this to handle specific errors, e.g. file operations or Lua runtime errors, differently
* Majorly simplified AnimationNode control interface
    * Removed 'timeDelta', 'play', 'loop', 'rewindOnStop', 'timeRange' inputs and 'progress' output
    * Instead the animation is now controlled directly by setting its progress via new input 'progress'
    * Removed AnimationNode::getDuration and added new 'duration' output property instead (due to this channel outputs are shifted by 1 if accessed by index!)
    * Removed 'timeDelta' output from TimerNode as the time delta concept is not used on rlogic level, but can be done fully on application side if desired
      (due to this ticker_us output is shifted by 1 if accessed by index!)
    * Added a new example which demonstrates how to implement an 'animateTo' functionality using Lua script (8b_dynamic_animation)
* Added LogicEngine::createLuaInterface
    * Creates a LuaInterface object based on Lua script that implements only  "interface" function, and defines only IN variables.
    * More information can be found in the [API docs](https://ramses-logic.readthedocs.io/en/latest/classes/LuaInterface.html)
    * Interfaces have to have a non-empty and unique name
    * LogicNode::setName() can fail now if the node is a LuaInterface - can't change the name of interface objects
    * An interface with unlinked outputs issues a warning on validation
* If validation is enabled and warnings were found, saveToFile will not write files
    * Override with FileSaveConfig::setValidationEnabled(false) (see section below about LogicEngine::validate())

**Features**

* Added LogicEngine::validate() which collects content warnings and reports them as a list of [WarningData](https://ramses-logic.readthedocs.io/en/latest/classes/WarningData.html)
    * By default, saveToFile performs validation and issues warning logs for each issue
    * This behavior can be altered by calling FileSaveConfig::setValidationEnabled(bool)
* Added possibility to assign and retrieve user IDs to any logic object, see LogicObject::setUserId and LogicObject::getUserId

# v0.15.0

Summary:
* Multiple features, bugfixes and more strict sandboxing
* New Ramses (27.0.116)
* Not backwards compatible
    * binary files need re-export

**Features**

* Added possibility to modify AnimationNode timestamps and keyframes on the fly
    * See AnimationNodeConfig::setExposingOfChannelDataAsProperties which exposes the channel data as node properties
      which can be set or linked as any other logic node properties.
* Can optionally supply asset metadata when saving content
    * see [SaveFileConfig](https://ramses-logic.readthedocs.io/en/latest/classes/SaveFileConfig.html)
    * see [LogicEngine::saveToFile](https://ramses-logic.readthedocs.io/en/latest/classes/LogicEngine.html#_CPPv4N6rlogic11LogicEngine10saveToFileENSt11string_viewERK14SaveFileConfig)
    * Versions are logged in the INFO channel every time LogicEngine::loadFromFile/Buffer is called

**Bugfixes**

* Type definitions declared in modules also work when the data is not wrapped in a function (example from docs works now)
    * Fixes https://github.com/bmwcarit/ramses-logic/issues/39
* Catches error correctly when returning any value from the main Lua script source code
* Fixed memory leak when re-creating LuaScripts (previously, creating above 1000 scripts caused Lua stack overflow)
* Correctly catches writing of global variables everywhere except when using the dedicated GLOBAL table
    * This may break existing Lua code which relied on the lack of checks!
    * Global variable access will be also caught and causes errors - use this to fix such assets
* Correctly catches error when declaring any function other than the predefined ones (init, interface, run)
    * Use syntax like 'GLOBAL.myFunc = function () ... end' inside the init() function to declare global functions
    * Declaring functions twice will result in error
* Improves quality of error messages related to sandbox violations
* Nodes linked to TimerNode's timeDelta will now be always updated (also when timeDelta stays constant)

**API Changes**

* Added AnimationNodeConfig which has to be used when creating AnimationNode (LogicEngine::createAnimationNode)
    * Animation channels are now provided to the config first, where also their validity is checked

# v0.14.2

Fully compatible to Logic Engine 0.14.0.

**Features**

* Weak links - ability to form a cycle in node dependency graph, see LogicEngine::linkWeak.
* Updates inline Ramses version to 27.0.115 (fully backwards compatible)

# v0.14.1

Fully compatible to Logic Engine 0.14.0.

**Features**

* Prebuilt packages for Windows and Debian available with each release from now on

**Bugfixes**

* Package generation for Debian works correctly

# v0.14.0

Summary:
* Fixed clang-tidy jobs for older clang-tidy versions
* File compatible, but ABI-incompatible to v0.13.0

**API Changes**

* [[nodiscard]] properly applied to pure const API getters

**Features**

* Logic engine viewer supports TimerNodes
* Added LogicEngine::setStatisticsLoggingRate and LogicEngine::setStatisticsLogLevel which periodically logs statistics about updated nodes.

**Bugfixes**

* Improved error message when assigning structs with less than required number of fields
* Fixed potential stack overflow problem when assigning large structs
* Reading VECxF and VECxI values from modules works (was broken in 0.13.0 because of read-only protection)

# v0.13.0

Summary:
* Major performance improvement for large scenes with lots of links alongside few bugfixes
* New Ramses (27.0.114)
* Not backwards compatible
    * binary files need re-export

**API Changes**

* Removed LuaScript::overrideLuaPrint
    * Logic engine has better means for debugging since this was introduced
    * Overriding built-in Lua print was a security threat and also violated sandboxing
    * If you still want to get string data out of scripts, just create a string property and read it out
* Can't modify module data any more (this was a technical debt which was fixed in this release)
* Added a built-in methods to make it easier to work with custom types from C++
    * 'rl_len' to get the size of ramses logic built-in types and modules from inside Lua
    * 'rl_next' to create and increment stateless iterators for custom types similar to standard next()
    * 'rl_pairs' and 'rl_ipairs' to iterate custom types similar to standard pairs() and ipairs()
    * See the [Lua docs](https://ramses-logic.readthedocs.io/en/latest/lua_syntax.html#custom-functions) for more info
* Added unique (in the scope of one Logic Engine) and immutable Logic Object Id that will be automatically assigned on object creation.
    *Object Ids start at 1 and are counted upwards for new objects.
* Added convenience method LogicEngine::findLogicObjectById(id) to find object with id regardless of its type
* Added LogicEngineReport, LogicEngine::enableUpdateReport, LogicEngine::getLastUpdateReport which collects statistics about updated nodes.
* Added PropertyEnumToType traits to be able to obtain a C++ data type of a property (inverse of EPropertyTypeToEnum)
* Added TimerNode which is a new logic node to handle timing information for AnimationNode and scripts
* All LogicEngine collection getters and finders were replaced with templated versions:
    * LogicEngine::scripts() -> LogicEngine::getCollection<LuaScript>
      LogicEngine::ramsesNodeBindings() -> LogicEngine::getCollection<RamsesNodeBinding>()
      ...
    * LogicEngine::findScript(name) -> LogicEngine::findByName<LuaScript>(name)
      LogicEngine::findNodeBinding(name) -> LogicEngine::findByName<RamsesNodeBinding>(name)
      ...

**Features**

* Added EPropertyType::Int64 (INT64 in Lua) property type to represent 64bit signed integer in logic network
    * Also added INT32 Lua interface property which is equivalent to INT (INT is kept and act as alias for INT32)
* Animation begin and end can be modified by setting AnimationNode 'timeRange' input property.
* Added Update Report (statistics about updated nodes) to ramses-logic-viewer
* Large scenes with many links now don't cause unnecessary CPU usage when input data didn't change
* Link updates are generally faster after major rework of link handling

**Bugfixes**

* Fixed bug where ramses bindings re-applied their values to ramses when linked, even though the value didn't change
* It's now possible to access array elements by index in interface()
* Improved error messages when converting numeric types. Errors now contain info why the conversion failed,
  i.e. because of numeric rounding, silent truncation, negative numbers etc.. Also works for arrays and VECx types
* Fixed bug in the viewer program where default settings were not exported correctly
* Property::isLinked() works for outputs too, not just inputs
* Linking/unlinking errors contain more useful information

# v0.12.0

**API Changes**

* Re-implemented Lua environment sandboxing
    * This is a safety feature which was temporarily reverted to allow usage of global variables
    * This version reintroduces it - there are now better alternatives:
        * [init() function](https://ramses-logic.readthedocs.io/en/v0.11.0/lua_syntax.html#global-variables-and-the-init-function)
        * [Custom modules](https://ramses-logic.readthedocs.io/en/v0.11.0/lua_syntax.html#custom-modules)
    * Docs for new behavior: see [Lua docs](https://ramses-logic.readthedocs.io/en/v0.12.0/lua_syntax.html#environments-and-isolation)

**Features**

* Added a new tool - a viewer binary which can load, show and configure binary ramses/logic scenes
    * For more info, read the [docs](https://ramses-logic.readthedocs.io/en/v0.12.0/viewer.html#ramses-logic-viewer)

# v0.11.0

**Features**

* Global variables can now be declared in a new function `init()` in Lua scripts using the GLOBAL table
    * See docs or example for more info and exact usage

**Improvements**

* Logic also builds a Ramses renderer now
    * Builds a default renderer for the host system
    * Affected by setting -Dramses-logic_PLATFORM
    * Can still build custom Ramses and inject as target, or find from system
    * See build docs for more details and options
* Added Iterator and collection for LogicObjects to LogicEngine
* Added convenience method LogicEngine::findLogicObject(name) to find object with name regardless of its type
* Added convenience method LogicObject::as<Type> that returns the logic object cast to given concrete type
* Updates Ramses to 27.0.113

# v0.10.2

**Improvements**

* Updates Ramses to 27.0.112

# v0.10.1

**Bugfixes**

* Restore access to global data from the interface() function
    * This was a hardening measure which turned out to be breaking user projects
    * Will be re-introduced after we have added proper support for global variables

# v0.10.0

**API Changes**

* Added LuaModule that can be used to load Lua source code to be shared in one or more Lua scripts
    * LogicEngine::createLuaScriptFromSource and LogicEngine::createLuaScriptFromFile have new optional argument
        to provide list of module dependencies and access alias for each of them
    * Modules can use other modules recursively
* Reworked API for creating scripts (accepts an optional configuration object which can be used to refer to modules)
    * Old API methods (createLuaScriptFromSource and createLuaScriptFromFile) are not available any more
    * New API createLuaScript accepts Lua source code, and optionally configuration object and a name
    * File handling is expected to be performed outside the Logic Engine
        * Rationale: text file loading is not considered a core functionality of the logic engine
    * Also removed corresponding LuaScript::getFilename() method
    * Standard modules must be loaded explicitly via the LuaConfig object
        * When loading older binary files, standard modules are implicitly loaded for compatibility
        * New code should request standard modules explicitly and only where needed

**Bugfixes**

* Property::isLinked() is correctly exported in the shared library
* LogicEngine move constructor and assignment are correctly exported in the shared library

# v0.9.1

Summary: update Ramses to version 27.0.111

**Improvements**

* Node scaling is applied before rotation in Ramses now (see Ramses 27.0.111 changelog)
    * Attention: if your nodes combine scaling and rotations, upgrade to Ramses 27.0.111
      in your runtime or use the built-in ramses shipped with v0.9.1. Otherwise the scene
      may look wrong!


# v0.9.0

Summary:
* New major feature (Animations)
* Not backwards compatible
    * binary files need re-export
    * minimal API changes (see note on RamsesNodeBinding below)
* More small fixes and quality of life changes

**API Changes**

* Added AnimationNode, a logic node that can animate properties
    * Added new entity DataArray for storage of various types of immutable data arrays used in AnimationNode
* RamsesNodeBinding accepts a new enum value which configures how rotations should work
    * Supports multiple Euler angle conventions (a subset of the ones offered by Ramses)
    * Has an option for Quaternion. In this case, the 'rotation' property input is of type vec4f, not vec3f
    * Rotation convention is statically configured in RamsesNodeBinding and can't be changed dynamically any more
    * Rotation values in RamsesNodeBinding are imported from the bound Ramses node if the conventions match, otherwise
        initialized with zero and a warning is issued. Quaternions are always initialized with zero
    * Default rotation convention is Euler XYZ rotation (corresponds to ramses::ERotationConvention::ZYX)
* Added a base class LogicObject for all rlogic objects that can be created from LogicEngine
    * ErrorData now holds pointer to LogicObject, not LogicNode, so that more errors can be reported with pointer to the error object

**Improvements**

* LogicEngine can be move constructed and move assigned using std::move semantics now
* Added the function Property::hasChild that checks whether the Property has a child with the given name
* Can check if property input is linked to an output (new method Property::isLinked)

**Bugfixes**

* Improve error message when trying to assign individual components of Lua script output vector types (VEC3F etc.)

# v0.8.1

**Features**

* Internal Release

# v0.8.0

**API Changes**

* Fixed constness of LogicEngine::find* methods

**Features**

* Allow file compatibility loading, i.e. loading binary file exported with v0.7.0 using runtime v0.8.0
* Added missing size() method for object collections
* Update ramses to 27.0.110

**Bugfixes**

* Numeric checks are applied correctly to primitive types (FLOAT, INT)
* Sol Lua numeric safeties are always active (also in non-debug builds). This ensures consistent behavior for user
* Can't access ARRAY() from within the run() method in Lua anymore
* Can't access global symbols from the interface() function anymore (this blocks the implementation of modules)

# v0.7.0

**Features**

* Struct properties from Lua are now sorted in ascending lexicographic order
* Loading from file or buffer data handles most error cases gracefully, and handles more error cases
    * Ramses object type mismatches after loading result in errors
    * Appearances must be from the same base effect after loading
    * Corrupted data results in graceful errors, not in crashes and undefined behavior

**Breaking changes**

* Ramses Bindings are now statically attached to their Ramses object
    * Ramses object is provided as reference during construction of the binding
    * Can't be changed or set to nullptr
    * Fixes bug when ramses object was re-set and links were not automatically removed (can not happen now by design)
    * Binding input values are initialized with the values from the bound ramses object (all except appearance)
    * See reworked documentation for more details
* Renamed Camera binding inputs to have shorter names
    * See [class docs](https://ramses-logic.readthedocs.io/en/v0.7.0/api.html#_CPPv4N6rlogic19RamsesCameraBindingE) for new names
    * Reason: shorter strings are faster to resolve and easier to read
* Bindings receive their input values from Ramses (after construction and after loading from file/memory)
    * This is more consistent and eliminates data race conditions
* New serialization format (must re-export binary files to use this version of the logic engine)
    * This is a preparation for the first LTS version of the logic engine (API, ABI and file format)

# v0.6.2

**Features**

* Invalid access to properties generate error logs in addition to returning 'false' and 'nullopt'
* Update ramses to 27.0.105

**Bugfixes**

* Setting an input value in Lua results in error, instead of triggering an assert

# v0.6.1

**Features**

* All numeric errors are hard-checked, except rounding of double to float. Added a section in the Lua user docs with details
* Update ramses to 27.0.103

**Bugfixes**

* Fix conversion of very large numbers to int32 when crossing the Lua to C boundary (used to clamp them wrongly)
* Correctly catch and report error when setting negative or zero camera viewport sizes to RamsesCameraBindings

# v0.6.0

This release contains minor API improvements and a new feature (RamsesCameraBinding), which requires a change in the serialization format. Therefore
it is not API and binary compatible with 0.5.3!

**Features**

* [API break!] Log message types are consistent with those of Ramses and DLT. Existing ones are the same (but camel-case instead of upper case).
  New log levels added (Off, Fatal, Debug and Trace)
* Update ramses to 27.0.102
* Add RamsesCameraBinding to control RamsesCamera
* Setting output and linked input Properties with `Property::set<T>` is now treated as an error
* [API breaking!] `LogicEngine::getErrors()` returns a vector of structs holding additional error information.

**Bugfixes**

* Fix exotic bug where loading from file/buffer silently ignores ramses objects with mismatched types

# v0.5.3

**Features**

* Possible to load from memory buffer instead of file
* Bindings' name is empty string by default
* Upgrade ramses to 27.0.101
* Possible to rename objects after their creation

**Bugfixes**

* `LogicEngine::loadFromFile()` and `loadFromBuffer()` check for data corruption by default (possible to disable)
* `LogicEngine::saveToFile()` fails if target is a folder; `loadFromFile()` does not crash when giving folder as a path

# v0.5.2

**Features**

* Upgraded Ramses that ships with the logic engine from 27.0.5 to 27.0.100 (backwards compatible)

# v0.5.0

This version is API-compatible to v0.4.2, but the file-format is not backwards compatible. Please re-export
all binary content when switching to v0.5.0!

**Features**

* Upgraded Ramses that ships with the logic engine from 27.0.2 to 27.0.5 (backwards compatible)
* Added support for the '#' operator on non-primitive properties.
  Semantics is same as for Lua tables - returns the number of elements in the array/struct. Also works for vec2/3/4 (returns 2/3/4)
* RamsesAppearanceBinding supports uniform arrays
* Possible to set log verbosity to dynamically adjust how much is being logged
* Added convenience methods `find<Object>(name)` in LogicEngine
* Can set rotation convention on RamsesNodeBinding. Default is still euler XYZ.
* RamsesNodeBinding's visibility and scaling inputs have the same values like Ramses after initialization
* Possible to build with ramses renderer

**Semantic changes**

* Changed behavior of `RamsesBinding`. See
  [binding docs](https://ramses-logic.readthedocs.io/en/v1.0.0/api.html#linking-logic-nodes-to-ramses-scenes) for details.
  See [data flow docs](https://ramses-logic.readthedocs.io/en/latest/api.html#data-flow) for general overview
  how data flow works in the logic engine.

**Bugfixes**

* `LogicEngine::saveToFile()` correctly reports error if two or more different Ramses scenes are being referenced by binding objects
* Logic engine crashes no more when nodes with nested links (script, array) are destroyed
* `LogicEngine::unlink()` will correctly report error when called with non-primitive properties
* `LogicEngine::isLinked()` returns consistent results for nested links (any link, also nested, will cause a node to be reported as linked)
* Fixed a bug where LogicNode update order was not correctly adjusted when removing and adding links to its properties
* Reports error when a cycle of links is created (`update()` and `saveToFile()` will fail until the cycle is resolved)

# v0.4.2

**Features**

* Added benchmarks for basic functionality. Enabled when unit tests are enabled, works based on google-benchmark.
  To run them, execute the `benchmarks` executable after having built the project in release mode for maximum accuracy.

**Bugfixes**

* Fixed an exotic bug related to links and deserialization.
  Used to trigger when deserializing twice from file which had links.
* Does not wrongly create array properties out of GLSL uniforms arrays in RamsesAppearanceBinding.
  Array feature not supported there yet!
* Does not wrongly create LogicNode inputs out of semantic uniforms in RamsesAppearanceBinding.

**Other**

* Removed support for i/o and os lua libs. See [lua module docs](https://ramses-logic.readthedocs.io/en/latest/lua_syntax.html#using-lua-modules) for details

# v0.4.1

**Bugfixes**

* Fix bug which broke links created between complex data objects

**Improvements**

* Added missing error reporting when trying to link arrays directly

**Dependencies**

* Added new dependency: google benchmarks, a library for benchmarking

# v0.4.0

**New Features**

* Support arrays of complex types
* Added more logging
* Upgrade Ramses to v27.0.2

    * Uses correct rotation semantics fixed in Ramses 27.0.1
    * Currently hardcoded right-handed XYZ Euler rotation (same as Blender default)

**Improvements**

* Added `[[nodiscard]]` attribute to API methods where it makes sense, mostly getter Methods
  This will trigger compiler warnings if you call these methods but don't use the result
* New CMake option 'ramses-logic_FOLDER_PREFIX' to set custom folder prefix for MSVS
* Restructured folders for easier source redistributions.
  See [docs](https://ramses-logic.readthedocs.io/en/latest/dev.html#source-contents) for more info

# v0.3.1

**Bugfixes**

* Fixed a bug which caused a crash when unlinking and destroying nodes
* Upgrade ramses from 26.0.4 -> 26.0.6 (fixes important resource creation bug)

**Improvements**

* `Property::set<T>` and `Property::get<T>`  trigger a  static assert when used with the wrong type T
* Add a few debug logs, mainly aimed at debugging if/when logic nodes are updated based on their input changes (only published on custom logger)
* Errors are now also logged in the order of their appearance, both in console logger and in custom logger

**Dependencies**

* Updated googletest to a newer version (fixes some clang-tidy issues)

# v0.3.0

**New Features**

* Optimization to only execute LogicNodes with changed inputs
* Support arrays of primitives

**Bugfixes**

* Const-iterators can be initialized from non-const iterators

**Improvements**

* Check Ramses version during build time to ensure compatibility
* `loadFromFile()` checks ramses version for compability
* Lua Scripts have all standard Lua modules by default (see docs for details)
* Currently supports ramses >= 26.0.4 and < 27

**Build system**

* Provides version info as CMake Cache variable
* Fails build if ramses version is not compatible

# v0.2.0

**Bugfixes**

* Fixed a bug with recent sol and Visual Studio 16.7.4

    * Only a workaround, until properly fixed in sol + MSVS
    * Results in minor mismatch in reported errors when using VECx types
    * Errors are still readable and have a stack trace, just the message is different

**Features**

* Improved class hierarchy:

    * All binding-classes inherit from RamsesBinding
    * Scripts and RamsesBinding inherit from LogicNode
    * Can call destroy(LogicNode&) for all object types now

* RamsesAppearanceBinding class for manipulation of RAMSES appearances.
* Linking of outputs of LogicNodes to inputs of other LogicNodes, with some limitations:

    * No checks for cycles yet
    * Must link struct properties one-by-one
    * Some error checks missing (see API docs of link())
    * LogicNode has "isLinked" function for checking if a LogicNode is linked

* Iterators and collections to iterate over objects of LogicEngine class
* Saving and loading of LogicEngine to and from files
* Upgrade to Ramses v.26.0.4 (from v25.0.6)
* Added API to obtain version of ramses logic
* Added CMake option to disable installation of Ramses Logic

    * Does not affect ramses installation (Ramses has no such option yet)
    * Sol doesn't support disabling of installation - Sol headers are still installed

* Improved documentation

**Fixes**

* Remove flatbuffers targets from build

# v0.1.0

First version published on Github

**Initial features**

* Script loading and execution
* Script input/output access from C++
* Supported property types: bool, string, float, integers, vec[2|3|4][f|i]
* Basic debugging support

    * error handling support with full lua stack information and human-readable error descriptions
    * override print() method in Lua
    * default logger with different log levels
    * option to override default logging with custom logger

* RamsesNodeBindings to control ramses node properties (visibility, transformation)


```{warning} RamsesNodeBindings still can't be linked to script outputs, this feature is coming soon
```

* Code examples with description of API usage and semantics
* Documentation based on sphinx
* Possible to build as a static and dynamic library
* Possible to install, package, or build standalone using CMake
* Embeddable to other projects via CMake add_subdirectory()
