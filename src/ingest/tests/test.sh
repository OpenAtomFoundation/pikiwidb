#!/bin/bash
# test.sh 放在 src/ingest/tests 目录下使用

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
TEST_BIN_DIR="$ROOT_DIR/output/src/ingest/tests"

if [ ! -d "$TEST_BIN_DIR" ]; then
  echo "[FAIL] 没有找到测试可执行文件目录: $TEST_BIN_DIR"
  exit 1
fi

echo ">>> 运行 ingest 测试 (目录: $TEST_BIN_DIR)"

FAILED=0
for test_bin in "$TEST_BIN_DIR"/*_test; do
  if [ -x "$test_bin" ]; then
    echo "--------------------------------------------------"
    echo "运行: $(basename "$test_bin")"
    "$test_bin" --gtest_color=yes || FAILED=$((FAILED+1))
  fi
done

if [ $FAILED -eq 0 ]; then
  echo "[SUCCESS] 所有测试通过"
else
  echo "[FAIL] 有 $FAILED 个测试失败"
  exit 1
fi
