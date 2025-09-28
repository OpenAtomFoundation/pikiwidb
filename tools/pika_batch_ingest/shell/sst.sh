#!/bin/bash
set -e

# -----------------------
# 路径基准：脚本所在目录 & 工程根
# -----------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DATA_DIR="${PROJECT_ROOT}/data"
PIKA_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
ROCKSDB_DIR="${PIKA_ROOT}/buildtrees/Build/rocksdb"
SST_DUMP_BIN="${ROCKSDB_DIR}/tools/sst_dump"

DEFAULT_SST_FILE="${DATA_DIR}/sst/test-1M/data_0.sst"

# -----------------------
# 帮助信息
# -----------------------
show_help() {
  echo "用法："
  echo "  $0 [相对路径.sst]              # 单文件模式（相对 data/sst）"
  echo "  $0                             # 默认读取 data/sst/test-1M/data_0.sst"
  echo "  $0 --help                      # 显示帮助"
  echo
  echo "示例："
  echo "  $0 test-1M/data_1.sst          # 显示指定文件"
  echo "  $0                             # 默认显示 data_0.sst"
}

# -----------------------
# 处理参数
# -----------------------
if [[ "$1" == "--help" ]]; then
  show_help
  exit 0
elif [[ "$1" == -* ]]; then
  echo "[ERROR] 无效参数: $1"
  show_help
  exit 1
fi

if [[ -n "$1" ]]; then
  SST_FILE="${DATA_DIR}/sst/$1"
else
  SST_FILE="${DEFAULT_SST_FILE}"
fi

# -----------------------
# 检查文件和 sst_dump
# -----------------------
if [[ ! -f "$SST_FILE" ]]; then
  echo "[ERROR] SST 文件不存在: $SST_FILE"
  exit 1
fi

if [[ ! -x "$SST_DUMP_BIN" ]]; then
  echo "[编译] sst_dump..."
  cd "${ROCKSDB_DIR}"
  make sst_dump -j$(nproc)
fi

if [[ ! -x "$SST_DUMP_BIN" ]]; then
  echo "[ERROR] sst_dump 编译失败，请检查 RocksDB 构建日志"
  exit 1
fi

# -----------------------
# 执行
# -----------------------
echo "[运行] $SST_DUMP_BIN --file=$SST_FILE --command=raw"
"$SST_DUMP_BIN" --file="$SST_FILE" --command=raw
