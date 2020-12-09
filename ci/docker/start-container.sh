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

SRC_DIR=$(realpath "${SCRIPT_DIR}/../../")
DIR_POSTFIX="default"
SRC_DIR_MOUNT_MODE="rw"

if [[ $# -eq 0 ]]; then
    DIR_POSTFIX="default"
elif [[ $# -eq 1 ]]; then
    DIR_POSTFIX=$1
elif [[ $# -eq 2 ]]; then
    DIR_POSTFIX=$1
    SRC_DIR_MOUNT_MODE=$2
else
    echo "Usage: $0 <build dir postfix> [src dir mount mode (default ro)]"
    echo "       $0 -auto [src dir mount mode (default ro)]"
    echo "example: $0 x86_64-debug"
    exit
fi

SRC_ROOT=$(realpath "${SRC_DIR}")

if [ "${DIR_POSTFIX}" == "-auto" ]; then
    BASE_PATH=$(realpath -s "${HOME}/docker-storage")
    DIR_POSTFIX=${SRC_ROOT//\//-}
else
    BASE_PATH=$(realpath -s "${SRC_ROOT}/..")
    DIR_POSTFIX=-${DIR_POSTFIX}
fi

BUILD_ROOT=$(realpath -s "${BASE_PATH}/build-docker${DIR_POSTFIX}")
PACKAGE_ROOT=$(realpath -s "${BASE_PATH}/package-docker${DIR_POSTFIX}")
PERSISTENT_STORAGE=$(realpath -s "${BASE_PATH}/persistent-storage-docker${DIR_POSTFIX}")

rm -rf "${BUILD_ROOT}"
rm -rf "${PACKAGE_ROOT}"

mkdir -p "${BUILD_ROOT}"
mkdir -p "${PACKAGE_ROOT}"
mkdir -p "${PERSISTENT_STORAGE}"

echo "Build root:         $BUILD_ROOT"
echo "Package root:       $PACKAGE_ROOT"
echo "Persistent storage: $PERSISTENT_STORAGE"

# copy in user ssh keys to persistent storage
if [ -f "${HOME}/.ssh/id_rsa"  ]; then
    cp "${HOME}/.ssh/id_rsa" "${PERSISTENT_STORAGE}/.id_rsa"
fi

# optional link in tokens when existing
if [ -f ${HOME}/.github-token ]; then
    TOKEN_MIXIN_GITHUB=(-v ${HOME}/.github-token:/home/rl-build/.github-token)
fi

docker run \
        -i \
        -t \
        --privileged \
        --rm \
        --init \
        -v $SRC_ROOT:/home/rl-build/git:$SRC_DIR_MOUNT_MODE \
        -v $BUILD_ROOT:/home/rl-build/build \
        -v $PACKAGE_ROOT:/home/rl-build/package \
        -v $PERSISTENT_STORAGE:/home/rl-build/persistent-storage \
        ${TOKEN_MIXIN_GITHUB[@]} \
        -e DEV_UID=$(id -u) \
        -e DEV_GID=$(id -g) \
        $DOCKER_REGISTRY/$IMAGE_NAME:$DOCKERTAG
