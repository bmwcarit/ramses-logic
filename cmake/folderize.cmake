#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# When build as submodule and no folder prefix set by user, set prefix based on relative path
if (ramses-logic_FOLDER_PREFIX)
    set(RL_FOLDER_PREFIX "${ramses-logic_FOLDER_PREFIX}/")
elseif (NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    string(REGEX REPLACE "${CMAKE_SOURCE_DIR}/" "" folder_prefix "${PROJECT_SOURCE_DIR}")
    set(RL_FOLDER_PREFIX "${folder_prefix}/")
endif()

function(folderize_target tgt folder)
    # error on interface libs because VS generator ignores INTERFACE_FOLDER property
    get_target_property(tgt_type ${tgt} TYPE)
    if (tgt_type STREQUAL INTERFACE_LIBRARY)
        message(FATAL_ERROR "${tgt} is an interface library and can't be folderized")
    endif()

    set_property(TARGET ${tgt} PROPERTY FOLDER "${RL_FOLDER_PREFIX}${folder}")

endfunction()
