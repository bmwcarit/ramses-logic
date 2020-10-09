#  -------------------------------------------------------------------------
#  Copyright (C) 2020 BMW AG
#  -------------------------------------------------------------------------
#  This Source Code Form is subject to the terms of the Mozilla Public
#  License, v. 2.0. If a copy of the MPL was not distributed with this
#  file, You can obtain one at https://mozilla.org/MPL/2.0/.
#  -------------------------------------------------------------------------

TARGETS="L64_GCC L64_GCCv7 L64_LLVM TEST_COVERAGE DOC_PACKAGE CLANG_TIDY THREAD_SANITIZER ADDRESS_SANITIZER UB_SANITIZER L64_GCC_LTO ANDROID_LIB_x86 ANDROID_LIB_x86_64 ANDROID_LIB_arm64-v8a ANDROID_LIB_armeabi-v7a L64_LLVM_COVERAGE"

# preset defaults
CONFIG=Release
BUILD_PACKAGE=False
RUN_TESTS=False

if [ $# -eq 4 ]; then
    TARGET=$1
    CONFIG=$2
    BUILD_PACKAGE=$3
    RUN_TESTS=$4
elif [ $# -eq 3 ]; then
    TARGET=$1
    CONFIG=$2
    BUILD_PACKAGE=$3
elif [ $# -eq 2 ]; then
    TARGET=$1
    CONFIG=$2
elif [ $# -eq 1 ]; then
    TARGET=$1
else
    echo "Usage: $0 <target> [config (default Release)] [Build package (default False)] [Run unittests (default False)]"
    echo ""
    echo "target             one of: $TARGETS"
    echo "config             one of: Debug Release (or any other supported CMake build type)"
    echo "build package      one of: True False"
    echo "run unit tests     one of: True False"
    echo ""
    exit 0
fi

if [ "$BUILD_DIR" = "" ]; then
    echo "$0 error: ENV[BUILD_DIR] is undefined!"
    exit -1
fi

# TODO Violin check that package name is set, if package is supposed to be built!

SCRIPT_DIR=$( cd "$( dirname $(realpath "${BASH_SOURCE[0]}") )" && pwd )
export RL_SRC=$(realpath "${SCRIPT_DIR}/../../..")

case $TARGET in
    L64_GCC | L64_GCCv7)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64.toolchain
             ;;
    L64_GCC_LTO)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64.toolchain
             ;;
    L64_LLVM|L64_LLVM_COVERAGE)
            TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm.toolchain
            ;;
    L64_GCC_CLIENT_ONLY)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64.toolchain
             ;;
    TEST_COVERAGE)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm.toolchain
             CONFIG=Debug
             ;;
    DOC_PACKAGE)
             ;;
    CLANG_TIDY)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm.toolchain
             CONFIG=Debug
             ;;
    THREAD_SANITIZER)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm_sanitize_thread.toolchain
             ;;
    ADDRESS_SANITIZER)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm_sanitize_address.toolchain
             ;;
    UB_SANITIZER)
             TOOLCHAIN=$RL_SRC/cmake/toolchain/Linux_X86_64_llvm_sanitize_ub.toolchain
             ;;
    ANDROID_LIB_x86 | ANDROID_LIB_x86_64 | ANDROID_LIB_arm64-v8a | ANDROID_LIB_armeabi-v7a)
             ;;
    *)
             echo "$0 error: unknown target '$TARGET', valid targets = $TARGETS"
             exit -1
esac

set -e

BUILD_DIR=$BUILD_DIR/$TARGET-$CONFIG
INSTALL_DIR=$BUILD_DIR/install
INSTALL_CHECK_DIR=$BUILD_DIR/install-check
mkdir -p $BUILD_DIR &> /dev/null
mkdir -p $BUILD_DIR/install &> /dev/null

pushd $BUILD_DIR

CMAKE_WARNINGS="-Wdev -Werror=dev -Wdeprecated -Werror=deprecated"

# TODO Violin/Tobias this needs more cleanup!

if [ "$TARGET" = "ANDROID_LIB_x86" ] || [ "$TARGET" = "ANDROID_LIB_x86_64" ] || [ "$TARGET" = "ANDROID_LIB_arm64-v8a" ] || [ "$TARGET" = "ANDROID_LIB_armeabi-v7a" ]; then
    abi=$(echo $TARGET | cut -d"_" -f 3-)
    echo "Building android lib with abi $abi"
    cmake \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DCMAKE_BUILD_TYPE=$CONFIG \
        -DCMAKE_TOOLCHAIN_FILE=/opt/Android/Sdk/ndk-bundle/build/cmake/android.toolchain.cmake \
        # Can't generate code on android
        -Dramses-logic_ENABLE_FLATBUFFERS_GENERATION=OFF \
        -DANDROID_PLATFORM=18 \
        -DANDROID_ABI=$abi \
        -DCPACK_PACKAGE_NAME=${PACKAGE_NAME} \
        -G Ninja \
        $CMAKE_WARNINGS \
        $RL_SRC

    cmake --build $BUILD_DIR --config $CONFIG --target install

    if [ "$BUILD_PACKAGE" = True ]; then
        cmake --build $BUILD_DIR --config $CONFIG --target package
        cp $BUILD_DIR/*.tar.gz $PACKAGE_DIR
    fi

elif [ "$TARGET" = "DOC_PACKAGE" ]; then
    cmake \
        -G"Ninja"                       \
        -Dramses-logic_BUILD_DOCS=ON    \
        $RL_SRC

    ninja rlogic-sphinx

    # Obtain project version
    pushd $RL_SRC
    VERSION_MAJOR=`cat CMakeLists.txt | grep "set(RLOGIC_VERSION_MAJOR" | sed 's/^[^0-9]*\([0-9]*\).*$/\1/'`
    VERSION_MINOR=`cat CMakeLists.txt | grep "set(RLOGIC_VERSION_MINOR" | sed 's/^[^0-9]*\([0-9]*\).*$/\1/'`
    VERSION_PATCH=`cat CMakeLists.txt | grep "set(RLOGIC_VERSION_PATCH" | sed 's/^[^0-9]*\([0-9]*\).*$/\1/'`
    GIT_COMMIT_HASH=`git rev-parse --short HEAD`
    popd

    VERSION_NAME="$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH-sha-$GIT_COMMIT_HASH"

    pushd $BUILD_DIR/sphinx
    DOC_PACKAGE_NAME=RAMSES_Logic_docs_$VERSION_NAME.tar.gz
    tar cfz $PACKAGE_DIR/$DOC_PACKAGE_NAME *
    popd

elif [ "$TARGET" = "CLANG_TIDY" ]; then

    cmake \
     -DCMAKE_BUILD_TYPE=$CONFIG \
     -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
     -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/install \
     -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
     -G Ninja \
     $CMAKE_WARNINGS \
     $RL_SRC

    pushd $RL_SRC
    $RL_SRC/ci/docker/runtime-files/clang-tidy-wrapper.py $BUILD_DIR/compile_commands.json
    popd

elif [ "$TARGET" = "TEST_COVERAGE" ]; then

    cmake \
     -DCMAKE_BUILD_TYPE=$CONFIG \
     -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
     -DCMAKE_INSTALL_PREFIX=$BUILD_DIR/install \
     -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
     -Dramses-logic_ENABLE_FLATBUFFERS_GENERATION=OFF \
     -Dramses-logic_ENABLE_TEST_COVERAGE=ON \
     -G Ninja \
     $CMAKE_WARNINGS \
     $RL_SRC

    ninja

    # ensure no old results
    rm -rf $BUILD_DIR/bin/*.profraw

    # run all ctest tests
    ctest --os test -V

    # create reports for unittests and all
    $RL_SRC/ci/docker/runtime-files/collect-coverage.py \
        --prof-dir $BUILD_DIR/bin \
        --report-dir $BUILD_DIR/test_coverage \
        --export-summary $BUILD_DIR/test_coverage/coverage-summary.json

    # archive reports
    cd $BUILD_DIR/test_coverage
    tar czf /home/rl-build/build/TEST_COVERAGE-Debug/coverage.tar.gz *

    # copy summaries when should package
    if [ "$BUILD_PACKAGE" = True ]; then
       cp coverage-summary.json $PACKAGE_DIR
    fi

elif [ "$TARGET" == "THREAD_SANITIZER" ] || [ "$TARGET" == "ADDRESS_SANITIZER" ] || [ "$TARGET" == "UB_SANITIZER" ]; then
    cmake \
        -DCMAKE_BUILD_TYPE=$CONFIG \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
        -G Ninja \
        $CMAKE_WARNINGS \
        $RL_SRC

    cmake --build $BUILD_DIR --config $CONFIG --target all
    export UBSAN_OPTIONS=report_error_type=1:symbolize=1:print_stacktrace=1:suppressions=$RL_SRC/ci/scripts/config/ubsan_suppressions
    export ASAN_OPTIONS=symbolize=1:alloc_dealloc_mismatch=0
    export LSAN_OPTIONS=symbolize=1:print_stacktrace=1
    ctest --os test -V

else
    # enable LTO build when requested by target type
    if [ "$TARGET" = "L64_GCC_LTO" ]; then
        ENABLE_LTO=ON
    else
        ENABLE_LTO=OFF
    fi

    # enable docs & flatbuffers generation, but only in one build to save build resources
    if [ "$TARGET" = "L64_LLVM" ]; then
        GEN_FLATBUFFERS=ON
        BUILD_DOCS=ON
    else
        GEN_FLATBUFFERS=OFF
        BUILD_DOCS=OFF
    fi

    # enable coverage build when requested by target type
    if [ "$TARGET" = "L64_LLVM_COVERAGE" ]; then
        ENABLE_COVERAGE=ON
    else
        ENABLE_COVERAGE=OFF
    fi

    # Test with GCC7 to make sure the project works out of the box on Ubuntu 18.04 (current LTS)
    if [ "$TARGET" = "L64_GCCv7" ]; then
        export CC=/usr/bin/gcc-7
        export CXX=/usr/bin/g++-7
    fi

    cmake \
        -DCMAKE_BUILD_TYPE=$CONFIG \
        -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
        -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        -Dramses-logic_BUILD_DOCS=$BUILD_DOCS \
        -Dramses-logic_BUILD_WITH_LTO=${ENABLE_LTO} \
        -Dramses-logic_ENABLE_FLATBUFFERS_GENERATION=${GEN_FLATBUFFERS} \
        -Dramses-logic_ENABLE_TEST_COVERAGE=${ENABLE_COVERAGE} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
        -DCPACK_PACKAGE_NAME=${PACKAGE_NAME} \
        -G Ninja \
        $CMAKE_WARNINGS \
        $RL_SRC

    cmake --build $BUILD_DIR --config $CONFIG --target install

    if [ "$BUILD_PACKAGE" = True ]; then
        cmake --build $BUILD_DIR --config $CONFIG --target package
        cp $BUILD_DIR/*.tar.gz $PACKAGE_DIR
    fi

    # Check that generated files are consistent with checked-in ones
    # Attention: when running this locally, the source tree has to be mounted as rw
    # see start_container.sh for details how to do that
    if [ "$GEN_FLATBUFFERS" = "ON" ]; then
        ninja FlatbufCheck
    fi

    if [ "$BUILD_DOCS" = "ON" ]; then
        ninja rlogic-sphinx
    fi

    pushd $BUILD_DIR
    ctest --os test -V
    popd

    # process coverage information
    if [ "$TARGET" = "L64_LLVM_COVERAGE" ] && [ -n "${GITHUB_PR}" ] && [ -n "${GITHUB_URL}" ]; then
        echo "Generate PR coverage"
        $RL_SRC/ci/docker/runtime-files/collect-coverage.py --prof-dir $BUILD_DIR/bin --export-file $BUILD_DIR/coverage.json
        $RL_SRC/ci/scripts/annotate-coverage.py \
            --coverage $BUILD_DIR/coverage.json \
            --github-url ${GITHUB_URL} \
            --repo-owner ramses \
            --repo-name ramses-logic \
            --pr ${GITHUB_PR} \
            --html-output $BUILD_DIR/pr-coverage.html \
            --post-log-link-in-issue "${ZUUL_LOG_URL}"
    fi

    # Perform install checks
    # They are quick, so we execute them always
    echo "Checking installed headers"
    python3 $RL_SRC/ci/scripts/installation-check/check-installation.py --install-dir $INSTALL_DIR/ --src-dir $RL_SRC/
    echo "Building against shared library"
    bash $RL_SRC/ci/scripts/installation-check/check-build-with-install-shared-lib.sh $BUILD_DIR/install-check/ $INSTALL_DIR
    echo "Building against ramses logic as a source tree (submodule) and linking statically"
    bash $RL_SRC/ci/scripts/installation-check/check-build-with-submodule.sh $BUILD_DIR/build-with-submodule/ $RL_SRC $INSTALL_DIR/

fi

if [ "$BUILD_PACKAGE" = True ] && [ "$TARGET" != "DOC_PACKAGE" ]; then
     cp $BUILD_DIR/*.tar.gz $PACKAGE_DIR
fi

popd
