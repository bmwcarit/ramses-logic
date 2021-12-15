..
    -------------------------------------------------------------------------
    Copyright (C) 2020 BMW AG
    -------------------------------------------------------------------------
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at https://mozilla.org/MPL/2.0/.
    -------------------------------------------------------------------------

.. default-domain:: lua
.. highlight:: lua

=========================
Quick start
=========================

The ``Logic Engine`` uses an extended Lua syntax to declare what should happen with the attached Ramses scene when application
inputs/signals/configuration changes. If you prefer learning by example and/or have previous experience with scripting languages,
you could have a look at the `Ramses Composer examples <https://github.com/COVESA/ramses-composer-docs>`_ and come back to these docs for details.
If not, we suggest reading through the docs and trying out the presented concepts on your own in a freshly created `Ramses Composer <https://github.com/COVESA/ramses-composer>`_ project.
Just create a new LuaScript object (right-clicking the resource menu), set its URI to a file on your local machine, and start jamming!

.. todo: (Violin) maybe create a dedicated "test" binary, a-la "Lua shell" which can read a lua file and execute/print result? Or maybe have a special mode in the IMgui tool?

Alternatively, if you have a C++ compiler/IDE, you can use some of the :ref:`examples <List of all examples>`, e.g. :ref:`the structs example <Example with structured properties>`
as a sandbox to compile and try out the code presented here.


=========================
Basics of Lua
=========================

Lua is a scripting language designed to be efficient, easily extensible, and highly portable. These properties are also the reason
we chose it over Python, C# and other popular languages for gaming/3D development.

.. note::

    The Lua community has two camps - those who use the latest version
    (5.4) and those who use 5.1 (there have been major changes since 5.1 which split the community to an extent). We use a subset of the 5.1 version of Lua, although
    there are no major differences from a syntax point of view.

You can have a look at the official docs of Lua `here <https://www.lua.org/manual/5.1/>`_ for the full language specification. A basic script in Lua looks
like this:

.. code-block:: lua

    -- Comments start with `--`
    name = "Jack"
    age = "forty two"
    -- Lua is dynamically typed!
    age = 42

    -- equivalent to "add = function(a, b)..."
    function add(a, b)
        -- adding 'local' here makes the variable only visible in the function
        local result = a + b
        return result
    end

    -- Complex data is stored in `tables`, i.e. unsorted key-value pairs with no specific order
    user_data = {
        user_name = name,
        user_age = add(age, 2),
        add_function = add
    }

    -- Can also use user_data["user_name"]
    print(user_data.user_name)

The ``Logic Engine`` provides full support for the Lua 5.1 language, with some restrictions and some enhancements.
The restrictions are necessary to ensure the safety of applications running the scripts, while the enhancements are
needed to enable a seamless integration with the outside world (other scripts, Ramses, the Ramses Composer GUI). Let's get a closer look!

==============================================
Declaring an interface() and a run() function
==============================================

All scripts in the ``Logic Engine`` are required to have two special functions - ``interface()`` and ``run()``. These functions
should not accept any parameters and should not return values. The ``interface()`` function is used to declare what inputs a script accepts and
what outputs it may produce. The ``run()`` function, which is executed whenever any of the inputs is changed, defines
how the outputs' data shall be computed based on the current values of the inputs.

The ``interface()``
function is the place where script inputs and outputs are declared using the special global objects
``IN`` and ``OUT``. The ``run()`` function can access the values of inputs and may set
the values of outputs using the ``IN`` and ``OUT`` globals respectively.

The ``interface()`` function can declare inputs and outputs by adding properties to ``IN`` and ``OUT`` as
if they were standard ``Lua`` tables, with two restrictions:

* the key has to be a string (and not a number or anything else) so that it can be shown as a human-readable property in the Ramses Composer
* the value has to be one of the types supported by the ``Logic Engine`` (INT, FLOAT, etc. - full list further down) or a table which obeys the same rules

Here is an example:

.. code-block:: lua

    function interface()
        IN.name = STRING
        IN.hungry = BOOL

        OUT.coala = {
            name = STRING,
            coalafications = {
                bear = BOOL,
                bamboo_eaten = INT
            }
        }
    end

Following types are supported for individual properties:

* INT
* FLOAT
* BOOL
* STRING
* VEC2F, VEC3F, VEC4F (fixed vector of Floats)
* VEC2I, VEC3I, VEC4I (fixed vector of Ints)

Additionally, a property can be a `struct` or an `array`. In the example above, ``coala`` is a struct, and ``coalafications`` is also a struct
nested under ``coala``. Here is another example, this time with arrays:

.. code-block:: lua

    function interface()
        IN.bamboo_coordinates = ARRAY(10, VEC3F)
        IN.bamboo_sizes = ARRAY(10, FLOAT)

        OUT.located_bamboo = ARRAY(10, {
            position = VEC3F,
            not_eaten_yet = BOOL
        })
    end

In the example above, ``IN.bamboo_coordinates`` is an input array of 10 elements, each of type ``VEC3F``. ``OUT.located_bamboo`` is an output
array with a `struct` type - each of the 10 elements has a ``position`` and a ``not_eaten_yet`` property.

Even though the ``IN`` and ``OUT`` objects are accessible in both ``interface()`` and ``run()`` functions,
they have different semantics in each function. The ``interface`` function only **declares** the interface
of the script, thus properties declared there can **only have a type**, they don't have a **value** yet -
similar to function signatures in programming languages.

In contrast to the ``interface()`` function, the ``run()`` function can't declare new properties any more,
but the properties have a value which can be read and written. Like in this example

.. code-block:: lua

    function interface()
        IN.name = STRING
        IN.hungry = BOOL

        OUT.coala = {
            name = STRING,
            coalafications = {
                bear = BOOL,
                bamboo_eaten = INT
            }
        }
    end

    function run()
        local coala_name = IN.name .. " the Coala"
        local bamboos_fed = 3

        if IN.hungry then
            bamboos_fed = 5
        end

        OUT.coala = {
            name = coala_name,
            coalafications = {
                bear = true,
                bamboo_eaten = bamboos_fed
            }
        }
    end

Here, ``run()`` will compute a few values and store the result in the output ``coala``. Note that the structure of the ``coala`` output table is exactly the
same as declared in the ``interface()`` function. In this example we assign all properties of ``coala``, but you can only set specific ones.

Furthermore, trying to declare new properties in ``run()``
will result in errors, same goes when trying to get/set the values of properties from within ``interface()``.

The ``interface()`` function is only ever executed once - during the creation of the script. The ``run()``
function is executed every time one or more of the values in ``IN`` changes, either when changed explicitly (in the Composer GUI or in code),
or when any of the inputs is linked to another script's output whose value changed.

The examples above demonstrate how structs can be nested in other structs or in arrays. The ``Logic Engine`` supports arbitrary nesting for structs. Arrays can
have a primitive type (e.g. ``INT``) or a complex type (a struct) which can have arbitrary properties, also nested ones. It is not possible to have arrays of arrays
(multidimensional arrays). Also, array size is limited to 255 elements currently.

==============================================
Global variables and the init() function
==============================================

Global symbols (symbols declared outside of the scope of functions) are **not** visible in the ``interface()`` or the ``run()`` functions
(see :ref:`Environments and isolation`).
This restriction makes sure that scripts are stateless and not execution-dependent and that they behave the same after loading from a file as when they
were created.

In order to declare global variables, use the ``init()`` function in conjunction with the ``GLOBAL`` special table for holding global symbols.
Here is an example:

.. code-block:: lua

    function init()
        GLOBAL.coala = {
            name = "Mr. Worldwide",
            age = 14
        }
    end

    function interface()
    end

    function run()
        print(GLOBAL.coala.name .. " is " .. tostring(GLOBAL.coala.age))
    end

The ``init()`` function is executed exactly once right after the script is created, and once when it is loaded from binary
data (:cpp:func:`rlogic::LogicEngine::loadFromFile`, :cpp:func:`rlogic::LogicEngine::loadFromBuffer`). The contents of the ``GLOBAL``
table can be modified the same way as normal global Lua variables, and can also be functions. It also allows declaring types which
can be then used in the ``interface()`` function. The ``init()`` function is optional, contrary to the other
two functions - ``interface()`` and ``run()``.

You can also use modules in ``init()``, see the :ref:`modules section <Using Lua modules>`.

==============================================
Custom functions
==============================================

The ``Logic Engine`` provides additional methods to work with extended types and modules, which are otherwise not possible with
standard Lua. Here is a list of these methods:

* `modules` function: declares dependencies to modules, can be called in Lua scripts and in modules themselves.
  Accepts a variable set of arguments, which have to be all of type string
* `rl_len` implements the `#` semantics, but also works on custom types (``IN``, ``OUT`` and their child types) and modules
    * use to obtain the size of IN, OUT or their sub-types (structs, arrays etc.) or data tables coming from modules
* `rl_next` custom stateless iterator similar to ``Lua`` built-in `next`
    * provides a way to iterate over custom types (``IN``, ``OUT``, etc.) and Logic engine custom modules' data
    * semantically behaves exactly like next()
* `rl_pairs` iterates over custom types, similar to ``Lua`` built-in `pairs`
    * uses `rl_next` internally to loop over built-ins, see above
    * semantically behaves like pairs(), yields integers [1, N] for array keys and strings for struct keys
* `rl_ipairs` behaves exactly the same as rl_pairs when used on arrays
    * it's there for better readibility and compatibility to plain Lua
    * rl_ipairs(array) yields the same result as rl_pairs(array)

All of the ``rl_*`` functions also work on plain Lua tables. However, we suggest to use the built-in Lua versions
for better performance if you know that the underlying type is a plain Lua table and not a usertype (IN, OUT, a Logic Engine module, etc.).
An exception to this is the length (``#``) operator for module data - you have to use rl_len instead as modules are write-protected and
the ``#`` operator in Lua 5.1 does not support write-protected tables.

.. warning::
    The iterator functions work in the ``interface()`` phase as well. However, properties there are mutable (you can add a new
    property to any container). Changing containers while iterating over them can result in undefined behavior and crashes, similar
    to other iterator implementations in C++!

==============================================
Environments and isolation
==============================================

``Lua`` is a powerful scripting language which can do practically anything. This can be a problem sometimes - especially
if the scripts are running in a restricted environment with strict safety and security concerns. In order to reduce the
risk of security attacks and stability problems, the ``Logic Engine`` isolates scripts in their own `environment <https://www.lua.org/pil/14.3.html>`_ and limits
the access of data and code. This ensures that a script can not be influenced by other scripts, modules, or dynamically loaded
content, unless explicitly desired by the content creator.

The following set of rules describes which part of the ``Lua`` script is assigned to which environment:

* Each script has its own ``runtime`` environment - applied to the ``run()`` function
* The ``init()`` function is also executed in the runtime environment
* The ``interface()`` function is executed in a temporary environment which is destroyed afterwards (alongside all its data!)
* The ``interface()`` function has access to modules and the ``GLOBAL`` table, but nothing else

==================================================
Indexing inside Lua
==================================================

``Lua`` has traditionally used array indexing starting at 1, in contrast to other popular script or
programming languages. Thus, the syntax and type checks of the ``Ramses Logic`` runtime honours
standard indexing used in Lua (starting by 1). This allows for example to use ``Lua`` tables as initializer
lists for arrays, without having to provide indices. Take a look at the following code sample:

.. code-block:: lua
    :linenos:
    :emphasize-lines: 7,9-12,14-17

    function interface()
        OUT.array = ARRAY(2, INT)
    end

    function run()
        -- This will work
        OUT.array = {11, 12}
        -- This will also work and produce the same result
        OUT.array = {
            [1] = 11,
            [2] = 12
        }
        -- This will not work and will result in error
        OUT.array = {
            [0] = 11,
            [1] = 12
        }
    end

The first two snippets are equivalent and will work. The first syntax (line 7) is obviously most convenient - just
provide all array elements in the Lua table. Note that **Lua will implicitly index elements starting from 1 with this syntax**.
The second syntax (line 9-12) is equivalent to the first one, but explicitly sets table indices. The third syntax (line 14-17)
is the one which feels intuitive for ``C/C++`` developers, but will result in errors inside Lua scripts.

.. note::

    Generic Lua scripts will allows any kind of index - even negative ones. In ``Ramses Logic``, we require arrays which are
    declared over ``IN`` and ``OUT`` to be strictly indexed from 1 to N without 'holes' to prevent inconsistencies and ensure a
    strict and safe data transfer between the scripts and the native runtime.

In order to achieve memory efficiency, but also to be consistent with ``C/C++`` rules, the ``C++`` API of ``Ramses Logic``
provides index access starting from 0 on the code side. The index mapping is taken over by
the ``Ramses Logic`` library.

=========================
Errors in scripts
=========================

General ``Lua`` syntax errors, but also violations of the ``Logic Engine`` rules (e.g. forgetting to declare an ``interface()`` function)
will be caught and reported. Scripts which contain errors will stop their execution at the line of code where the error occured.
Other scripts which may be linked to the erroneous script will not be executed to prevent faulty results.

==================================================
Using Lua modules
==================================================

--------------------------------------------------
Standard modules
--------------------------------------------------

The ``Logic Engine`` restricts which Lua modules can be used to a subset of the standard modules
of ``Lua 5.1``:

* Base library
* String
* Table
* Math
* Debug

For more information on the standard modules, refer to the official
`Lua documentation <https://www.lua.org/manual/5.1/manual.html#5>`_ of the standard modules.

Some of the standard modules are deliberately not supported:

* Security/safety concerns (loading files, getting OS/environment info)
* Not supported on all platforms (e.g. Android forbids direct file access)
* Stability/integration concerns (e.g. opening relative files in Lua makes the scripts non-relocatable)

--------------------------------------------------
Custom modules
--------------------------------------------------

It is possible to create custom user modules (see :cpp:class:`rlogic::LuaModule` for the ``C++`` docs).
A custom module can contain any Lua source code which obeys the modern Lua module definition convention
(i.e. declare a table, fill it with data and functions, and return the table as a result of the module
script):

.. code-block:: lua
    :linenos:
    :emphasize-lines: 1,5, 10,14

    local coalaModule = {}

    coalaModule.coalaChief = "Alfred"

    coalaModule.coalaStruct = {
        preferredFood = STRING,
        weight = INT
    }

    function coalaModule.bark()
        print("Coalas don't bark...")
    end

    return coalaModule

The name of the module (line 1) is not of importance and won't be visible anywhere outside of
the module definition file. You can declare structs and other types you could otherwise use
in the interface() functions of scripts (line 5). You can declare functions and make them part
of the module by using the syntax on line 10. Make sure you return the module (line 14)!

You can use modules in scripts as you would use a standard Lua module. The only exception
is that you can't import the module with the ``require`` keyword, but have to use a free
function ``modules()`` to declare the modules needed by the script:

.. code-block:: lua
    :linenos:
    :emphasize-lines: 1,4,20,21

    modules("coalas")

    function interface()
        local s = coalas.coalaStruct
        OUT.coalas = ARRAY(2, s)
    end

    function run()
        OUT.coalas = {
            {
                preferredFood = "bamboo",
                weight = 5
            },
            {
                preferredFood = "donuts",
                weight = 12
            }
        }

        print(coalas.chief .. " says:")
        coalas.bark()
    end

The name ``coalas`` on line 1 is the name under which the module is mapped and available in the
script (e.g. on lines 4, 20-21). The name obeys the same rules as Lua labels - it can only contain digits, letters and the
underscore character, and it can't start with a digit. Also, the names used in the mapping must
be unique (otherwise the script won't be able to uniquely resolve which modules are supposed to
be used).

It is also possible to use modules in other modules, like this:

.. code-block:: lua
    :linenos:

    modules("quaternions")

    local rotationHelper = {}

    function rotationHelper.matrixFromEuler(x, y, z)
        local q = quaternions.createFromEuler(x, y, z)
        return q.toMatrix()
    end

    return rotationHelper

In the example above, the ``rotationHelper`` module uses another module ``quaternions`` to provide
a new function which computes a rotation matrix using quaternions as an intermediate step.

.. note::
    Modules are read-only to prevent misuse and guarantee safe usage.

=====================================
Additional Lua syntax specifics
=====================================

``RAMSES Logic`` fuses ``C++`` and ``Lua`` code which are quite different, both in terms of language semantics,
type system and memory management. This is mostly transparent to the user, but there are some noteworthy
special cases worth explaining.

-----------------------------------------------------
Userdata vs. table
-----------------------------------------------------

The properties declared in the IN and OUT objects are stored as so-called `usertype` Lua objects, not standard tables.
`Userdata` are C++ objects which are exposed to the Lua script. This limits the operations possible with
those types - only the `index`, `newIndex` and for some containers the size (`#` operator) are supported.
Using other Lua operations (e.g. pairs/ipairs) will result in errors.

-----------------------------------------------------
Vec2/3/4 types
-----------------------------------------------------

While the property types which reflect Lua built-in types (BOOL, INT, FLOAT, STRING) inherit the standard
Lua value semantics, the more complex types (VEC2/3/4/I/F) have no representation in Lua, and are wrapped as
``Lua`` tables. They have the additional constraint that all values must be set simultaneously. It's not possible
for example to set just one component of a VEC3F - all three must be set at once. The reason for this design decision
is to ensure consistent behavior when propagating these values - for example when setting ``Ramses`` node properties
or uniforms.

-----------------------------------------------------
Numerics
-----------------------------------------------------

Lua's internal `number` type is represented by a IEEE-754 double precision float internally.
This is very flexible for scripting, but numerically dangerous when converting to other number
types. Examples for such types are floats (commonly used for uniforms), and unsigned integers
(when indexing arrays). To avoid numeric issues, the ``Logic Engine`` treats all value overflows and
automatic roundings as hard errors when implicitly rounding to ints or casting large doubles.

To avoid such numeric runtime errors, make sure you are not using
numbers larger than what a signed int32 type permits when indexing, and not rounding using
floating point arithmetics when computing indices. One way to denote an `invalid index` is using a
negative number and explicitly checking the sign of indices. Floats can be assigned
to integers by using the `math.floor/ceil` Lua functions explicitly.

Numeric rules apply for all number types, independent if they are part of a struct, array or component in VECx.

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
  will set the ``OUT`` label to refer to the ``IN`` object, essentially yielding two *references* to the same object - the ``IN`` object.
  If you want to be able to assign all input values to all output values, put them in a struct and assign the struct, e.g.:

.. code:: lua

    function interface()
        IN.struct = {}
        OUT.struct = {} -- must have the exact same properties as IN.struct! In this example - zero properties
    end

    function run()
        -- This will work!
        OUT.struct = IN.struct
        -- This will not work!
        OUT = IN
    end

* Do anything with ``IN`` and ``OUT`` in the global script scope - these objects don't exist there. However, you
  can pass ``IN`` and ``OUT`` as arguments to other functions, but consider :ref:`Special case: using OUT object in other functions`
* Calling the ``interface()`` function from within the ``run()`` function or vice-versa
* Using recursion in the ``interface()`` or ``run()`` functions
* Overwriting the ``IN`` and ``OUT`` objects. Exception to this is assigning ``OUT = IN`` in the ``run()`` function
* Using threads or coroutines. We might add this in future, but for now - don't use them

-----------------------------------------------------
Things you should avoid if possible
-----------------------------------------------------

Even though it is not strictly prohibited, it is not advised to store and read global variables
inside the ``run()`` function, as this introduces a side effect and makes the script more vulnerable
to errors. Instead, design the script so that it needs only be executed if the values of any of the
inputs changed - similar to how functional programming works.

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
