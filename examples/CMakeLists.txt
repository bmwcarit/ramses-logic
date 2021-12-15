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

    # Prefer shared library for linking examples
    if(TARGET rlogic::ramses-logic)
        target_link_libraries(${EXAMPLE_NAME}
            PRIVATE
                rlogic::ramses-logic
        )
    else()
        target_link_libraries(${EXAMPLE_NAME}
            PRIVATE
                rlogic::ramses-logic-static
        )
    endif()

    folderize_target(${EXAMPLE_NAME} "examples")

    if(ramses-logic_BUILD_TESTS AND ${EXAMPLE_MAKE_TEST})
        MakeTestFromTarget(${EXAMPLE_NAME})
    endif()
endfunction()

add_example(NAME 00_minimal               FILES 00_minimal/main.cpp               MAKE_TEST)
add_example(NAME 01a_primitive_properties FILES 01a_primitive_properties/main.cpp MAKE_TEST)
add_example(NAME 01b_struct_properties    FILES 01b_struct_properties/main.cpp    MAKE_TEST)
add_example(NAME 01c_array_properties     FILES 01c_array_properties/main.cpp     MAKE_TEST)
add_example(NAME 02_errors_compile_time   FILES 02_errors_compile_time/main.cpp   MAKE_TEST)
add_example(NAME 03_errors_runtime        FILES 03_errors_runtime/main.cpp        MAKE_TEST)
# Can't execute ramses runtime example as test - it has a loop which blocks test automation and wastes resources
add_example(NAME 04_ramses_scene          FILES 04_ramses_scene/main.cpp)
add_example(NAME 05_serialization         FILES 05_serialization/main.cpp         MAKE_TEST)
add_example(NAME 07_links                 FILES 07_links/main.cpp                 MAKE_TEST)
add_example(NAME 08_animation             FILES 08_animation/main.cpp             MAKE_TEST)
add_example(NAME 09_modules               FILES 09_modules/main.cpp               MAKE_TEST)
add_example(NAME 10_globals               FILES 10_globals/main.cpp               MAKE_TEST)
