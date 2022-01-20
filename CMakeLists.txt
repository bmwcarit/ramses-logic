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
set(RLOGIC_VERSION_MINOR 14)
set(RLOGIC_VERSION_PATCH 1)

set(RLOGIC_VERSION ${RLOGIC_VERSION_MAJOR}.${RLOGIC_VERSION_MINOR}.${RLOGIC_VERSION_PATCH})
set(ramses-logic_VERSION "${RLOGIC_VERSION}" CACHE STRING "Ramses Logic version" FORCE)

project(RamsesLogic
    VERSION ${RLOGIC_VERSION}
    DESCRIPTION "A scripting and animation runtime for Ramses based on Lua"
    LANGUAGES CXX
    HOMEPAGE_URL  "https://github.com/COVESA/ramses-logic"
)

#==========================================================================
# Configuration options
#==========================================================================

set(ramses-logic_SHARED_LIB_TARGET_NAME "ramses-logic" CACHE STRING "Name of ramses-logic shared library target")
set(ramses-logic_RAMSES_TARGET "" CACHE
    STRING "Set this cache variable to a custom RAMSES target. If this is a valid target name, Ramses Logic will use it instead of building Ramses")
set(ramses-logic_FOLDER_PREFIX "" CACHE
    STRING "Set a custom prefix for target folders in Visual Studio. If not set, will be set based on project's relative path")
set(ramses-logic_BUILD_RAMSES_RENDERER true CACHE BOOL "Set whether the ramses renderer will be built as part of this build. If this option is set to ON, ramses-logic_RAMSES_TARGET has to be set to a shared library target of ramses which contains a renderer (e.g ramses-shared-lib-android-egl-es-3-0)")
option(ramses-logic_ALLOW_RAMSES_BUILD "Set this to OFF to explicitly disable Ramses build, even if ramses-logic_RAMSES_TARGET is not a valid ramses target" ON)
option(ramses-logic_FIND_RAMSES_FROM_SYSTEM "Set this to ON if you want to use an existing installation or package of Ramses (find_package)" OFF)
set(ramses-logic_PLATFORM "" CACHE
    STRING "Use this in conjunction with ramses-logic_FIND_RAMSES_FROM_SYSTEM or ramses-logic_BUILD_RAMSES_RENDERER to search for a specific Ramses platform")
set(ramses-logic_PACKAGE_TYPE "TGZ" CACHE STRING "Passed to CPack to determine the type of package to be created. Default is TGZ.")
option(ramses-logic_BUILD_SHARED_LIB "Enable/disable build of Ramses Logic shared lib target" ON)
option(ramses-logic_BUILD_STATIC_LIB "Enable/disable build of Ramses Logic static lib target" OFF)
option(ramses-logic_ENABLE_INSTALL "Enable/disable installation of Ramses Logic" ON)
option(ramses-logic_BUILD_WITH_LTO "Build all targets with link time optimization enabled (not supported on all platforms)" OFF)
option(ramses-logic_WARNINGS_AS_ERRORS "Treat warnings as errors when compiling" ON)
option(ramses-logic_FORCE_BUILD_TESTS "Force build tests, even if built as a submodule" OFF)
option(ramses-logic_FORCE_OFF_TESTS "Fully disable tests" OFF)
option(ramses-logic_BUILD_DOCUMENTATION "Enable building documentation when dependencies available" ON)
option(ramses-logic_FORCE_BUILD_DOCS "Abort build if documentation dependencies not found(doxygen, sphinx, breathe and the rtd theme for sphinx)" OFF)
option(ramses-logic_BUILD_EXAMPLES "Build examples" ON)
option(ramses-logic_BUILD_TOOLS "Build tools" ON)
option(ramses-logic_ENABLE_FLATBUFFERS_GENERATION "Create target to generate flatbuffers serialization files (ON/OFF)" ON)
option(ramses-logic_ENABLE_TEST_COVERAGE "Enable test coverage - works on clang only (ON/OFF)" OFF)
option(ramses-logic_ENABLE_CODE_STYLE "Enable code style checker target (requires python3.6+)" ON)
option(ramses-logic_USE_CCACHE "Enable ccache for build" OFF)

if(NOT ramses-logic_BUILD_STATIC_LIB AND NOT ramses-logic_BUILD_SHARED_LIB)
    message(FATAL_ERROR "One of the ramses-logic_BUILD_SHARED_LIB/ramses-logic_BUILD_STATIC_LIB options must be enabled!")
endif()

if(ramses-logic_FIND_RAMSES_FROM_SYSTEM AND ramses-logic_ALLOW_RAMSES_BUILD)
    message(FATAL_ERROR "Can't have both ramses-logic_FIND_RAMSES_FROM_SYSTEM and ramses-logic_ALLOW_RAMSES_BUILD options enabled!")
endif()

if(ramses-logic_FIND_RAMSES_FROM_SYSTEM AND ramses-logic_BUILD_RAMSES_RENDERER)
    message(FATAL_ERROR "Can't have both ramses-logic_FIND_RAMSES_FROM_SYSTEM and ramses-logic_BUILD_RAMSES_RENDERER options enabled!")
endif()

if(ramses-logic_BUILD_TOOLS AND NOT ramses-logic_BUILD_RAMSES_RENDERER)
    message(FATAL_ERROR "Can't build tools without also having a ramses renderer. Enable a renderer or disable the tools!")
endif()

if(ramses-logic_BUILD_RAMSES_RENDERER)
    # Set some automatic values if platform not set explicitly
    if(NOT ramses-logic_PLATFORM)
        if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
            set(ramses-logic_PLATFORM "WINDOWS")
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
            set(ramses-logic_PLATFORM "LINUX-X11")
        elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
            set(ramses-logic_PLATFORM "ANDROID")
        endif()
    endif()
endif()

#==========================================================================
# Global project setup
#==========================================================================

set(RLOGIC_INSTALL_HEADERS_PATH     include)
set(RLOGIC_INSTALL_RUNTIME_PATH     bin)
set(RLOGIC_INSTALL_LIBRARY_PATH     lib)
set(RLOGIC_INSTALL_ARCHIVE_PATH     lib)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
if(NOT ramses-logic_FORCE_OFF_TESTS AND (ramses-logic_FORCE_BUILD_TESTS OR CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME))
    set(ramses-logic_BUILD_TESTS ON)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

include(cmake/platformConfig.cmake)
include(cmake/externalTools.cmake)
include(cmake/folderize.cmake)

if(ramses-logic_BUILD_TESTS)
    include(cmake/testSetup.cmake)
endif()

#==========================================================================
# Create project targets
#==========================================================================

add_subdirectory(external)

if(ramses-logic_ENABLE_FLATBUFFERS_GENERATION)
    include(${PROJECT_SOURCE_DIR}/cmake/flatbuffersGeneration.cmake)
endif()

file(GLOB public_headers        include/ramses-logic/*.h)
file(GLOB impl_headers          lib/impl/*.h)
file(GLOB internal_headers      lib/internals/*.h)
file(GLOB flatbuf_gen_headers   lib/flatbuffers/generated/*.h)
file(GLOB flatbuf_schemas       lib/flatbuffers/schemas/*.fbs)

file(GLOB impl_src      lib/impl/*.cpp)
file(GLOB internals_src lib/internals/*.cpp)

add_library(ramses-logic-obj OBJECT)

target_sources(ramses-logic-obj
    PRIVATE
        ${public_headers}
        ${impl_headers}
        ${internal_headers}
        ${impl_src}
        ${internals_src}
        ${flatbuf_gen_headers}
        ${flatbuf_schemas}
)

# Not obvious, but this is the only robust way to add files to a VS project with CMake
# without having them treated as source files
set_source_files_properties(${flatbuf_gen_headers} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${flatbuf_schemas} PROPERTIES HEADER_FILE_ONLY TRUE)

source_group("Header Files" FILES ${public_headers})
source_group("Header Files\\internals\\impl" FILES ${impl_headers})
source_group("Header Files\\internals" FILES ${internal_headers})
source_group("Header Files\\generated" FILES ${flatbuf_gen_headers})
source_group("Source Files\\impl" FILES ${impl_src})
source_group("Source Files\\internals" FILES ${internals_src})
source_group("FlatbufSchemas" FILES ${flatbuf_schemas})

# TODO investigate option to use single-header version of sol2 and flatbuffers
target_link_libraries(ramses-logic-obj
    # We use public linking here, because ramses-logic-obj is an object library.
    # The hiding of this dependencies happens in the ramses-logic target (shared library)
    PUBLIC
        sol2::sol2
        lua::lua
        rlogic::flatbuffers
        fmt::fmt
        ${RAMSES_TARGET}
)

# Link required Android dependencies if found
find_package(AndroidSDK)
if(AndroidSDK_FOUND)
    target_link_libraries(ramses-logic-obj PUBLIC ${AndroidSDK_LIBRARIES})
endif()

# Add dependency to flatbuffer generation target, unless the target is disabled
if(TARGET FlatbufGen)
    add_dependencies(ramses-logic-obj FlatbufGen)
endif()

target_include_directories(ramses-logic-obj
    PRIVATE
        include
        lib
        lib/flatbuffers
        ${PROJECT_BINARY_DIR}/BuildConfig
)

folderize_target(ramses-logic-obj "ramses-logic")

if (NOT ramses-logic_DISABLE_SYMBOL_VISIBILITY)
    target_compile_definitions(ramses-logic-obj PRIVATE RLOGIC_LINK_SHARED_EXPORT=1)
endif()

include(cmake/addRamsesLogicTarget.cmake)

# Build static lib when top-level project or if explicitly requested by user, or if unit tests are enabled
if(ramses-logic_BUILD_STATIC_LIB OR ramses-logic_BUILD_TESTS OR CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    ################## ramses-logic-static ##################
    add_ramses_logic_target("ramses-logic-static" "STATIC")
endif()

################## ramses-logic (shared library) ##################
if(ramses-logic_BUILD_SHARED_LIB)
    add_ramses_logic_target(${ramses-logic_SHARED_LIB_TARGET_NAME} "SHARED")

    if(ramses-logic_ENABLE_INSTALL)
        install(
            TARGETS ${ramses-logic_SHARED_LIB_TARGET_NAME}
            EXPORT ramses-logic-targets
            RUNTIME DESTINATION         ${RLOGIC_INSTALL_RUNTIME_PATH}
            LIBRARY DESTINATION         ${RLOGIC_INSTALL_LIBRARY_PATH}
            ARCHIVE DESTINATION         ${RLOGIC_INSTALL_ARCHIVE_PATH}
            PUBLIC_HEADER DESTINATION   ${RLOGIC_INSTALL_HEADERS_PATH}/ramses-logic
        )

        if (MSVC)
            install(
                FILES $<TARGET_PDB_FILE:ramses-logic>
                DESTINATION ${RLOGIC_INSTALL_RUNTIME_PATH} CONFIGURATIONS Debug RelWithDebInfo)
        endif()
    endif()
endif()

#==========================================================================
# Optional and complementary targets
#==========================================================================

if(ramses-logic_BUILD_TESTS)
    add_subdirectory(unittests)
    add_subdirectory(benchmarks)
endif()


if(ramses-logic_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(ramses-logic_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

include(cmake/addCheckerTargets.cmake)

#==========================================================================
# build and install documentation
#==========================================================================

if (ramses-logic_BUILD_DOCUMENTATION)
    add_subdirectory(doc)

    if(ramses-logic_ENABLE_INSTALL)
        install(FILES       README.md CHANGELOG.md LICENSE.txt
                DESTINATION share/doc/${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
                COMPONENT   ${PROJECT_NAME}-${PROJECT_VERSION}
            )
    endif()
endif()

#==========================================================================
# Installation and packaging
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

    # generate CMake config file from template
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

set(CPACK_GENERATOR ${ramses-logic_PACKAGE_TYPE})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_VENDOR "ramses")
set(CPACK_PACKAGE_CONTACT "ramses-oss@list.bmw.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ramses scripting and animation library")
set(CPACK_PACKAGE_DESCRIPTION "A packaged version of the logic and animation library for ramses. Generated using CPack.")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_SOURCE_STRIP_FILES    TRUE)
set(CPACK_STRIP_FILES           FALSE)
set(CPACK_PACKAGE_ICON          "${CMAKE_CURRENT_SOURCE_DIR}/doc/sphinx/_static/logo.png")

include(CPack)
