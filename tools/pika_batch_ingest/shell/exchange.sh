#!/bin/bash

# =================== 默认配置 ===================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

DEFAULT_KV="${SCRIPT_DIR}/data/mock/kvdict/data_1.json"
DEFAULT_SST="${SCRIPT_DIR}/data/sst/kvdict/data_1.sst"
DEFAULT_DICT="kvdict"

# =================== 变量初始化 ===================

kv=""
sst=""
dict=""
USE_DEFAULT_SINGLE=false

# =================== 手动解析 --default-single 和 --help ===================

for arg in "$@"; do
  if [[ "$arg" == "--default-single" ]]; then
    USE_DEFAULT_SINGLE=true
    # 从参数列表中移除这个标志，避免 getopts 误报
    set -- "${@/--default-single/}"
  elif [[ "$arg" == "--help" ]]; then
    echo "用法："
    echo "  $0 -k <kvPath> -s <sstPath>             # 单文件模式"
    echo "  $0 -d <jsonDirectory>                   # 多线程目录模式"
    echo "  $0                                      # 默认进入批处理模式"
    echo "  $0 --default-single                     # 默认进入单文件模式"
    exit 0
  fi
done

# =================== 解析参数 ===================

while getopts ":k:s:d:" opt; do
  case $opt in
    k) kv="$OPTARG" ;;
    s) sst="$OPTARG" ;;
    d) dict="$OPTARG" ;;
    \?)
      echo "无效选项: -$OPTARG" >&2
      exit 1
      ;;
    :)
      echo "选项 -$OPTARG 需要参数值." >&2
      exit 1
      ;;
  esac
done

# =================== 冲突检查与默认行为 ===================

if [[ -n "$dict" && ( -n "$kv" || -n "$sst" ) ]]; then
  echo "错误：不能同时使用 -d 和 -k/-s 参数." >&2
  exit 1
fi

# 用户指定了参数：单文件
if [[ -n "$kv" && -n "$sst" ]]; then
  echo "[单文件模式]"
  echo "使用 kv : $kv"
  echo "使用 sst: $sst"
  mode="single"

# 用户指定了参数：目录
elif [[ -n "$dict" ]]; then
  echo "[多线程目录模式]"
  echo "使用目录: $dict"
  mode="batch"

# 用户未传任何参数：根据 USE_DEFAULT_SINGLE 决定
elif [[ -z "$kv" && -z "$sst" && -z "$dict" ]]; then
  if [[ "$USE_DEFAULT_SINGLE" == "true" ]]; then
    echo "[使用默认单文件模式]"
    kv="$DEFAULT_KV"
    sst="$DEFAULT_SST"
    echo "使用默认 kv : $kv"
    echo "使用默认 sst: $sst"
    mode="single"
  else
    echo "[使用默认多线程目录模式]"
    dict="$DEFAULT_DICT"
    echo "使用默认目录: $dict"
    mode="batch"
  fi

else
  echo "参数组合不合法，请使用 -k 与 -s，或 -d，或 --help 查看用法。" >&2
  exit 1
fi

# =================== 构建与执行 ===================

# ./build.sh || { echo "构建失败"; exit 1; }
source "$(dirname "$0")/env.sh"
cd_build_bin

start_time=$(date +%s%3N)

if [[ "$mode" == "single" ]]; then
  ./exchange -k "$kv" -s "$sst"
elif [[ "$mode" == "batch" ]]; then
  ./exchange -d "$dict"
fi


end_time=$(date +%s%3N)
elapsed=$((end_time - start_time))  # 毫秒总数

# 转换
hours=$((elapsed / 3600000))
minutes=$(((elapsed % 3600000) / 60000))
seconds=$(((elapsed % 60000) / 1000))
millis=$((elapsed % 1000))

echo "[EXCHANGE] 执行耗时: ${hours}小时 ${minutes}分钟 ${seconds}秒 ${millis}毫秒"