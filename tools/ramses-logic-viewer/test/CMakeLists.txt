#  -------------------------------------------------------------------------
#  Copyright (C) 2022 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

add_executable(ramses-logic-viewer-test
    LogicViewerTestBase.h
    LogicViewerTestBase.cpp
    LogicViewerTest.cpp
    LogicViewerLuaTest.cpp
)

target_link_libraries(ramses-logic-viewer-test
    PRIVATE
        ramses-logic-viewer-obj
        rlogic::gmock-main
    )

target_include_directories(ramses-logic-viewer-test
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/ramses-logic-viewer>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/unittests/shared>
)

MakeTestFromTarget(ramses-logic-viewer-test)
folderize_target(ramses-logic-viewer-test "tools")

add_executable(ramses-logic-viewer-test_SWRAST
    LogicViewerAppTest.cpp
)

target_link_libraries(ramses-logic-viewer-test_SWRAST
    PRIVATE
        ramses-logic-viewer-obj
        rlogic::gmock-main
    )

target_include_directories(ramses-logic-viewer-test_SWRAST
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/ramses-logic-viewer>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/unittests/shared>
)

MakeTestFromTarget(ramses-logic-viewer-test_SWRAST)
folderize_target(ramses-logic-viewer-test_SWRAST "tools")
