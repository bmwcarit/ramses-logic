#  -------------------------------------------------------------------------
#  Copyright (C) 2021 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

add_executable(ramses-logic-viewer
    main.cpp
    ImguiClientHelper.h
    ImguiClientHelper.cpp
    LogicViewer.h
    LogicViewer.cpp
    LogicViewerGui.h
    LogicViewerGui.cpp
    LogicViewerLuaTypes.h
    LogicViewerLuaTypes.cpp
    Arguments.h
    SceneSetup.h
    UpdateReportSummary.h
    )

target_include_directories(ramses-logic-viewer
    PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tools/ramses-logic-utils>
)

set(RAMSES_LOGIC_VIEWER_LIBS imgui fmt::fmt sol2::sol2 lua::lua)

target_link_libraries(ramses-logic-viewer PRIVATE rlogic::ramses-logic-utils)
if(TARGET rlogic::ramses-logic)
    target_link_libraries(ramses-logic-viewer PRIVATE rlogic::ramses-logic ${RAMSES_LOGIC_VIEWER_LIBS})
else()
    target_link_libraries(ramses-logic-viewer PRIVATE rlogic::ramses-logic-static ${RAMSES_LOGIC_VIEWER_LIBS})
endif()

folderize_target(ramses-logic-viewer "tools")

if (ramses-logic_ENABLE_INSTALL)
    install(
        TARGETS ramses-logic-viewer
        DESTINATION ${RLOGIC_INSTALL_RUNTIME_PATH})
endif()
