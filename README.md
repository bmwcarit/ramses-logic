# Introduction


[![build status](https://github.com/COVESA/ramses-logic/workflows/CMake/badge.svg?branch=master)](https://github.com/COVESA/ramses-logic/actions?query=branch%3Amaster) [![docs status](https://readthedocs.org/projects/ramses-logic/badge/?style=flat)](https://ramses-logic.readthedocs.io/en/latest/)

`RAMSES logic` extends the [RAMSES rendering ecosystem](https://ramses-sdk.readthedocs.io/) with scripting support based on
[Lua](https://github.com/lua/lua). `RAMSES` is designed to be minimalistic and closely aligned to OpenGL, which can be a
limitation for more complex applications. `RAMSES logic` addresses this limitation by providing a runtime library which can
load and run `Lua` scripts and provides a standard set of tools to let these scripts interact between each other and control
a sophisticated `RAMSES` scene.

You can find the full documentation of `RAMSES logic` [here](https://ramses-logic.readthedocs.io/).

# Build

Clone RAMSES logic along with its dependencies:

```bash
$ git clone https://github.com/COVESA/ramses-logic <path>
$ cd <path>
$ git submodule update --init --recursive
```

You can find the compiled version of the examples in `<path>/build/bin`.

For more in-depth build instructions and customization options, have a look at
the [detailed build documentation](https://ramses-logic.readthedocs.io/en/latest/build.html).

# Examples

Prefer to learn by example? Have a look at our [self-contained example snippets](https://ramses-logic.readthedocs.io/en/latest/api.html#list-of-all-examples).

# Version matrix

|Logic    | Included Ramses version       | Minimum required Ramses version    | Binary file compatibility    |
|---------|-------------------------------|------------------------------------|------------------------------|
|0.13.0   | 27.0.114                      | 27.0.102                           | >= 0.13.0                    |
|0.12.0   | 27.0.113                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.11.0   | 27.0.113                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.10.2   | 27.0.112                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.10.1   | 27.0.111                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.10.0   | 27.0.111                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.9.1    | 27.0.111                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.9.0    | 27.0.110                      | 27.0.102                           | 0.9.0 - 0.12.0               |
|0.8.1    | 27.0.110                      | 27.0.102                           | 0.7.x or 0.8.x               |
|0.8.0    | 27.0.110                      | 27.0.102                           | 0.7.x or 0.8.x               |
|0.7.0    | 27.0.105                      | 27.0.102                           | 0.7.x                        |
|0.6.2    | 27.0.105                      | 27.0.102                           | 0.6.x                        |
|0.6.1    | 27.0.103 (includes 27.0.11)   | 27.0.102                           | 0.6.x                        |
|0.6.0    | 27.0.102 (includes 27.0.10)   | 27.0.100                           | 0.6.x                        |
|0.5.3    | 27.0.101                      | 27.0.100                           | 0.5.3                        |

# License

The Ramses Logic Engine is licensed under the Mozilla Public License 2.0 (MPL-2.0),
same as Ramses itself. Have a look at the Ramses README file for more information
regarding Ramses and its dependencies.

In addition to Ramses, the Ramses Logic Engine has following dependencies,
listed alongside their licenses here:

* Lua (MIT)
* Sol (MIT)
* Flatbuffers (Apache-2.0)
* Fmtlib (MIT)
* Googletest (BSD-3-Clause)
* Google Benchmark (Apache-2.0)

All of the above dependencies are referenced as Git submodules pointing to their original
repository. Hence, no modifications are made by Ramses Logic.

