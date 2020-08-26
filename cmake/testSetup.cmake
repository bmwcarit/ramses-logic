#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# helper functions for unit and integration tests

# Disable unneeded Dart targets on windows, but keep them on Linux for Valgrind
# Sadly, the CTest docs are fuzzy about this hidden and weird behavior...
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_testing()
else()
    include(CTest)
endif()

string(CONCAT VALGRIND_OPTIONS
    "--log-fd=1 "
    "--error-exitcode=1 "
    "--leak-check=full "
    "--show-leak-kinds=definite,possible "
    "--errors-for-leak-kinds=definite,possible "
    "--undef-value-errors=yes "
    "--track-origins=no "
    "--child-silent-after-fork=yes "
    "--trace-children=yes "
    "--num-callers=50 "
    "--fullpath-after=${CMAKE_CURRENT_SOURCE_DIR}/ "
    "--gen-suppressions=all "
)
set(MEMORYCHECK_COMMAND_OPTIONS ${VALGRIND_OPTIONS} CACHE INTERNAL "")

