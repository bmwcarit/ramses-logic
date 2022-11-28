#  -------------------------------------------------------------------------
#  Copyright (C) 2022 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

find_package(ImageMagick COMPONENTS compare)

if(ramses-logic_USE_IMAGEMAGICK AND NOT ImageMagick_FOUND)
    message(FATAL_ERROR "ImageMagick compare not found, but required (ramses-logic_USE_IMAGEMAGICK=ON)")
endif()

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

if (ImageMagick_FOUND AND ramses-logic_PLATFORM)
    add_executable(ramses-logic-viewer-test_SWRAST
        LogicViewerAppTest.cpp
    )

    target_compile_definitions(ramses-logic-viewer-test_SWRAST
        PRIVATE MAGICK_COMPARE="${ImageMagick_compare_EXECUTABLE}"
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

    add_custom_command(TARGET ramses-logic-viewer-test_SWRAST PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res/unittests)
endif()
