========================
Introduction
========================


``RAMSES logic`` extends the `RAMSES rendering ecosystem <https://github.com/GENIVI/ramses>`_ with scripting support based on
`Lua <https://github.com/lua/lua>`_. ``RAMSES`` is designed to be minimalistic and closely aligned to OpenGL, which can be a
limitation for more complex applications. ``RAMSES logic`` addresses this limitation by providing a runtime library which can
load and run ``Lua`` scripts and provides a standard set of tools to let these scripts interact between each other and control
a sophisticated ``RAMSES`` scene.

You can find the full documentation of ``RAMSES logic`` `here <https://genivi.github.io/ramses-logic>`_.

.. _quickstart:

========================
Build
========================

Clone RAMSES logic along with its dependencies:

.. code-block:: bash

    git clone https://github.com/GENIVI/ramses-logic <path>
    cd <path>
    git submodule update --init --recursive

Configure and build with CMake (CMake 3.13 or newer required):

.. code-block:: bash

    mkdir build && cd build
    cmake -G"Visual Studio 16 2019" ../   # Or any other generator!
    cmake --build .

You can find the compiler version of the examples in ``<path>/build/bin``.

For more in-depth build instructions and customization options, have a look at
the `detailed build documentation <https://genivi.github.io/ramses-logic/build.html>`_.

========================
Examples
========================

Prefer to learn by example? Have a look at our `self-contained example snippets <https://genivi.github.io/ramses-logic/api.html#list-of-all-examples>`_.
