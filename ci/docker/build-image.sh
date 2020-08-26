#!/bin/bash

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

set -e

DOCKER_VAR_CONFIG_FILENAME=docker-var-config
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source $SCRIPT_DIR/$DOCKER_VAR_CONFIG_FILENAME

if [ -n "$HTTP_PROXY" ]; then
    EXTRA_BUILD_ARGS=( --build-arg HTTP_PROXY=$HTTP_PROXY --build-arg HTTPS_PROXY=$HTTPS_PROXY --build-arg http_proxy=$http_proxy --build-arg https_proxy=$https_proxy --network host )
fi

DOCKERTAG=$(uuid -v 4 | cut -d'-' -f5)
docker build ${EXTRA_BUILD_ARGS[@]} -t $DOCKER_REGISTRY/$IMAGE_NAME:$DOCKERTAG $IMAGE_FOLDER
echo $DOCKERTAG > $IMAGE_FOLDER/DOCKER_TAG
