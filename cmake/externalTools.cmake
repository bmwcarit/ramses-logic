#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

### find python3

# ensure python3 can be found (when python2 was searched before)
if(ramses-logic_ENABLE_CODE_STYLE)
    unset(PYTHONINTERP_FOUND CACHE)
    unset(PYTHON_EXECUTABLE CACHE)
    find_package(PythonInterp 3.6)

    if (PYTHONINTERP_FOUND AND PYTHON_EXECUTABLE)
        set(rlogic_PYTHON3 "${PYTHON_EXECUTABLE}")
    endif()
endif()


### enable ccache when configured and available

if(ramses-logic_USE_CCACHE)
    find_program(CCACHE_EXECUTABLE ccache)
    if(CCACHE_EXECUTABLE)
        message(STATUS " + CCACHE")
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
        set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_EXECUTABLE})
    else()
        message(STATUS " - CCACHE [missing executable]")
    endif()
endif()
