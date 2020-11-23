#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# error if doxygen not found
find_package(Doxygen QUIET REQUIRED)
find_package(Sphinx QUIET REQUIRED)

# input files
file(GLOB_RECURSE
    DOXYGEN_INPUT_LIST
    ${PROJECT_SOURCE_DIR}/include/ramses-logic/*.h
)

# image dirs
file(GLOB_RECURSE
    DOXYGEN_IMAGE_PATH_LIST
    LIST_DIRECTORIES TRUE
    ${CMAKE_CURRENT_SOURCE_DIR}/images)

# variable setup
set(DOXYGEN_TARGET_DIR     "${CMAKE_BINARY_DIR}/doxygen")
set(DOXYGEN_EXAMPLE_PATH   "${PROJECT_SOURCE_DIR}/examples")

string(REPLACE ";" " " DOXYGEN_INPUT  "${DOXYGEN_INPUT_LIST}")
string(REPLACE ";" " " DOXYGEN_IMAGE_PATH  "${DOXYGEN_IMAGE_PATH_LIST}")

# generate doxygen config file
set(DOXYGEN_CONFIG_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/doxygen/Doxyfile.in")
set(DOXYGEN_CONFIG_FILE "${CMAKE_BINARY_DIR}/Doxyfile")

configure_file(${DOXYGEN_CONFIG_FILE_IN} ${DOXYGEN_CONFIG_FILE} @ONLY)

# create error checker script
file(WRITE "${CMAKE_BINARY_DIR}/check-doxygen-errors.cmake"
    "file(READ \"DoxygenWarningLog.txt\" INP)\n"
    "if (NOT \"\${INP}\" STREQUAL \"\")\n"
    "  message(FATAL_ERROR \"Encountered doxygen warnings:\\n\${INP}\")\n"
    "endif()\n")

set(DOXYGEN_STAMP_FILE ${DOXYGEN_TARGET_DIR}/doxygen.stamp)

# run doxygen
add_custom_command(OUTPUT ${DOXYGEN_STAMP_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DOXYGEN_TARGET_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch ${DOXYGEN_STAMP_FILE}
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/check-doxygen-errors.cmake"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating doxygen files in: ${DOXYGEN_TARGET_DIR}" VERBATIM
    DEPENDS ${DOXYGEN_INPUT_LIST} ${DOXYGEN_CONFIG_FILE_IN})

# add target to create doxygen xml
add_custom_target(rlogic-doxygen
    COMMENT "Generated doxygen files in: ${DOXYGEN_TARGET_DIR}" VERBATIM
    DEPENDS ${DOXYGEN_STAMP_FILE}
    SOURCES ${DOXYGEN_INPUT_LIST} ${DOXYGEN_CONFIG_FILE_IN}
)

folderize_target(rlogic-doxygen "doc")

set(SPHINX_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/sphinx)
set(SPHINX_TARGET_DIR ${CMAKE_BINARY_DIR}/sphinx)
set(SPHINX_IDX_FILE ${SPHINX_TARGET_DIR}/index.html)

file(GLOB_RECURSE
    SPHINX_INPUT_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/sphinx/*)

# Only regenerate Sphinx files when needed (either doxygen target or sphinx input files changed)
add_custom_command(OUTPUT ${SPHINX_IDX_FILE}
                   COMMAND
                    ${SPHINX_EXECUTABLE}
                        -b html
                        -Dbreathe_projects.ramses_logic=${DOXYGEN_TARGET_DIR}/xml
                        -W --keep-going
                        ${SPHINX_SRC_FILES} ${SPHINX_TARGET_DIR}
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   DEPENDS
                        ${SPHINX_INPUT_LIST}
                        ${DOXYGEN_STAMP_FILE}
                        ${PROJECT_SOURCE_DIR}/README.rst
                        ${PROJECT_SOURCE_DIR}/CONTRIBUTING.rst
                        ${PROJECT_SOURCE_DIR}/CHANGELOG.rst
                   MAIN_DEPENDENCY ${SPHINX_SRC_FILES}/conf.py
                   COMMENT "Generating Sphinx docs")

# add target to create sphinx html
add_custom_target(rlogic-sphinx ALL
    DEPENDS ${SPHINX_IDX_FILE}
    SOURCES ${SPHINX_INPUT_LIST} ${SPHINX_SRC_FILES}/conf.py
)

folderize_target(rlogic-sphinx "doc")
