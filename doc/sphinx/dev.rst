.. _developer-docs:

The following sections are aimed at ``RAMSES Logic`` developers and contributors. If you simply
want to use the library, stop reading here, unless you are interested in the internal workings
of the code and are not afraid of technical details!

=====================================================
Understand RAMSES Logic architecture and design
=====================================================

-----------------------------------------------------
Current Architecture
-----------------------------------------------------

TODO (Violin/Sven)

-----------------------------------------------------
Design decision log
-----------------------------------------------------

The following list documents the design decisions behind the ``RAMSES Logic`` project in
inverse-chronological order (latest decisions come on top).

* Do we support animations?
    Yes, but they are not implemented yet.
    TODO (Violin/Sven) document this once we have fully decided
* How to implement serialization?
    Serialization is implemented using Flatbuffers - a library which allows binary optimizations
    when loading objects which are flat in memory.
* Static vs. shared library
    Both are supported, but shared lib is the preferred option. The public API is designed in a way
    that no memory allocation is exposed, so that DLLs on Windows will not have the problem of
    incompatible runtimes.
* Which version of ``C++`` to use?
    As with CMake, ``C++`` version is a tradeoff between modern code and compatibility.
    We settled on ``C++17`` so that we can use some of the modern features like ``string_view``,
    and ``std::optional`` and ``std::variant``.
* Which version of``CMake`` to use?
    ``CMake`` is an obvious choice for any C/C++ project as it is de-facto industry standard
    for cross-platform development. There is a tradeoff between cleaniness and compatibility when
    choosing a minimal CMake version, and we settled on version 3.13, despite it being relatively
    new and not installed by default on the majority of Linux distributions. CMake made great improvements
    over the last few versions, and 3.13 specifically offers much better ways to install and package
    projects, as well as many QoL changes.
* Which tool to use for documentation
    We use a combination of Doxygen, Sphinx and Breathe. Doxygen because it has great support
    for inline C++ documentation and lexer capabilities. Sphinx because it is industry standard
    and provides great flexibility in putting multiple different projects under the same umbrella.
    Breathe is a bridge between Doxygen and Sphinx which provides beautiful html output from
    raw Doxygen output.
* Plain ``Lua`` vs. ``Sol`` wrapper
    Since ``Lua`` has a low-level API in C which requires careful handling of the stack as well
    as working with raw pointers and casts, we decised to use ``sol`` - a great C++ wrapper for
    Lua which also provides easy switch between different ``Lua`` implementations with negligable
    performance overhead.
* Should scripts interact between each other?
    One of the difficulties of script environments is to define boundaries and state of scripts.
    It is often required that data is passed between scripts, but it is highly undesirable that
    scripts global variables influence each other and create unexpected side effects. To address
    this we put each script in its own ``Lua`` environment, but allow scripts to pass data to
    each other by explicitly linking one script's output(s) to another script's input. This can
    be done during asset design time or during runtime.
* What is the interface of scripts to the ``RAMSES Logic`` runtime?
    Based on the latter decision, we had to define how ``Lua`` scripts interact with the C++ side of
    the runtime. After lengthy discussions and considering various different options, we settled on
    a solution where each ``Lua`` script declares explicitly its interface via a special function with the
    same name. There is a differentiation between inputs and outputs which also defines when a script will be
    executed, namely when it's inputs values changed. Script payload must be placed in another special function
    called ``run`` which can access inputs as declared, and also set outputs' values.
* How should scripts interact with the ``RAMSES`` scene(s)?
    ``Lua`` offers multiple ways to interact between scripts and C++. The two main options we considered:

        * provide custom classes and overload them in C++ code (e.g. RamsesNode type in ``Lua``)
        * provide interface in ``Lua`` to set predefined types (e.g. int, float, vec3, etc.) and link them to C++ objects

    We chose the second option because it allows decoupling of the ``Lua`` scripts from the actual 3D scene, and also allows
    the ``RAMSES Composer`` which uses the ``Lua`` runtime to generically link the scripts
    properties to ``RAMSES`` objects.
* Which tools to use for CI/testing?
    All CI tools are based on Docker for two reasons: Docker is industry standard for reproducible
    build environments, and it nicely abstracts tool installation details from the user - such as Python
    environment, extensions, tool versions etc. Also, the Docker image recipe can be used as a baseline
    for a custom build environment or debug build issues arising from using special compilers. The concrete
    tools we use for quality checks are: Googletest, clang-tidy, valgrind, and a bunch of custom python scripts
    for license checking and code-style. We also use llvm tools for test coverage and statistics.
* Which approach to use for continuous integration?
    There are multiple open platforms for CI available for open source projects, but all of them
    have limitations, mostly in the capacity. Therefore we chose to use an internal commercial CI system
    for the initial development of the project in order to not hit limitations. It is planned to switch to
    an open platform in future (i.e. Github actions, CircleCI or similar).
* Should we support JIT compilation of scripts?
    Currently we use standard Lua, no JIT, but we maintain the possibility to switch to LuaJIT in the
    future.
* Which version of ``Lua``?
    We chose Lua 5.1 as it is still compatible with LuaJIT and enables potentially compiling scripts
    dynamically in a performance-friendly way.
* Which scripting language to use?
    ``Lua`` was an easy choice, because it provides by far the best combination between extensibility
    and performance. The stack concept of Lua provides unprecendented flexibility for custom extensions
    in C++ code, and the metatable approach provides great way to provide object oriented features in a
    pure scripting language. The other option we considered was Python due to its power and popularity, but
    ultimately ruled out as an option due to the size of the interpreter itself and the complexity of it
    which is not required for real time graphics applications.
* Should we make separate library or embed support in ``RAMSES`` directly?
    ``RAMSES`` is designed to be minimalistic and closely aligned to OpenGL. Even though it would be more
    convenient to have a single library, we decided it's better to create a separate lib/module so that
    users which don't need the scripting support are not forced with it, and we can also be more flexible
    with the choice of technology.
* Should scripts be executed remotely (renderer), or client-side only? Or both?
    ``RAMSES`` provides distribution support for graphical content - in fact that's the primary feature
    of ``RAMSES`` that distinguishes it from other engines. We had the choice to whether to make scripting
    execution also remote (i.e. executed on a renderer component rather than a client component). There are
    pros and cons for both approaches, but ultimately we decided to implement a client-side scripting runtime
    in favor of security and stability concerns. Sending scripts to a remote renderer poses a security risk
    mostly for embedded devices which must fulfill safety and quality criteria, and the benefits of executing
    scripts remotely is comparatively small.
* Should we add scripting support to ``RAMSES``?
    As more and more users of ``RAMSES`` use the rendering engine, various applications had to
    find a solution to the lack of scripting capabilities. We tried several solutions - code generation,
    proprietary scripting formats, as well as implementing everything in C++ purely. Reality showed that
    these solutions do very similar things - abstract and control the structure of ramses scene(s) and
    they even shared the same code, for e.g. animation, grouping of nodes, offscreen rendering. Thus we
    decided that scripting support would provide a common ground for implementing such logic and abstraction.

========================================
Developer guidelines
========================================

Apart from the general project architecture and design decisions outlined above, we try to be as open to change as
possible. Want to try out something new? Feel free to experiment with the codebase and propose a change
(see :ref:`contributing <Contributing>`).

Here are some additional things to consider when modifying the project:

* Execute existing tests before submitting PRs and write new tests for new functionality
* Have an idea or feature request? Feel free to create a Github issue
* Treat documentation at least as good as the code itself

    * Added a new feature? Mention it in the CHANGELOG!
    * Modified the public API? Adapt the doxygen documentation!
    * Changed the serialization format? Re-run the FlatbufGen target in CMake
    * Made an API, ABI or any other incompatible change? Don't forget to bump the semantic version of the project
      in the main CMakeLists.txt file

.. note::

    When changing the serialization schemes of Ramses Logic, the generated flatbuffers header files have to be
    re-generated and committed as part of the change. This may seem counter-intuitive for code generation, but not
    all build system support dynamically generated code. Hence, we keep the generated header files checked in with
    the rest of the code, and re-generate them when needed. To re-create those files, execute the ``FlatbufGen`` target
    on your build system.

========================================
Contributing
========================================

.. include:: ../../CONTRIBUTING.rst
