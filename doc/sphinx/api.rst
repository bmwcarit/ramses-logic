.. default-domain:: cpp
.. highlight:: cpp

=========================
Overview
=========================

.. note::

    Prefer learning by example? Jump straight to the :ref:`examples <List of all examples>`!

The entry point to ``RAMSES logic`` is a factory-style class :class:`rlogic::LogicEngine` which can
create instances of all other types of objects supported by ``RAMSES Logic``:

* :class:`rlogic::LuaScript`
* :class:`rlogic::RamsesNodeBinding`
* :class:`rlogic::RamsesAppearanceBinding`

You can create multiple instances of :class:`rlogic::LogicEngine`, but each copy owns the objects it
created, and must be used to destroy them, as befits a factory class.

You can create scripts using the :class:`rlogic::LogicEngine` class like this:

.. code-block::
    :linenos:
    :emphasize-lines: 5-14,16-17

    #include "ramses-logic/LogicEngine.h"

    using namespace ramses::logic;

    std::string source = R"(
        function interface()
            IN.gear = INT
            OUT.speed = FLOAT
        end

        function run()
            OUT.speed = IN.gear * 15
        end
    )"

    LogicEngine engine;
    LuaScript* script = engine.createLuaScriptFromSource(source, "simple script");
    script->getInputs()->getChild("gear")->set<int32_t>(4);

    script->execute();
    float speed = script->getOutputs()->getChild("speed")->get<float>();
    std::cout << "OUT.speed == " << speed;

The syntax required in all scripts is plain ``Lua`` (version 5.1) with the special addition of the
special ``interface()`` and ``run()`` functions which every script needs to have. The ``interface()``
function is the place where script inputs and outputs are declared using the special global objects
``IN`` and ``OUT``. The ``run()`` function can access the values of inputs and may set
the values of outputs using the ``IN`` and ``OUT`` globals respectively.

Even though the ``IN`` and ``OUT`` objects are accessible in both ``interface()`` and ``run()`` functions,
they have different semantics in each function. The ``interface`` function only **declares** the interface
of the script, thus properties declared there can **only have a type**, they don't have a **value** yet -
similar to function signatures in programming languages.

In contrast to the ``interface()`` function, the ``run()`` function can't declare new properties any more,
but the properties have a value which can be read and written.

The ``interface()`` function is only ever executed once - during the creation of the script. The ``run()``
function is executed every time one or more of the values in ``IN`` changes, either when the ``C++`` application
changed them explicitly, or when any of the inputs is linked to another script's output whose value changed.

For more information about ``Lua`` scripts, refer to the :class:`rlogic::LuaScript` class documentation.

You can :ref:`link scripts <Creating links between scripts>` to form a more sophisticated logic execution graph.

You can :ref:`bind to Ramses objects <Linking scripts to Ramses scenes>` to control a 3D ``Ramses`` scene.

Finally, the :class:`rlogic::LogicEngine` class and all its content can be also saved/loaded from a file. Refer to
:ref:`the section on saving/loading from files for more details <Saving/Loading from file>`.

==================================================
Creating links between scripts
==================================================

One of the complex problems of 3D graphics development is managing complexity, especially for larger projects.
For that purpose it is useful to split the application logic into multiple scripts, so that individual scripts
can remain small and easy to understand. To do that, ``Ramses Logic`` provides a mechanism to link script
properties - either statically or during runtime. Here is a simple example how links are created:

.. code-block::
    :linenos:

    LogicEngine logicEngine;
    LuaScript* sourceScript = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            OUT.source = STRING
        end
        function run()
            OUT.source = "World!"
        end
    )");

    LuaScript* destinationScript = logicEngine.createLuaScriptFromSource(R"(
        function interface()
            IN.destination = STRING
        end
        function run()
            print("Hello, " .. IN.destination)
        end
    )");

    logicEngine.link(
        *sourceScript->getOutputs()->getChild("source"),
        *destinationScript->getInputs()->getChild("destination"));

    // This will print 'Hello, World!' to the console
    logicEngine.update();


In this simple example, the 'sourceScript' provides string data to the 'destinationScript' every time the  ``LogicEngine::update``
method is called. The 'destinationScript' receives the data in its input property and can process  it further. After
two scripts are linked in this way, the :class:`rlogic::LogicEngine` will execute them in a order which ensures data consistency, i.e.
scripts which provide data to other scripts' inputs are executed first. In this example, the 'sourceScript' will be executed before
the 'destionationScript' because it provides data to it over the link.

Creating links as shown above enforces a so-called 'directed acyclic graph', or ``DAG``, to the :class:`rlogic::LogicNode` inside a given
:class:`rlogic::LogicEngine`. In order to ensure data consistency, this graph can not have cyclic dependencies, thus following error
conditions will cause undefined behavior:

.. todo: (Violin) Fix docs after we have implemented cycle detection

* A :class:`rlogic::LogicNode` can not create links to itself
* Node A can not be linked to node B if node B is linked to node A (links have a direction!)
* Any set of :class:`rlogic::LogicNode` instances whose links form a (directed) circle, e.g. A->B->C->A

A link can be removed in a similar fashion:

.. code-block::
    :linenos:

    logicEngine.unlink(
        *sourceScript->getOutputs()->getChild("source"),
        *destinationScript->getInputs()->getChild("destination"));

ATTENTION: Currently it is not possible to link structured data to other structured data directly. If you want to link all parts of a structured data,
you need to link each property individually.

For more detailed information on the exact behavior of these methods, refer to the documentation of the :func:`rlogic::LogicEngine::link`
and :func:`rlogic::LogicEngine::unlink` documentation.

==================================================
Linking scripts to Ramses scenes
==================================================

Lua scripts would not make much sense on their own if they can't interact with ``Ramses`` scene objects. The way to
link script output properties to ``Ramses`` scene objects is by creating :class:`rlogic::RamsesBinding` instances and linking their inputs to scripts' outputs.
There are different binding types depending on the type of ``Ramses`` object - refer to :class:`rlogic::RamsesBinding` for the full list of derived classes.
Bindings can be linked in the exact same way as :ref:`scripts can <Creating links between scripts>`. In fact, they derive from the
same base class - :class:`rlogic::LogicNode`. The only
difference is that the bindings have only input properties (the outputs are implicitly defined and statically linked to the Ramses
objects attached to them), whereas scripts have inputs and outputs explicitly defined in the script interface.

One might wonder, why not allow to directly link script outputs to ``Ramses`` objects?
The reason for that is two-fold:

* Separation of concerns between pure script logic and ``Ramses``-related scene updates
* This allows to handle all inputs and outputs in a generic way using the :class:`rlogic::LogicNode` class' interface from
  which both :class:`rlogic::LuaScript` and :class:`rlogic::RamsesNodeBinding` derive


.. note::
    Bindings alre always 'updated' in the end of :func:`rlogic::LogicEngine::update`, regardless how they are linked. This
    ensures that updates to the ``Ramses`` scene are only ever applied if all of the ``Lua`` scripts ran without errors. This prevents
    partial graphics updates if a any of the scripts has an error.

If you destroy a still linked LogicNode with :func:`rlogic::LogicEngine::destroy`, all links from and to this :class:`rlogic::LogicNode` will
be removed aswell.

=========================
Error handling
=========================

Some of the ``RAMSES Logic`` classes' methods can issue errors when used incorrectly or when
a ``Lua`` script encounters a compile-time or run-time error. Dealing with such errors gracefully
can be done like this:

.. code-block::
    :linenos:

    #include <iostream>

    ARamsesLogicClass object;
    bool success = object.doSomething();
    if(!success)
    {
        for(auto& error: object.getErrors())
        {
            // Or do something else with the error
            std::cout << "Encountered error: " << error << std::endl;
        }
    }

===============================
Print messages from within Lua
===============================

In common ``Lua`` code you can print messages e.g. for debugging with the "print" function.
Because ramses-logic can be used in different environments which not always have a console
to print messages, the "print" function is overloaded. The default behavior is that your
message will be piped to std::cout together with the name of the calling script.
If you need more control of the print messages, you can overload the printing function with
you own one like this:

.. code-block::
    :linenos:

    LogicEngine logicEngine;
    LuaScript* script = logicEngine.createLuaScriptFromSource(R"(
        function interface()
        end
        function run()
            print("message")
        end
    )");

    script->overrideLuaPrint([](std::string_view scriptName, std::string_view message){
        std::cout << scriptName << ": " << message << std::endl;
    });

=====================================
Iterating over object collections
=====================================

Iterating over objects can be useful, for example when :ref:`loading content from files <Saving/Loading from file>`
or when applying search or filter algorithms over all objects from a specific type.
The :class:`rlogic::LogicEngine` class provides iterator-style access to all of its objects:

.. code-block::
    :linenos:

    LogicEngine logicEngine;
    Collection<LuaScript> allScripts = logicEngine.scripts();

    for(const auto script : allScripts)
    {
        std::cout << "Script name: " << script->getName() << std::endl;
    }

The :class:`rlogic::Collection` class and the iterators it returns are STL-compatible, meaning that you can use them with any
other STL algorithms or libraries which adhere to STL principles. The iterators implement ``forward`` iterator semantics
(`have a look at C++ docs <https://en.cppreference.com/w/cpp/named_req/ForwardIterator>`_).

.. note::

    The :class:`rlogic::Iterator` and :class:`rlogic::Collection` classes are not following the ``pimpl`` pattern as the rest of
    the ``Ramses Logic`` to performance ends. Be careful not to depend on any internals of the classes (mostly the Internally
    wrapped STL containers) to avoid compatibility problems when updating the ``Ramses Logic`` version!

=====================================
Saving/Loading from file
=====================================

The :class:`rlogic::LogicEngine` class and its content can be stored in a file and loaded from file again using the functions
:func:`rlogic::LogicEngine::saveToFile` and :func:`rlogic::LogicEngine::loadFromFile`. The latter has an optional argument
to provide a ``Ramses`` scene which should be used to resolve references to Ramses objects in the Logic Engine file. Read
further for more details.

.. note::

    Even though it would be technically possible to combine the storing and loading of Ramses scenes together with the Logic Engine
    and its scripts in a single file, we decided to not do this but instead keep the content in separate files and load/save it independently.
    This allows to have the same Ramses scene stored multiple times or with different settings, but using the same logic content,
    as well as the other way around - having different logic implementations based on the same Ramses scene. It also leaves more freedom
    to choose how to store the Ramses scene.

--------------------------------------------------
Object lifecycle when saving and loading to files
--------------------------------------------------

After loading,
the current state of the object will be completely overwritten by the contents from the file. If you don't want this behavior,
use two different instances of the class - one dedicated for loading from files and nothing else.

Here is a simple example which demonstrates how saving/loading from file works in the simplest case (i.e. no references to Ramses objects):

.. code-block::
    :linenos:

    // Creates an empty LogicEngine instance, saves it to file and destroys the object
    {
        rlogic::LogicEngine engine;
        engine.saveToFile("logicEngine.bin");
    }
    // Loads the file we saved above into a freshly created LogicEngine instance
    {
        rlogic::LogicEngine engine;
        engine.loadFromFile("logicEngine.bin");
    }

After the call to :func:`rlogic::LogicEngine::loadFromFile` successfully returns (refer to the :ref:`Error handling` section
for info on handling errors), the state of the :class:`rlogic::LogicEngine` class will be overwritten with
the contents loaded from the file. This implies that all objects created prior loading will be deleted and pointers to them
will be pointing to invalid memory locations. We advise designing your object lifecycles around this and immediately dispose
such pointers after loading from file.

.. warning::

    In case of error during loading the :class:`rlogic::LogicEngine` may be left in an inconsistent state. In the future we may implement
    graceful handling of deserialization errors, but for now we suggest discarding a :class:`rlogic::LogicEngine` object which failed to load.

--------------------------------------------------
Saving and loading together with a Ramses scene
--------------------------------------------------

In a slightly less simple, but more realistic setup, the Logic Engine will contain objects of type ``Ramses<Object>Binding`` which
contain references to Ramses objects. In that case, use the optional ``ramses::Scene*`` argument to :func:`rlogic::LogicEngine::loadFromFile`
to specify the scene from which the references to Ramses objects should be resolved. ``Ramses Logic`` uses the ``getSceneObjectId()`` method of the
``ramses::SceneObject`` class to track references to scene objects. This implies that those IDs must be the same after loading, otherwise
:func:`rlogic::LogicEngine::loadFromFile` will report error and fail. ``Ramses Logic`` makes no assumptions on the origin of the scene, its name
or ID.

For a full-fledged example, have a look at `the serialization example <https://github.com/GENIVI/ramses-logic/tree/master/examples/06_serialization>`_.

=====================================
Additional Lua syntax specifics
=====================================

``RAMSES Logic`` fuses ``C++`` and ``Lua`` code which are quite different, both in terms of language semantics,
type system and memory management. This is mostly transparent to the user, but there are some noteworthy
special cases worth explaining.

-----------------------------------------------------
The global IN and OUT objects
-----------------------------------------------------

The ``IN`` and ``OUT`` objects are global ``Lua`` variables accessible anywhere. They are so-called user
types, meaning that the logic to deal with them is in ``C++`` code, not in ``Lua``. This means that any kind of
error which is not strictly a ``Lua`` syntax error will be handled in ``C++`` code. For example, assigning a boolean value
to a variable which was declared of string type is valid in ``Lua``, but will cause a type error when using
``RAMSES Logic``. This is intended and desired, however the ``Lua`` VM will not know where this error comes from
other than "somewhere from within the overloaded ``C++`` code". This, stack traces look something like this
when such errors happen:

.. code-block:: text

    lua: error: Assigning boolean to string output 'my_property'!
    stack traceback:
        [C]: in ?
        [string \"MyScript\"]:3: in function <[string \"MyScript\"]:2>

The top line in this stack is to be interpreted like this:

* The error happened somewhere in the ``C`` code (remember, ``Lua`` is based on ``C``, not on ``C++``)
* The function where the error happened is not known (**?**) - ``Lua`` doesn't know the name of the function

The rest of the information is coming from ``Lua``, thus it makes more sense - the printed error message originates
from ``C++`` code, but is passed to the ``Lua`` VM as a verbose error. The lower parts of the stack trace are
coming from ``Lua`` source code and ``Lua`` knows where that code is.

Furthermore, assigning any other value to the ``IN`` and ``OUT`` globals is perfectly fine in ``Lua``, but will
result in unexpected behavior. The ``C++`` runtime will have no way of knowing that this happened, and will
not receive any notification that something is being written in the newly created objects.

-----------------------------------------------------
Things you should never do
-----------------------------------------------------

There are other things which will result in undefined behavior, and ``RAMSES Logic`` has no way of capturing
this and reporting errors. Here is a list:

* Assign ``IN`` directly to ``OUT``. This will not have the effect you expect (assigning values), but instead it
  will set the ``OUT`` label to point to the ``IN`` object, essentialy yielding two *pointers* to the same object - the ``IN`` object.
  If you want to be able to assign all input values to all output values, put them in a struct and assign the struct, e.g.:

.. code:: lua

    function interface()
        IN.struct = {}
        OUT.struct = {} -- must have the exact same properties as IN.struct!!
    end

    function run()
        -- This will work!
        OUT.struct = IN.struct
        -- This will not work!
        OUT = IN
    end

* Do anything with ``IN`` and ``OUT`` in the global script scope - these objects don't exist there. However, you
  can pass ``IN`` and ``OUT`` as arguments to other functions, but consider :ref:`Special case: using OUT object in other functions`
* Calling the ``interface()`` method from within the ``run()`` method or vice-versa
* Using recursion in the ``interface()`` or ``run()`` methods
* Overwriting the ``IN`` and ``OUT`` objects. Exception to this is assigning ``OUT = IN`` in the ``run()`` method
* using threads or coroutines. We might add this in future, but for now - don't use them

-----------------------------------------------------
Things you should avoid if possible
-----------------------------------------------------

Even though it is not strictly prohibited, it is not advised to store and read global variables
inside the ``run()`` function, as this introduces a side effect and makes the script more vulnerable
to errors. Instead, design the script so that it needs only be executed if the values of any of the
inputs changed - similar to how functional programming works.
:class:`rlogic::LogicNode` provides an interface to access the inputs and outputs declared by the ``interface()``
function - see :func:`rlogic::LogicNode::getInputs()` and :func:`rlogic::LogicNode::getOutputs()`.

-----------------------------------------------------
Special case: using OUT object in other functions
-----------------------------------------------------

It is possible to pass the OUT struct from the run() function to a different function to set the output values.
But be aware that not all constellations are working. Here are some examples to explain the working variants:

.. code-block:: lua
    :linenos:
    :emphasize-lines: 13,18-20

    function interface()
        OUT.param1 = INT
        OUT.struct1 = {
            param2 = INT
        }
    end

    function setParam1(out)
        out.param1 = 42 -- OK
    end

    function setDirect(p)
        p = 42 -- NOT OK: Will create local variable "p" with value 42
    end

    function setStruct(struct)
        struct.param2 = 42 -- OK
        struct = {
            param2 = 42 -- NOT OK: Will create local variable "struct" with table
        }
    end

    function run()
        setParam1(OUT)
        setDirect(OUT.param1)
        setStruct(OUT.struct1)
    end

As the above example demonstrates, passing objects to functions in ``Lua`` is done by reference. However, whenever the
reference is overwritten with something else, this has no effect on the object which was passed from outside, but only
lets the local copy of the reference point to a different value.

=========================
Logging
=========================

Internally there are four log levels available.

* Info
* Debug
* Warn
* Error

By default all internal logging messages are sended to std::cout. If you want to handle the messages yourself,
you can register your own log handler function. This function is called each time a log message occurs.

.. code-block::
    :linenos:

    #include <iostream>

    Logger::SetLogHandler([](ElogMessageType msgType, std::string_view message){
        switch(type)
        {
            case ELogMessageType::ERROR:
                std::cout << "Error: " << message << std::endl;
                break;
            default:
                std::cout << message << std::endl;
                break;
        }
    });

Inside the log handler function, you get the type of the message and the message itself as a std::string_view.
Keep in mind, that you can't store the std::string_view. It will be invalid after the call to the log handler
function. If you need the message for later usage, store it in a std::string.

Note that having a custom logger does not disable the default logging - you have to do this explicitly
if you don't want to see the ramses logic default logs using :func:`rlogic::Logger::SetDefaultLogging`.


======================================
Security and memory safety
======================================

One of the biggest challenges of modern ``C++`` is finding a balance between compatibility with older compilers
and platforms, while not sacrificing memory safety and code readibility. In the ``RAMSES`` ecosystem we try to
find a good balance by testing with different compilers, employing automation techniques and making use of
modern compiler-based tools to perform static code analysis and introspection. The methods and tools we use are:

* compiling on different compilers (MSVC, gcc, clang) with strict compiler settings
* clang-tidy with fairly strict configuration
* valgrind
* treat warnings as errors
* use various clang-based sanitizers (undefined behavior, thread sanitizer, address sanitizer)

Those tools cover a lot of the standard sources of problems with ``C++`` revolving around memory. We also uphold
a strict code review, ensuring that each line of code is looked at by at least two pairs of eyes, for critical
parts of the code usually more than that. Still, no project is safe from bugs, thus we recommend following
some or all of the additional conventions and best practices from below subchapters to minimize the risk of
memory-related bugs and malicious attacks when using ``Ramses Logic``.

-----------------------------------------------------
Additional memory safety measures
-----------------------------------------------------

One of the biggest sources of bugs and security problems in ``C++`` arise from memory management, both in terms of
allocation/deallocation and memory access and boundary checks. ``Ramses Logic`` takes care of memory lifecycle
for all objects created by it, and provides raw pointer access to their memory. We suggest creating your own wrapper
objects for anything created or loaded by the :class:`rlogic::LogicEngine` class and ensure it is destroyed exactly once
and only after not used any more.

Furthermore, pay special attention when passing strings as ``std::string_view`` to and from the ``Logic Engine`` as those
may not be terminated by a 0 and may lead to out of bounds accesses when used by functions expecting 0-termination.

-----------------------------------------------------
Additional security considerations
-----------------------------------------------------

``Lua`` is a script language, and as such provides great flexibility and expresiveness at the cost of
more error potential and security risks compared to other techniques for describing logic. The ``Logic engine`` and the
underlying ``sol`` library do a lot of error checking and prevents undefined behavior by executing faulty script code,
but there are cases which can't be checked.

To give one example, a script may overwrite the global variables ``IN`` or ``OUT``
from within script code because of the nature of ``Lua`` scripts. This can't be automatically checked by the runtime without
overloading the global ``Lua`` metatable and injecting every single assignment operation, which is too high a cost to avoid
faulty scripts.

To avoid malicious or broken script, we suggest implementing an additional security mechanism on top
of ``Ramses Logic`` which doesn't allow execution of scripts of unknown origin. Also, build your code with errors in mind
and force scripts into an automated testing process. We also advise to use hashsums and whitelisting techniques to only
execute scripts which are tested and verified to be benign.

.. TODO add more docs how environment work, what is the level of isolation between different scripts etc.

=========================
List of all examples
=========================

.. toctree::
    :maxdepth: 1
    :caption: Examples

    examples/00_minimal
    examples/01_properties_simple
    examples/02_properties_complex
    examples/03_errors_compile_time
    examples/04_errors_runtime
    examples/05_ramses_scene
    examples/06_serialization

=========================
Class Index
=========================

.. doxygenindex::
