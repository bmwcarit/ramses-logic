#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

function(add_example)
    cmake_parse_arguments(
        EXAMPLE             # Prefix of parsed args
        "MAKE_TEST"         # Options
        "NAME"              # Single-value args
        "FILES"             # Multi-value-args
        ${ARGN}
    )

    add_executable(${EXAMPLE_NAME} ${EXAMPLE_FILES})

    target_link_libraries(${EXAMPLE_NAME}
        PRIVATE
            rlogic::ramses-logic
    )

    FOLDERIZE_TARGET(${EXAMPLE_NAME})

    if(ramses-logic_BUILD_TESTS AND ${EXAMPLE_MAKE_TEST})
        MakeTestFromTarget(${EXAMPLE_NAME})
    endif()
endfunction()

add_example(NAME 00_minimal               FILES 00_minimal/main.cpp               MAKE_TEST)
add_example(NAME 01_properties_simple     FILES 01_properties_simple/main.cpp     MAKE_TEST)
add_example(NAME 02_properties_complex    FILES 02_properties_complex/main.cpp    MAKE_TEST)
add_example(NAME 03_errors_compile_time   FILES 03_errors_compile_time/main.cpp   MAKE_TEST)
add_example(NAME 04_errors_runtime        FILES 04_errors_runtime/main.cpp        MAKE_TEST)
# Can't execute ramses examples as test - they have a loop which blocks test automation and wastes resources
add_example(NAME 05_ramses_scene          FILES 05_ramses_scene/main.cpp)
add_example(NAME 06_serialization         FILES 06_serialization/main.cpp)
