======
master
======

======
v0.1.0
======

First version published on Github

------------------
Initial features
------------------

* Script loading and execution
* Script input/output access from C++
* Supported property types: bool, string, float, integers, vec[2|3|4][f|i]
* Basic debugging support

    * error handling support with full lua stack information and human-readable error descriptions
    * override print() method in Lua
    * default logger with different log levels
    * option to override default logging with custom logger

* RamsesNodeBindings to control ramses node properties (visibility, transformation)

.. warning::

    RamsesNodeBindings still can't be linked to script outputs, this feature is coming soon

* Code examples with description of API usage and semantics
* Documentation based on sphinx
* Possible to build as a static and dynamic library
* Possible to install, package, or build standalone using CMake
* Embeddable to other projects via CMake add_subdirectory()
