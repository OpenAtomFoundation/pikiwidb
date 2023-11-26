#!/bin/bash

#color code
C_RED="\033[31m"
C_GREEN="\033[32m"

C_END="\033[0m"

BUILD_TIME=$(git log -1 --format=%ai)
BUILD_TIME=${BUILD_TIME: 0: 10}

COMMIT_ID=$(git rev-parse HEAD)
SHORT_COMMIT_ID=${COMMIT_ID: 0: 8}

if [ ! -f "/proc/cpuinfo" ];then
  CPU_CORE=$(sysctl -n hw.ncpu)
else
  CPU_CORE=$(cat /proc/cpuinfo| grep "processor"| wc -l)
fi
if [ ${CPU_CORE} -eq 0 ]; then
  CPU_CORE=1
fi

echo "cpu core ${CPU_CORE}"

if [ -z "$SHORT_COMMIT_ID" ]; then
    echo "no git commit id"
    SHORT_COMMIT_ID="pikiwidb"
fi

echo "BUILD_TIME:" $BUILD_TIME
echo "COMMIT_ID:" $SHORT_COMMIT_ID

cmake -DCMAKE_BUILD_TYPE=release -DBUILD_TIME=$BUILD_TIME -DGIT_COMMIT_ID=$SHORT_COMMIT_ID -S . -B build
cmake --build build -- -j ${CPU_CORE}

if [ $? -eq 0 ]; then
    echo -e "pika compile complete, output file ${C_GREEN} ${BUILD_DIR}/pika ${C_END}"
else
    echo -e "${C_RED} pika compile fail ${C_END}"
    exit 1
fi
