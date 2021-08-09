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

function(ensure_submodule_exists SUBMOD_NAME)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBMOD_NAME}")
        message(FATAL_ERROR "Missing submodule /external/${SUBMOD_NAME}. Please fetch Git submodules! (see build documentation)")
    endif()
endfunction()


# #### externals that are build with normal compiler flags ####

################################################
################     Googletest      ###########
################################################

#project specific setup for gtest/gmock
set(gtest_force_shared_crt true CACHE BOOL "Must be enabled for linking our unit tests" FORCE)
if(ramses-logic_BUILD_TESTS AND NOT TARGET gmock)
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    set(INSTALL_GMOCK OFF CACHE INTERNAL "")

    ensure_submodule_exists(googletest)

    add_subdirectory(googletest)
    target_include_directories(gmock INTERFACE "googletest/googletest/include")
    target_include_directories(gmock_main INTERFACE "googletest/googletest/include")
    target_compile_options(gtest PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-field-initializers>)
    target_compile_options(gmock PRIVATE $<$<CXX_COMPILER_ID:Clang>:-Wno-missing-field-initializers>)

    folderize_target(gmock "external")
    folderize_target(gtest "external")
    folderize_target(gmock_main "external")
    folderize_target(gtest_main "external")
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
################   Google Benchmark  ###########
################################################

#project specific setup for google benchmark
if(ramses-logic_BUILD_TESTS AND NOT TARGET benchmark::benchmark)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE INTERNAL "")
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE INTERNAL "")

    ensure_submodule_exists(google-benchmark)

    add_subdirectory(google-benchmark)

    folderize_target(benchmark "external")
    folderize_target(benchmark_main "external")
endif()

add_library(rlogic-google-benchmark-main INTERFACE)
target_link_libraries(rlogic-google-benchmark-main INTERFACE benchmark_main)
add_library(rlogic::google-benchmark-main ALIAS rlogic-google-benchmark-main)

################################################
################     Lua      ##################
################################################

if(NOT TARGET lua)
    ensure_submodule_exists(lua)

    # Collect all source and header files
    file(GLOB LUA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lua/*.c)
    file(GLOB LUA_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/lua/*.h)

    # Remove luac from the list, because it contains a main function
    list(REMOVE_ITEM LUA_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/lua/lua.c)

    set_source_files_properties(${LUA_SOURCE} PROPERTIES LANGUAGE CXX)

    # Create library for lua
    add_library(lua STATIC ${LUA_SOURCE} ${LUA_HEADER})

    # This is required for Lua on Android to suppress a false-positive fortification trigger when
    # the garbage collector is invoked
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        target_compile_options(lua PRIVATE -fno-stack-protector -U_FORTIFY_SOURCE)
    endif()

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        # We compile lua as c++ even though it has .c files. convince clang to just do it without complaining
        target_compile_options(lua PRIVATE -x c++)
    endif()

    target_include_directories(lua
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lua>
    )

    folderize_target(lua "external")
endif()

add_library(lua::lua ALIAS lua)

################################################
################     Sol      ##################
################################################

if(NOT TARGET sol2)
    set(SOL2_LUA_VERSION "5.1.1" CACHE STRING "" FORCE)
    set(INSTALL_SOL2 OFF CACHE INTERNAL "")

    ensure_submodule_exists(sol)

    add_subdirectory(sol)

    # Ensure sol is expecting c++ compiled lua
    target_compile_definitions(sol2 INTERFACE SOL_USING_CXX_LUA=1 SOL_EXCEPTIONS_ALWAYS_UNSAFE=1 SOL_SAFE_NUMERICS=1)
endif()

################################################
################ Flatbuffers  ##################
################################################

if(NOT TARGET flatbuffers)
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

    ensure_submodule_exists(flatbuffers)

    add_subdirectory(flatbuffers)
    folderize_target(flatbuffers "external")
endif()

add_library(rlogic::flatbuffers ALIAS flatbuffers)

if(ramses-logic_ENABLE_FLATBUFFERS_GENERATION)
    folderize_target(flatc "external")
endif()

################################################
################     fmt      ##################
################################################

if (NOT TARGET fmt)
    ensure_submodule_exists(fmt)

    add_subdirectory(fmt)
    folderize_target(fmt "external")
endif()

################################################
################     Ramses   ##################
################################################


if(ramses-logic_RAMSES_TARGET AND TARGET ${ramses-logic_RAMSES_TARGET})
    message(STATUS " + Using existing ramses target '${ramses-logic_RAMSES_TARGET}'")
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
        set(ramses-sdk_ENABLE_DLT OFF CACHE BOOL "" FORCE)
        set(ramses-sdk_FORCE_OFF_SOMEIP_HU ON CACHE BOOL "" FORCE)
        set(ramses-sdk_FORCE_OFF_SOMEIP_IC ON CACHE BOOL "" FORCE)

        # Disable some things which don't have a CMake option
        set(ramses-sdk_ramses-daemon          OFF CACHE BOOL "" FORCE)

        ensure_submodule_exists(ramses)

        #TODO Daniel find a better way to configure ramses renderer
        if(ramses-logic_BUILD_RAMSES_RENDERER)
            set(ramses-sdk_BUILD_FULL_SHARED_LIB ON CACHE BOOL "" FORCE)
            set(ramses-sdk_BUILD_CLIENT_ONLY_SHARED_LIB OFF CACHE BOOL "" FORCE)

            add_subdirectory(ramses)
            if ((NOT TARGET ${ramses-logic_RAMSES_TARGET})
                OR (${ramses-logic_RAMSES_TARGET} MATCHES "ramses-shared-lib-client-only")
                OR (NOT ${ramses-logic_RAMSES_TARGET} MATCHES "ramses-shared-lib-"))
                message(FATAL_ERROR "Invalid Ramses target '${ramses-logic_RAMSES_TARGET}'. If ramses-logic_BUILD_RAMSES_RENDERER is set, ramses-logic_RAMSES_TARGET has to be set to a shared library target of ramses which contains a renderer (e.g ramses-shared-lib-android-egl-es-3-0)")
            endif()
        else()
            set(ramses-sdk_BUILD_FULL_SHARED_LIB OFF CACHE BOOL "" FORCE)
            set(ramses-sdk_BUILD_CLIENT_ONLY_SHARED_LIB ON CACHE BOOL "" FORCE)
            # Disable some things which don't have a CMake option
            set(ramses-sdk_ramses-renderer-lib    OFF CACHE BOOL "" FORCE)
            set(ramses-sdk_renderer_RendererLib   OFF CACHE BOOL "" FORCE)

            add_subdirectory(ramses)

            set(ramses-logic_RAMSES_TARGET "ramses-shared-lib-client-only" CACHE STRING "" FORCE)
        endif()

        # This is kind of an assert - if this fails, there is a version mismatch with ramses
        if(NOT TARGET ${ramses-logic_RAMSES_TARGET})
            message(FATAL_ERROR "Invalid Ramses target '${ramses-logic_RAMSES_TARGET}'")
        endif()
    else()
        message(FATAL_ERROR "Must either provide a valid ramses target (current value: '${ramses-logic_RAMSES_TARGET}') or set ramses-logic_ALLOW_RAMSES_BUILD=ON")
    endif()
endif()

# Requires >= 27.0.1 because we use new rotation API of R27 and it was implemented in patch version
# Version less than 28 because not tested with 28 yet
set(RAMSES_VERSION_GE 27.0.1)
set(RAMSES_VERSION_LT 28)
if(${ramses-sdk_VERSION} VERSION_GREATER_EQUAL ${RAMSES_VERSION_GE} AND ${ramses-sdk_VERSION} VERSION_LESS ${RAMSES_VERSION_LT})
    message(STATUS "Using ramses target '${ramses-logic_RAMSES_TARGET}' in version '${ramses-sdk_VERSION}' (for ramses logic)")
else()
    message(FATAL_ERROR "Using incompatible ramses version '${ramses-sdk_VERSION}'. Expected >= ${RAMSES_VERSION_GE} and < ${RAMSES_VERSION_LT}")
endif()
