#!/bin/sh

#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

# Access shared volumes as non root user
#
# Usage: docker run -e DEV_UID=$(id -u) -e DEV_GID=$(id -g) ...
# Source: http://chapeau.freevariable.com/2014/08/docker-uid.html

groupmod -o -g ${DEV_GID} rl-build
usermod -o -u ${DEV_UID} rl-build

cat <<EOF > /tmp/usersetup

# add source'ing of ramses logic bashrc
cat <<CODE >> ~/.bashrc

if [ -f ${SRC_DIR}/ci/docker/runtime-files/bashrc ]; then
    . ${SRC_DIR}/ci/docker/runtime-files/bashrc
fi
CODE

EOF
runuser -l rl-build -c 'bash /tmp/usersetup'
rm -f /tmp/usersetup

cd /home/rl-build

if [ $# -eq 0 ]; then
    # no commands supplied
    exec gosu rl-build /bin/bash
else
    # commands supplied
    exec gosu rl-build /bin/bash -c "source /home/rl-build/.bashrc; $*"
fi

