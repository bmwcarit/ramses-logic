#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# Only enable flatbuffer generation targets when built as top-level project
if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    return()
endif()

# This CMake file creates two targets (FlatbufGen and FlatbufCheck)
# FlatbufGen: re-generates flatbuffers header files from schemas, puts results in source tree
# FlatbufCheck: checks that generated headers are exactly the same as ones in source tree
#
# Works like this:
# Case #1: with code generation (default)
# cmake <src>       # Creates FlatbufGen and FlatbufCheck targets based on *.fbs files
# make              # generates *_gen.h files from *.fbs files and overwrites source tree.
#                   # If a new *.fbs file was added, has to re-run cmake (as with any other src files)
#                   # If existing *.fbs file changes, regenerates ALL *_gen.h files and rebuilds anything that depends on them
#                   # If no change in *.fbs files, does nothing
# make FlatbufCheck # Compares *_gen.h files in git tree to *_gen.h files in working tree
#                   # If *_gen.h files in working tree differ from their git counterparts - prints the diff and fails
#                   # Otherwise succeeds
#                   # This job is always executed in CI builds to ensure generated files are consistent with schema changes
#
# Case #2: no code generation (explicitly disabled)
# cmake -Dramses-logic_ENABLE_FLATBUFFERS_GENERATION=OFF <src>
# make              # Uses checked-in *_gen.h files and ignores any changes in *.fbs files
# make FlatbufGen   # Fails - no such targets because disabled
# make FlatbufCheck # Fails - no such targets because disabled

file(GLOB flatbuffers_schemas "${PROJECT_SOURCE_DIR}/src/flatbuffers/schemas/*.fbs")
set(flatbuffers_output_dir "${PROJECT_SOURCE_DIR}/src/flatbuffers/generated")

# create list of *_generated.h out of *.fbs file list
foreach(schema ${flatbuffers_schemas})
    get_filename_component(filename ${schema} NAME_WE)
    list(APPEND generated_headers "${flatbuffers_output_dir}/${filename}_gen.h")
endforeach()

# TODO Violin/Tobias investigate the "--conform" option of flatc - looks very promising
# as a solution for backwards compatibility which can be automatically checked!

# Additional interesting options to investigate in the future:
# --reflect-types/--reflect-names  - maybe for type backwards compatibility
# --root-type - maybe not specify the root type in schemas, but from outside
# --force-empty-vectors - avoid nullptr
# --flexbuffers - schemaless buffers

# Custom command which:
# - deletes all generated headers in source TARGET_FILE
# - regenerates headers from current *.fbs files in source tree
add_custom_command(
    OUTPUT  ${generated_headers}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${flatbuffers_output_dir}
    COMMAND ${CMAKE_COMMAND} -E make_directory  ${flatbuffers_output_dir}
    COMMAND $<TARGET_FILE:flatc>
        -o ${flatbuffers_output_dir}/
        --cpp ${flatbuffers_schemas}
        --filename-suffix _gen
        --filename-ext h
        --scoped-enums
        --no-prefix
        --cpp-std c++17
    DEPENDS ${flatbuffers_schemas} $<TARGET_FILE:flatc>
    COMMENT "Generating flatbuffers header files from schemas"
    VERBATIM
)

add_custom_target(FlatbufGen
    DEPENDS ${generated_headers}
)

set_target_properties(FlatbufGen PROPERTIES
    FOLDER "CMakePredefinedTargets")

message(STATUS " + FlatbufGen")

# Custom command which checks that generated flatbuffers files are not different than checked-in ones

find_package(Git)
if(Git_FOUND)
    # TODO Violin/Tobias check if there is a cleaner way to tell CMake to execute this command regardless of dependencies
    set(NON_EXISTENT_FILE this_file_does_not_exist.stamp)
    add_custom_command(
        OUTPUT  ${NON_EXISTENT_FILE}
        COMMAND ${GIT_EXECUTABLE} -C ${PROJECT_SOURCE_DIR} diff --exit-code ${flatbuffers_output_dir}
        COMMENT "Ensure flatbuffers header files are content-wise equivalent to checked-in ones"
        VERBATIM
    )
    add_custom_target(FlatbufCheck
        DEPENDS ${NON_EXISTENT_FILE}
    )

    # FlatbufCheck target should always be only ever executed if explicitly invoked
    set_target_properties(FlatbufCheck PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        FOLDER "CMakePredefinedTargets")

    message(STATUS " + FlatbufCheck")
else()
    message(STATUS " - FlatbufCheck (Git missing)")
endif()
