#!/bin/bash

SOURCE_DIR=$1
shift
BUILD_DIR=$1
shift
TARGET_NAME=$1
shift

echo "Creating pycmc build directory in $BUILD_DIR"
mkdir $BUILD_DIR
cd $BUILD_DIR
# avoid confusion with earlier (possibly failed) builds
rm CMakeCache.txt
echo "Calling CMake for sources in $SOURCE_DIR"
cmake $SOURCE_DIR
echo "Creating target $TARGET_NAME"
make -j`grep processor /proc/cpuinfo | wc -l` "$@" $TARGET_NAME
