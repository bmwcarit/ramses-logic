#!/usr/bin/env python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

"""
Runs all enabled style checking

"""

import sys
import os

import common_modules.common

from check_license import check_license_for_file
from check_header_guards import check_header_guards
from check_tabbing_and_spacing import check_tabbing_and_spacing
from check_tabbing_and_spacing import check_no_spacing_line_end
from check_tabbing_and_spacing import check_tabs_no_spaces
from check_enum_style import check_enum_style
from check_last_line_newline import check_last_line_newline
from check_deprecated import check_deprecated


def main():
    # TODO OSS Check for more elegant solution, which still ensures that license files are properly checked
    root_path = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..", ".."))

    print("Checking {}".format(root_path))

    binary_files = {
        r'\.res$',
        r'\.pptx$',
        r'\.svg',
        r'\.SVG',
        r'\.png$',
        r'\.PNG$',
        r'\.bmp$',
        r'\.BMP$',
        r'\.ttf$',
        r'\.pyc$',
        r'\.bin$',
        r'\.pac$',
        r'\.rex$',
        r'\.ctm$',
        r'\.tar\.bz2$',
        r'\.jar$',
    }

    # These files are not checked at all
    shared_blacklist = binary_files | {
        r'\.git',
        r'\.gitignore',
        r'gitconfig$',
        # Flatbuffer generated files
        r'_generated.h',
        # Should not be checked for anything, contains plain hashsum
        r'DOCKER_TAG',
        r'(^|/)build[^/]*/',
    }

    # Whitelist for source files
    src_whitelist = {
        r'\.h$',
        r'\.hpp$',
        r'\.cpp$'
    }

    generated_files = {
        r'^lib/flatbuffers/generated'
    }

    # Externals are allowed to have their own code style
    src_blacklist = shared_blacklist | {r'^external/'} | generated_files

    src_files = common_modules.common.get_all_files_with_filter(root_path, root_path, src_whitelist, src_blacklist)

    # Check all styles for source files
    for f in src_files:

        file_contents, file_lines = common_modules.common.read_file(f)
        clean_file_contents, clean_file_lines = common_modules.common.clean_file_content(file_contents)

        check_header_guards(f, file_contents)
        check_license_for_file(f, file_contents, root_path)
        check_tabbing_and_spacing(f, file_lines)
        check_enum_style(f, clean_file_contents)
        check_last_line_newline(f, file_contents)
        check_deprecated(f, file_contents, clean_file_contents, file_lines, clean_file_lines)
        # TODO figure out a better way to check API symbols are properly exported
        # check_api_export_symbols(f, clean_file_contents)

    shared_blacklist_non_src_files = shared_blacklist | {
        # Externals allowed to have own formatting and license
        r'^external',
        r'^CHANGELOG\.md$',  # Doesn't need a license
        r'^LICENSE\.txt$',  # Contains license info, not related to code/content
    }

    blacklist_files_formatting = shared_blacklist_non_src_files

    files_formatting = common_modules.common.get_all_files_with_filter(root_path, root_path, {r'.*'}, blacklist_files_formatting)

    # Check subset of the rules for non-source files
    for f in files_formatting:
        file_contents, file_lines = common_modules.common.read_file(f)

        check_tabs_no_spaces(f, file_lines)
        check_no_spacing_line_end(f, file_lines)
        check_last_line_newline(f, file_contents)

    blacklist_license = shared_blacklist_non_src_files | generated_files | {
        # Can be safely excluded, don't need license header because trivial
        r'__init__\.py$',
        # Excluded on purpose - add new lines reasonibly here!
        r'^ci/scripts/config/ubsan_suppressions$',   # config file, can't add license
        r'\.md$',                           # .md files don't need a license
        r'.*/AndroidManifest\.xml$',        # Android manifests are difficult to modify with a license header
        r'^android/gradle\.properties$',    # Gradle config file, no license needed
        r'^android/gradlew$',               # Gradle wrapper script, has own Apache 2.0 header (according to Gradle plugin license), exclude check here
        r'\.spdx$',                         # Spdx archives don't need a license header themselves
    }

    files_license_header = common_modules.common.get_all_files_with_filter(root_path, root_path, {r'.*'}, blacklist_license)

    # Check subset of the rules for non-source files
    for f in files_license_header:

        file_contents, file_lines = common_modules.common.read_file(f)

        check_license_for_file(f, file_contents, root_path)

    print('checked {0} files'.format(len(set(src_files) | set(files_formatting) | set(files_license_header))))

    if 0 == common_modules.common.G_WARNING_COUNT:
        print("your style is awesome! no style guide violations detected.")
        return 0
    else:
        print("detected {0} style guide issues".format(common_modules.common.G_WARNING_COUNT))
        return 1


sys.exit(main())
