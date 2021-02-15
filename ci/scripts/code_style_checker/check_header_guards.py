#!/usr/bin/env python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import sys
import re
from common_modules import common


def check_header_guards(filename, file_contents):
    """
    Check if a header files contains valid header guards:
        1- #ifndef GUARDNAME directly after license that has a guard name identical to file name in uppercase letters,
           underscores at beginnings and end are tollerable
        2- directly followed by #define that matches the guard name of #ifndef exactly
        3- the file has to end with an #endif

    """
    is_h_file = filename[-2:] == ".h"
    is_hpp_file = filename[-4:] == ".hpp"
    if not (is_h_file or is_hpp_file):
        return

    # remove strings
    string_re = re.compile(r'"((?!((?<!((?<!\\)\\))")).)*"', re.DOTALL)
    file_contents = common.clean_string_from_regex(file_contents, string_re, 'x')

    # remove multi-line comments
    ml_comment_re = re.compile(r'(\s*)/\*((?!\*/).)*\*/', re.DOTALL)
    file_contents = common.clean_string_from_regex(file_contents, ml_comment_re, '')

    # remove single line comments
    sl_comment_re = re.compile("//.*$", re.MULTILINE)
    file_contents = re.sub(sl_comment_re, '', file_contents)

    # remove any padding white space characters
    file_contents = file_contents.strip(" \t\n\f\r")

    # find the pragma once guard
    pragma_re = re.compile(r"(\s*)#pragma(\s+)once(\s*)", re.MULTILINE)
    pragma_match = re.findall(pragma_re, file_contents)
    if len(pragma_match) == 0:
        common.log_warning("check_header_guards", filename, 1, "header guard missing")

    if len(pragma_match) > 1:
        common.log_warning("check_header_guards", filename, 1, "more than one occurance of header guard found")

    return None


if __name__ == "__main__":
    targets = sys.argv[1:]
    targets = common.get_all_files(targets)

    if len(targets) == 0:
        print("""
\t**** No input provided ****
\tTakes a list of files/directories as input and performs specific style checking on all files/directories

\tGives warnings if header guards does not exist in header files. Header files must have a #ifndef and #define guards directly after the license.
\tThe names of the header guards must match together and match with the name of the file in uppercase letters.
""")
        exit(0)

    for t in targets:
        if t[-2:] == ".h":
            file_contents, _ = common.read_file(t)
            check_header_guards(t, file_contents)
