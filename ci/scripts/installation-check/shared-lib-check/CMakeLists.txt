#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ramses-logic-shared-check)

add_custom_target(run-all ALL)

unset(ramses-logic_FOUND CACHE)
unset(ramses-logic_LIBRARIES CACHE)
unset(ramses-logic_INCLUDE_DIRS CACHE)

find_package(ramses-logic REQUIRED NO_CMAKE_BUILDS_PATH)

add_executable(ramses-logic-shared-check "ramses-logic-shared-lib-check.cpp")
target_link_libraries(ramses-logic-shared-check rlogic::ramses-logic)

add_custom_target(run-ramses-logic-shared-check
    COMMAND $<TARGET_FILE:ramses-logic-shared-check>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ramses-logic-shared-check
    COMMENT "Run executable ramses-logic-shared-check")

add_dependencies(run-all run-ramses-logic-shared-check)
