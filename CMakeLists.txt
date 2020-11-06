#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.13)

#==========================================================================
# General project metadata
#==========================================================================

set(RLOGIC_VERSION_MAJOR 0)
set(RLOGIC_VERSION_MINOR 3)
set(RLOGIC_VERSION_PATCH 1)

set(RLOGIC_VERSION ${RLOGIC_VERSION_MAJOR}.${RLOGIC_VERSION_MINOR}.${RLOGIC_VERSION_PATCH})
set(ramses-logic_VERSION "${RLOGIC_VERSION}" CACHE STRING "Ramses Logic version" FORCE)

project(RamsesLogic
    VERSION ${RLOGIC_VERSION}
    DESCRIPTION "A scripting and animation runtime for Ramses based on Lua"
    LANGUAGES CXX
    HOMEPAGE_URL  "https://github.com/GENIVI/ramses-logic"
)

# Configurable options
set(ramses-logic_RAMSES_TARGET "ramses-shared-lib-client-only" CACHE
    STRING "Set this cache variable to a custom RAMSES target. If this is a valid target name, Ramses Logic will use it instead of building Ramses")
option(ramses-logic_ALLOW_RAMSES_BUILD "Set this to OFF to explicitly disable Ramses build, even if ramses-logic_RAMSES_TARGET is not a valid ramses target" ON)
option(ramses-logic_BUILD_SHARED_LIB "Enable/disable build of Ramses Logic shared lib target" ON)
option(ramses-logic_BUILD_STATIC_LIB "Enable/disable build of Ramses Logic static lib target" OFF)
option(ramses-logic_ENABLE_INSTALL "Enable/disable installation of Ramses Logic" ON)
option(ramses-logic_BUILD_WITH_LTO "Build all targets with link time optimization enabled (not supported on all platforms)" OFF)
option(ramses-logic_WARNINGS_AS_ERRORS "Treat warnings as errors when compiling" ON)
option(ramses-logic_FORCE_BUILD_TESTS "Force build tests, even if built as a submodule" OFF)
option(ramses-logic_BUILD_EXAMPLES "Build examples" ON)
option(ramses-logic_BUILD_DOCS "Build documentation (ON/OFF). Requires doxygen, sphinx, breathe and the rtd theme for sphinx" OFF)
option(ramses-logic_ENABLE_FLATBUFFERS_GENERATION "Create target to generate flatbuffers serialization files (ON/OFF)" ON)
option(ramses-logic_ENABLE_TEST_COVERAGE "Enable test coverage - works on clang only (ON/OFF)" OFF)
option(ramses-logic_ENABLE_CODE_STYLE "Enable code style checker target (requires python3.6+)" ON)
option(ramses-logic_USE_CCACHE "Enable ccache for build" OFF)

if(NOT ramses-logic_BUILD_STATIC_LIB AND NOT ramses-logic_BUILD_SHARED_LIB)
    message(FATAL_ERROR "One of the ramses-logic_BUILD_SHARED_LIB/ramses-logic_BUILD_STATIC_LIB options must be enabled!")
endif()

# Standard installation paths
# TODO Violin/Sven/Tobias consider making configurable from outside?
set(RLOGIC_INSTALL_HEADERS_PATH     include)
set(RLOGIC_INSTALL_RUNTIME_PATH     bin)
set(RLOGIC_INSTALL_LIBRARY_PATH     lib)
set(RLOGIC_INSTALL_ARCHIVE_PATH     lib)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

# TODO violin/tobias discuss which parts we want to reuse from ramses maybe
# Do we want to use add_compile_options instead?
include(cmake/platformConfig.cmake)
include(cmake/externalTools.cmake)
include(cmake/folderize.cmake)

# By default redirect binaries and shared libs to /bin for easier development
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    message(STATUS "Redirect ramses logic library output to ${CMAKE_CURRENT_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    message(STATUS "Redirect ramses logic binary output to ${CMAKE_CURRENT_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
endif()
set(CMAKE_DEBUG_POSTFIX "") # no debug suffix in this project

# Looks weird, but this is how CTest recommends enabling testing in libraries
if(ramses-logic_FORCE_BUILD_TESTS OR CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(ramses-logic_BUILD_TESTS ON)
endif()

if(ramses-logic_BUILD_TESTS)
    include(cmake/testSetup.cmake)
endif()

add_subdirectory(external)
add_subdirectory(src)

if(ramses-logic_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

#==========================================================================
# target: code style checker
#==========================================================================

include(cmake/addCheckerTargets.cmake)

#==========================================================================
# build and install documentation
#==========================================================================

if(ramses-logic_BUILD_DOCS)
    add_subdirectory(doc)
endif()

if(ramses-logic_ENABLE_INSTALL)
    install(FILES       README.rst CHANGELOG.rst LICENSE.txt
            DESTINATION share/doc/${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
            COMPONENT   ${PROJECT_NAME}-${PROJECT_VERSION}
        )
endif()

#==========================================================================
# generate CMake config files
#==========================================================================

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/build-config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/BuildConfig/ramses-logic-build-config.h"
    @ONLY
)

if(ramses-logic_ENABLE_INSTALL)
    include(CMakePackageConfigHelpers)

    # install paths for find/config script must differ on windows and linux because of different search
    # rules used by find_package. See https://cmake.org/cmake/help/git-master/command/find_package.html
    # for details
    set(ramses-logic-VERSION_DIR "ramses-logic-${RLOGIC_VERSION_MAJOR}.${RLOGIC_VERSION_MINOR}")
    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(ramses-logic_FIND_SCRIPT_INSTALL_DIR "lib/${ramses-logic-VERSION_DIR}/cmake")
    else()
        set(ramses-logic_FIND_SCRIPT_INSTALL_DIR "lib/cmake/${ramses-logic-VERSION_DIR}")
    endif()

    # generate CMake config file (use ramses-logicTemplate.cmake.in as base)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/ramses-logic-Template.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/ramses-logicConfig.cmake"
        INSTALL_DESTINATION "${ramses-logic_FIND_SCRIPT_INSTALL_DIR}"
        PATH_VARS RLOGIC_INSTALL_HEADERS_PATH RLOGIC_INSTALL_LIBRARY_PATH RLOGIC_INSTALL_RUNTIME_PATH RLOGIC_INSTALL_ARCHIVE_PATH
        NO_SET_AND_CHECK_MACRO
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )
    install(
        FILES       ${CMAKE_CURRENT_BINARY_DIR}/ramses-logicConfig.cmake
        DESTINATION ${ramses-logic_FIND_SCRIPT_INSTALL_DIR}
    )

    # generate CMake version file
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/ramses-logicConfigVersion.cmake"
        VERSION ${RLOGIC_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(
        FILES       ${CMAKE_CURRENT_BINARY_DIR}/ramses-logicConfigVersion.cmake
        DESTINATION ${ramses-logic_FIND_SCRIPT_INSTALL_DIR}
    )
endif()


#==========================================================================
# CPack package definition
#==========================================================================

# TODO Violin/Tobias discuss packaging and install + ramses dependency
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_VENDOR "ramses")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ramses scripting engine")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.rst")

set(CPACK_SOURCE_STRIP_FILES             TRUE)
set(CPACK_STRIP_FILES                    FALSE)

include(CPack)
