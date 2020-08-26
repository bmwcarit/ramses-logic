.. default-domain:: cpp
.. highlight:: cpp

=========================
Overview
=========================

The entry point to ``RAMSES logic`` is a factory-style class :class:`rlogic::LogicEngine` which can
create instances of all other types of objects supported by ``RAMSES Logic``:

* :class:`rlogic::LuaScript`
* :class:`rlogic::RamsesNodeBinding`

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

==================================================
Linking script outputs to ``ramses`` scenes
==================================================

You can link outputs of a :class:`rlogic::LuaScript` to ``Ramses`` scene data by created so-called bindings (e.g.
:class:`rlogic::RamsesNodeBinding`). One might wonder, why not allow to directly link script outputs to ``Ramses`` objects?
The reason for that is two-fold:

* Separation of concerns between pure script logic and ``Ramses``-related scene updates
* This allows to handle all inputs and outputs in a generic way using the :class:`rlogic::LogicNode` class' interface from
  which both :class:`rlogic::LuaScript` and :class:`rlogic::RamsesNodeBinding` derive

.. TODO: Violin/Sven add code example once we can link data between scripts

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
Because ramses-logic can be used in different environemnts which not always have a console
to print messages, the "print" function is overloaded. The default behavior is that your
message will be piped to std::cout together with the name of the calling script.
If you need more control of the print messages, you can overload the printing function with
you own one like this:

.. code-block::
    :linenos:

    ramses-logic::LogicEngine logicEngine;
    ramses-logic::LuaScript script = logicEngine.createLuaScriptFromSource(R"(
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

=====================================
Serialization/Deserialization
=====================================

As you've already learned, it is possible to create all parts of your logic programatically. To unleash the full
potential of the logic engine you can also serialize a complete state and load it for the next lifecicle.

.. code-block::
    :linenos:

    ramses-logic::LogicEngine logicEngine;
    logicEngine.saveToFile("logicEngine.bin");
    ...
    logicEngine.loadFromFile("logicEngine.bin");

During serialization all LuaScripts and RamsesNodeBindings will be serialized for the next lifecicle.

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
Note that having a custom logger does not disable the default logging - you have to do this explicitly
if you don't want to see the ramses logic default logs.

.. code-block::
    :linenos:

    #include <iostream>

    Logger::SetDefaultLogging([}(ELogMessageType type, std::string_view message){
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
Class Index
=========================

.. doxygenindex::
