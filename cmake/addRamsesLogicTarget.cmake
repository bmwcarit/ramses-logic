#  -------------------------------------------------------------------------
#  Copyright (C) 2021 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

function(add_ramses_logic_target TARGET_NAME LIB_TYPE)
    add_library(${TARGET_NAME} ${LIB_TYPE})
    add_library(rlogic::${TARGET_NAME} ALIAS ${TARGET_NAME})
    target_link_libraries(${TARGET_NAME} PRIVATE ramses-logic-obj)
    target_link_libraries(${TARGET_NAME} PUBLIC ${RAMSES_TARGET})
    target_include_directories(${TARGET_NAME} PUBLIC include)
    set_target_properties(${TARGET_NAME} PROPERTIES
            PUBLIC_HEADER "${public_headers}"
        )
    if(${LIB_TYPE} STREQUAL "STATIC")
        # no special additional properties for static lib required"
    elseif(${LIB_TYPE} STREQUAL "SHARED")
        set_target_properties(${TARGET_NAME} PROPERTIES
            SOVERSION ${PROJECT_VERSION_MAJOR}
        )
        if (NOT ramses-logic_DISABLE_SYMBOL_VISIBILITY)
            target_compile_definitions(${TARGET_NAME} PUBLIC RLOGIC_LINK_SHARED_IMPORT=1)
        endif()
    else()
        message(FATAL_ERROR "invalid LIB_TYPE ${LIB_TYPE}, variable has to be either set to 'STATIC' or 'SHARED'")
    endif()
    folderize_target(${TARGET_NAME} "ramses-logic")
endfunction()
