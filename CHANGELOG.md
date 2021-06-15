# master

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
  [binding docs](https://ramses-logic.readthedocs.io/en/latest/api.html#linking-scripts-to-ramses-scenes) for details.
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
