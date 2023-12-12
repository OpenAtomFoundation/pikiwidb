#!/bin/bash

BUILD_TIME=$(git log -1 --format=%ai)
BUILD_TIME=${BUILD_TIME: 0: 10}

COMMIT_ID=$(git rev-parse HEAD)
SHORT_COMMIT_ID=${COMMIT_ID: 0: 8}

BUILD_TYPE=release
VERBOSE=0
CMAKE_FLAGS=""
MAKE_FLAGS=""
PREFIX="build"

if [ -z "$SHORT_COMMIT_ID" ]; then
    echo "no git commit id"
    SHORT_COMMIT_ID="pikiwidb"
fi

echo "BUILD_TIME:" $BUILD_TIME
echo "COMMIT_ID:" $SHORT_COMMIT_ID

echo "BUILD_TYPE:" $BUILD_TYPE
echo "CMAKE_FLAGS:" $CMAKE_FLAGS
echo "MAKE_FLAGS:" $MAKE_FLAGS

cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TIME=$BUILD_TIME -DGIT_COMMIT_ID=$SHORT_COMMIT_ID ${CMAKE_FLAGS} -S . -B ${PREFIX}
