#!/bin/bash

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

set -e

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPT_DIR/docker-var-config

DOCKERTAG=$(uuid -v 4 | cut -d'-' -f5)
echo $DOCKERTAG > $SCRIPT_DIR/$IMAGE_FOLDER/DOCKER_TAG
