#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# skip policies not known to earlier cmake versions
if(POLICY CMP0054)
   cmake_policy(SET CMP0054 NEW)
endif(POLICY CMP0054)

# #### externals that are build with normal compiler flags ####

################################################
################     Googletest      ###########
################################################

#project specific setup for gtest/gmock
set(gtest_force_shared_crt true CACHE BOOL "Must be enabled for linking our unit tests" FORCE)
if(ramses-logic_BUILD_TESTS AND NOT TARGET gmock)
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(INSTALL_GMOCK OFF CACHE INTERNAL "")

    add_subdirectory(googletest)
    target_include_directories(gmock INTERFACE "googletest/googletest/include")
    target_include_directories(gmock_main INTERFACE "googletest/googletest/include")
    target_compile_options(gtest PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-field-initializers>)
    target_compile_options(gmock PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-field-initializers>)

    FOLDERIZE_TARGETS(gmock gtest gmock_main gtest_main)
endif()

# Decouple internally used googletest from possible mismatch with externally provided one
add_library(rlogic-gmock INTERFACE)
add_library(rlogic-gmock-main INTERFACE)
target_link_libraries(rlogic-gmock INTERFACE gmock gtest)
add_library(rlogic::gmock ALIAS rlogic-gmock)
target_link_libraries(rlogic-gmock-main INTERFACE gmock_main gtest)
add_library(rlogic::gmock-main ALIAS rlogic-gmock-main)

# TODO Violin/Tobias see if we can switch to more modern "target_compiler_option" way
include(${PROJECT_SOURCE_DIR}/cmake/adaptCompilerFlagsForExternals.cmake)

# #### externals that are build with adapted compiler flags ####

################################################
################     Lua      ##################
################################################

# TODO Violin don't build Lua if target already present

# Collect all source and header files
file(GLOB LUA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lua/*.c)
file(GLOB LUA_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/lua/*.h)

# Remove luac from the list, because it contains a main function
list(REMOVE_ITEM LUA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lua/lua.c)

set_source_files_properties(${LUA_SOURCE} PROPERTIES LANGUAGE CXX)

# Create library for lua
add_library(lua STATIC ${LUA_SOURCE} ${LUA_HEADER})
add_library(lua::lua ALIAS lua)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    # We compile lua as c++ even though it has .c files. convince clang to just do it without complaining
    target_compile_options(lua PRIVATE -x c++)
endif()

target_include_directories(lua
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lua>
)

FOLDERIZE_TARGET(lua)

################################################
################     Sol      ##################
################################################

set(SOL2_LUA_VERSION "5.1.1" CACHE STRING "" FORCE)
set(INSTALL_SOL2 OFF CACHE INTERNAL "")
add_subdirectory(sol)

# Ensure sol is expecting c++ compiled lua
target_compile_definitions(sol2 INTERFACE SOL_USING_CXX_LUA=1 SOL_EXCEPTIONS_ALWAYS_UNSAFE=1)

################################################
################ Flatbuffers  ##################
################################################

# TODO don't build flatbuffers if target already present

# Don't build flatbuf targets we don't need
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_INSTALL OFF CACHE BOOL "" FORCE)
set(FLATBUFFERS_BUILD_FLATHASH OFF CACHE BOOL "" FORCE)

# Only add flatc target if flatbuffers header generation needed
if(ramses-logic_ENABLE_FLATBUFFERS_GENERATION)
    set(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Build flatbuffers compiler" FORCE)
else()
    set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "Build flatbuffers compiler" FORCE)
endif()

add_subdirectory(flatbuffers)
FOLDERIZE_TARGET(flatbuffers)

add_library(rlogic::flatbuffers ALIAS flatbuffers)

if(ramses-logic_ENABLE_FLATBUFFERS_GENERATION)
    FOLDERIZE_TARGET(flatc)
endif()

################################################
################     fmt      ##################
################################################

if (NOT TARGET fmt)
    add_subdirectory(fmt)
    FOLDERIZE_TARGET(fmt)
endif()

################################################
################     Ramses   ##################
################################################


if(ramses-logic_RAMSES_TARGET AND TARGET ${ramses-logic_RAMSES_TARGET})
    message(STATUS " + Using existing ramses target '${ramses-logic_RAMSES_TARGET}'")
    # TODO Violin add version check here once ramses provides CMake variables for it
else()
    if(ramses-logic_ALLOW_RAMSES_BUILD)
        message(STATUS " + Building ramses from source and using target '${ramses-logic_RAMSES_TARGET}'")

        set(ramses-sdk_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_IVI_TEST_APPS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_DEMO_ECM OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_SMOKE_TESTS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_FULL_SHARED_LIB OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_BUILD_CLIENT_ONLY_SHARED_LIB ON CACHE BOOL "" FORCE)
        set(ramses-sdk_ENABLE_DLT OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_FORCE_OFF_SOMEIP_HU ON CACHE BOOL "" FORCE)
        set(ramses-sdk_FORCE_OFF_SOMEIP_IC ON CACHE BOOL "" FORCE)

        # Disable some things which don't have a CMake option
        set(ramses-sdk_ramses-daemon          OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_ramses-renderer-lib    OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_renderer_RendererLib   OFF CACHE BOOL "" FORCE)

        add_subdirectory(ramses)

        set(ramses-logic_RAMSES_TARGET "ramses-shared-lib-client-only" CACHE STRING "" FORCE)

        # This is kind of an assert - if this fails, there is a version mismatch with ramses
        if(NOT TARGET ${ramses-logic_RAMSES_TARGET})
            message(FATAL_ERROR "Invalid Ramses target '${ramses-logic_RAMSES_TARGET}'")
        endif()
    else()
        message(FATAL_ERROR "Must either provide a valid ramses target (current value: '${ramses-logic_RAMSES_TARGET}') or set ramses-logic_ALLOW_RAMSES_BUILD=ON")
    endif()
endif()
