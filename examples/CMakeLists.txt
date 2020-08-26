#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

function(add_example name)
    add_executable(${name} ${name}/main.cpp)

    target_link_libraries(${name}
        PRIVATE
            rlogic::ramses-logic
    )

    # TODO Violin reduce code duplication for creating tests
    if(ramses-logic_BUILD_TESTS)
        set(testname ${name})
        add_test(
            NAME ${testname}
            COMMAND $<TARGET_FILE:${name}>
            WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )

        set_target_properties(${name} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif()

    FOLDERIZE_TARGET(${name})

endfunction()

add_example(00_minimal)
add_example(01_properties_simple)
add_example(02_properties_complex)
add_example(03_errors_compile_time)
add_example(04_errors_runtime)
