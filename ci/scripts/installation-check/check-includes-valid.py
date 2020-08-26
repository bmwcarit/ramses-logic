#!/usr/bin/env python3

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

import glob
import os
import sys
import subprocess
import re



def get_files_in_directory_recursive(path, ext = None):
    res = []
    for (dir, _, files) in os.walk(path):
        for f in files:
            if not ext or os.path.splitext(f)[1] == ext:
                res.append(os.path.join(dir, f))
    return res


def get_installed_include_dir(installDir):
    fullIncludeDir = os.path.join(installDir, 'include')
    return fullIncludeDir


def get_installed_includes(installIncludeDir):
    allFiles = get_files_in_directory_recursive(installIncludeDir)
    relativeToInstallDir = [os.path.relpath(h, installIncludeDir) for h in allFiles]
    withoutRamses = [h for h in relativeToInstallDir if not re.match(r'^ramses-\d+', h)]
    return withoutRamses


def get_source_api_headers(srcDir):
    public_headers_dir = os.path.join(srcDir, 'src/include/public')

    headers = get_files_in_directory_recursive(public_headers_dir, '.h')
    res = [os.path.relpath(h, public_headers_dir) for h in headers]

    return res


def main():
    if len(sys.argv) != 3:
        print('Install header check: Arguments missing')
        print('Usage: ' + sys.argv[0] + ' <ramses-logic> <install-base>')
        sys.exit(1)

    print("Running install header check")

    srcDir = sys.argv[1]
    installDir = sys.argv[2]

    installedIncludeDir = get_installed_include_dir(installDir)

    installedHeaders = get_installed_includes(installedIncludeDir)
    srcApiHeaders = get_source_api_headers(srcDir)

    # check wich headers are unexpected and which are missing
    installedSet = set(installedHeaders)
    srcSet = set(srcApiHeaders)

    unexpected = list(installedSet - srcSet)
    missing = list(srcSet - installedSet)

    if len(unexpected) > 0:
        print('ERROR: Headers should not be installed\n  ' + '\n  '.join(unexpected))
        return 1
    if len(missing) > 0:
        print('ERROR: Headers are missing from installation\n  ' + '\n  '.join(missing))
        return 1

    # check installed header with pedantic
    print("Checking strict header compilation")
    numPedanticErrors = 0
    for h in installedHeaders:
        temp_file = f"/tmp/rlogic_pedantic_header.cpp"
        with open(temp_file, "w") as file:
            file.writelines(f"#include \"{h}\"\n\n")
            file.writelines("int main() {return 0;}")

        cmd = f'g++ -std=c++17 -Werror -pedantic -I"{installedIncludeDir}" "{temp_file}" -o /tmp/rlogic-pedantic-header.o'
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out = p.communicate()
        if p.returncode != 0:
            print(f'Header check failed for: {h}')
            print(cmd)
            print(out[0])
            print(out[1])
            numPedanticErrors += 1

    if numPedanticErrors > 0:
        print("ERROR: found errors with strict compilation in installed headers")
        return 1

    print("Done")

    return 0

if __name__ == "__main__":
    exit(main())
