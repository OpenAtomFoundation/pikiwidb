#!/bin/bash

source "$(dirname "$0")/env.sh"

cd_project_root
./build.sh

cd_build_bin

# 多个 test_xxx 可执行文件
TEST_EXECUTABLES=(./test_bingest ./test_mock ./test_iagent)

# 设置默认过滤器
FILTER="*"
if [ $# -ge 1 ]; then
    FILTER="$1"
    echo "🎯 指定测试过滤器: $FILTER"
else
    echo "🔄 未指定测试过滤器，将运行所有测试（*）"
fi

for exe in "${TEST_EXECUTABLES[@]}"; do
    if [ -x "$exe" ]; then
        echo "✅ 开始运行测试: $exe --gtest_filter=$FILTER"
        $exe --gtest_filter="$FILTER"
    else
        echo "⚠️ 测试可执行文件 $exe 不存在，跳过"
    fi
done
