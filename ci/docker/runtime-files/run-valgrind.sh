#!/bin/bash

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# TODO Violin this need not be its own script, move to playbooks

if [ $# -eq 1 ]; then
    BUILD_DIR=$1
else
    echo "Usage: $0 <build-dir>"
    exit 1
fi

set -e
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd $BUILD_DIR
export CONSOLE_LOGLEVEL=off

echo "running valgrind"
ctest -T memcheck -V
