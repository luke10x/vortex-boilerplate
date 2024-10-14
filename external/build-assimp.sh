#!/bin/bash

TARGET=$2

set -e  # Exit immediately if a command exits with a non-zero status
set -u  # Exit script if an undefined variable is used

SRC_DIR=$1

if [ ! -d "$SRC_DIR" ]; then
    echo "There is no such SRC_DIR: ${SRC_DIR}" >&2
    exit 1
fi

if [ "$TARGET" == "WASM" ]; then

    CXXFLAGS="-s DISABLE_EXCEPTION_CATCHING=0"
    CXXFLAGS="$CXXFLAGS -s USE_ZLIB=1"
    export CXXFLAGS

    # Build zlib for Wasm as it cannot locate it otherwise on Linux
    BUILD_ZLIB=ON

    CMAKE="emcmake cmake"
    MAKE="emmake make"

elif [ -z "$TARGET" ]; then
    
    CPLUS_INCLUDE_PATH=./include
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:$(pwd)/include
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/code
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/include
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib/utf8cpp/source
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib/unzip
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib/rapidjson/include
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib/openddlparser/include
    CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:${SRC_DIR}/contrib/pugixml/src
    export CPLUS_INCLUDE_PATH

    export CXXFLAGS="-O2 -Wno-error=array-bounds"

    # Do not build zlib for Wasm as it can locate it well on native systems,
    # but it fails to build it on Linux
    BUILD_ZLIB=OFF

    CMAKE="cmake -DCMAKE_INSTALL_PREFIX=$(pwd)/../installed"
    MAKE=make

else
    echo "TARGET has to be either WASM or unset, but is: $TARGET" >&2
    exit 1
fi

$CMAKE \
    -DASSIMP_BUILD_TESTS=OFF \
    -DASSIMP_BUILD_ZLIB=$BUILD_ZLIB \
    -DASSIMP_BUILD_ASSIMP_TOOLS=OFF \
    -DASSIMP_INSTALL_PDB=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    $SRC_DIR 
$MAKE -j4
$MAKE install

