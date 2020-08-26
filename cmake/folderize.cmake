#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# TODO Violin/Tobias this is a more generic, less ramses-y and no-ACME version of "folderize"
# Do we want to share such code across projects?

# extract and set folder name from path
function(GET_CURRENT_FOLDER_PATH OUT)
    # first get path relative to project root dir
    string(REGEX REPLACE "${PROJECT_SOURCE_DIR}/" "" folder_relative_path "${CMAKE_CURRENT_SOURCE_DIR}")
    string(REGEX REPLACE "/[^/]*$" "" filtered_folder_path "${folder_relative_path}")
    if (NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        # optionally adjust path when built as subdirectory
        string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/" "" folder_prefix "${PROJECT_SOURCE_DIR}")
        set(filtered_folder_path "${folder_prefix}/${filtered_folder_path}")
    endif()
    set(${OUT} ${filtered_folder_path} PARENT_SCOPE)
endfunction()

function(FOLDERIZE_TARGET tgt)
    # skip interface libs because VS generator ignores INTERFACE_FOLDER property
    get_target_property(tgt_type ${tgt} TYPE)
    if (tgt_type STREQUAL INTERFACE_LIBRARY)
        return()
    endif()

    GET_CURRENT_FOLDER_PATH(target_folder)
    set_property(TARGET ${tgt} PROPERTY FOLDER "${target_folder}")

endfunction()

function(FOLDERIZE_TARGETS)
    foreach (tgt ${ARGV})
        FOLDERIZE_TARGET(${tgt})
    endforeach()
endfunction()
