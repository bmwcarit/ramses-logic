#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

FROM ubuntu:18.04

#mkdir of /usr/share/man/man1 is necessary as otherwise openjdk-8 installation fails
# using openjdk-8 as openjdk-11 is not supported by Android sdkmanager; 9 and 10 are not available
RUN if [ -n "${HTTP_PROXY}" ]; then \
        echo "set proxy to ${HTTP_PROXY}"; \
        echo -n "Acquire::http::Proxy \"${HTTP_PROXY}\";\nAcquire::https::Proxy \"${HTTP_PROXY}\";\n" > /etc/apt/apt.conf; \
    fi \
    && mkdir -p /usr/share/man/man1 \
    && apt-get update \
    && env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        bash \
        git \
        ca-certificates \
        build-essential \
        pkg-config \
        curl \
        ninja-build \
        lcov \
        sudo \
        file \
        libreadline7 \
        libc6-dbg \
        doxygen \
        python3-git \
        python3-pip \
        python3-setuptools \
        python3-wheel \
        software-properties-common \
        gnupg \
        valgrind \
        graphviz\
        gosu\
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main\ndeb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main\n" >> /etc/apt/sources.list \
    && sh -c "curl -o - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -" \
    && apt-get update \
    && env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        gcc-9 g++-9 \
        clang-12 clang-tidy-12 llvm-12\
    && apt-get purge -y software-properties-common \
    && apt-get autoremove -y \
    && apt-get autoclean \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /usr/share/man/* /usr/share/info/* /usr/share/doc/* /usr/share/lintian/* \
    && ln -s /usr/bin/clang-12 /usr/bin/clang \
    && ln -s /usr/bin/clang++-12 /usr/bin/clang++ \
    && ln -s /usr/bin/clang-tidy-12 /usr/bin/clang-tidy \
    && ln -s /usr/bin/llvm-profdata-12 /usr/bin/llvm-profdata \
    && ln -s /usr/bin/llvm-cov-12 /usr/bin/llvm-cov

# use gcc-9 by default
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 700 --slave /usr/bin/g++ g++ /usr/bin/g++-7 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 900 --slave /usr/bin/g++ g++ /usr/bin/g++-9

# install pip3 packages
COPY requirements-py3.txt /tmp/
RUN pip3 install -r /tmp/requirements-py3.txt \
    && rm -f /tmp/requirements-py3.txt

# add ramses user and groups
RUN groupadd --gid 1000 rl-build \
    && useradd --create-home --uid 1000 --gid rl-build rl-build \
    && echo "%rl-build ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# boot & setup
COPY boot.sh /boot.sh
ENTRYPOINT ["/boot.sh"]

# export configuration flags
ENV SRC_DIR=/home/rl-build/git \
    BUILD_DIR=/home/rl-build/build \
    PACKAGE_DIR=/home/rl-build/package \
    CONAN_USER_HOME=/home/rl-build/package

# define the volumes, which must be
# mounted during 'docker run' call
VOLUME /home/rl-build/git \
       /home/rl-build/build \
       /home/rl-build/package

# add link to build.sh inside docker image (for convenience)
RUN ln -s /home/rl-build/git/ci/docker/runtime-files/build.sh /home/rl-build/build.sh
