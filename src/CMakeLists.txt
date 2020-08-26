#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(ramses-logic_ENABLE_FLATBUFFERS_GENERATION)
    include(${PROJECT_SOURCE_DIR}/cmake/flatbuffersGeneration.cmake)
endif()

# TODO split CMakeLists into "gen", "lib" and "test" parts
file(GLOB public_headers include/public/ramses-logic/*.h)
file(GLOB impl_headers include/internals/impl/*.h)
file(GLOB internal_headers include/internals/*.h)
file(GLOB flatbuf_gen_headers flatbuffers/generated/*.h)
file(GLOB flatbuf_schemas flatbuffers/schemas/*.fbs)

file(GLOB public_src src/public/*.cpp)
file(GLOB impl_src src/internals/impl/*.cpp)
file(GLOB internals_src src/internals/*.cpp)

add_library(ramses-logic-obj OBJECT)

target_sources(ramses-logic-obj
    PRIVATE
        ${public_headers}
        ${impl_headers}
        ${internal_headers}
        ${public_src}
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
source_group("Source Files" FILES ${public_src})
source_group("Source Files\\internals\\impl" FILES ${impl_src})
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
        ${ramses-logic_RAMSES_TARGET}
)

# Add dependency to flatbuffer generation target, unless the target is disabled
if(TARGET FlatbufGen)
    add_dependencies(ramses-logic-obj FlatbufGen)
endif()

target_include_directories(ramses-logic-obj
    PRIVATE
        include/public
        include
        flatbuffers
)

target_compile_definitions(ramses-logic-obj PRIVATE RLOGIC_LINK_SHARED_EXPORT=1)

################## ramses-logic-static ##################
add_library(ramses-logic-static STATIC)
add_library(rlogic::ramses-logic-static ALIAS ramses-logic-static)
target_link_libraries(ramses-logic-static PRIVATE ramses-logic-obj)
target_link_libraries(ramses-logic-static PUBLIC ${ramses-logic_RAMSES_TARGET})
target_include_directories(ramses-logic-static PUBLIC include/public)
set_target_properties(ramses-logic-static PROPERTIES PUBLIC_HEADER "${public_headers}")

################## ramses-logic (shared library) ##################
add_library(ramses-logic SHARED)
add_library(rlogic::ramses-logic ALIAS ramses-logic)
target_link_libraries(ramses-logic PRIVATE ramses-logic-obj)
target_link_libraries(ramses-logic PUBLIC ${ramses-logic_RAMSES_TARGET})
target_include_directories(ramses-logic PUBLIC include/public)
set_target_properties(ramses-logic PROPERTIES
    PUBLIC_HEADER "${public_headers}"
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
target_compile_definitions(ramses-logic PUBLIC RLOGIC_LINK_SHARED_IMPORT=1)

FOLDERIZE_TARGETS(ramses-logic-obj ramses-logic-static ramses-logic)

# TODO Violin/Sven/Tobias Discuss if we are happy with target naming before first release

# TODO Violin/Sven/Tobias consider making optional or settable by user?
install(
    TARGETS ramses-logic
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

if(ramses-logic_BUILD_TESTS)

    file(GLOB ramses-logic-test_SRC test/*.cpp)
    file(GLOB ramses-logic-test_INC test/*.h)

    add_executable(ramses-logic-test
        ${ramses-logic-test_INC}
        ${ramses-logic-test_SRC}
    )

    # TODO split tests into API tests (no sol/lua) and impl tests (with sol/lua)
    target_link_libraries(ramses-logic-test
        PRIVATE
            rlogic::ramses-logic-static
            rlogic::gmock-main
            sol2::sol2
            lua::lua
            fmt::fmt
            rlogic::flatbuffers
            ${ramses-logic_RAMSES_TARGET}

        )

    target_include_directories(ramses-logic-test
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/flatbuffers>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    add_test(
        NAME ramses-logic-test
        COMMAND $<TARGET_FILE:ramses-logic-test>
        WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    set_target_properties(ramses-logic-test PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    )

    FOLDERIZE_TARGET(ramses-logic-test)

    if (ramses-logic_ENABLE_TEST_COVERAGE)
        # attach environment variable for clang coverage
        set_tests_properties(ramses-logic-test PROPERTIES
            ENVIRONMENT LLVM_PROFILE_FILE=ramses-logic-test_%p.profraw)
    endif()

endif()
