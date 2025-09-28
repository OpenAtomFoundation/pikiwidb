#!/usr/bin/env bash
set -euo pipefail

# -----------------------
# 路径基准：脚本所在目录 & 工程根
# -----------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PIKA_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
cd "$PROJECT_ROOT"

# -----------------------
# 自动确保 jq 存在（下载到 third/jq/jq）
# -----------------------
JQ_DIR="$PROJECT_ROOT/third/jq"
JQ_BIN="$JQ_DIR/jq"

if [ ! -x "$JQ_BIN" ]; then
  echo "⚙ jq 未找到，正在下载到 $JQ_DIR ..."
  mkdir -p "$JQ_DIR"
  if command -v curl >/dev/null 2>&1; then
    curl -L -o "$JQ_BIN" https://github.com/stedolan/jq/releases/download/jq-1.6/jq-linux64
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$JQ_BIN" https://github.com/stedolan/jq/releases/download/jq-1.6/jq-linux64
  else
    echo "❌ 需要 curl 或 wget 用于下载 jq"
    exit 1
  fi
  chmod +x "$JQ_BIN" || { echo "❌ 下载 jq 失败"; exit 1; }
  echo "✅ jq 已下载到 $JQ_BIN"
fi

# 确保优先用本地 jq
export PATH="$JQ_DIR:$PATH"

# -----------------------
# 解析参数：-d <directory_name>
# -----------------------
DIR_NAME=""
while getopts "d:" opt; do
  case $opt in
    d) DIR_NAME=$OPTARG ;;
    *) echo "Usage: $0 [-d <directory_name>]"; exit 1 ;;
  esac
done

# -----------------------
# 处理 config/dics.json
# -----------------------
DICT_FILE="$PROJECT_ROOT/config/dics.json"

if [ -f "$DICT_FILE" ]; then
  tmpfile="$(mktemp)"
  if [ -z "$DIR_NAME" ]; then
    echo "Clearing all folders in $DICT_FILE ..."
    jq '.folders = []' "$DICT_FILE" > "$tmpfile" && mv "$tmpfile" "$DICT_FILE"
    echo "All folders cleared."
  else
    echo "Removing '$DIR_NAME' from folders in $DICT_FILE ..."
    jq --arg dir "$DIR_NAME" '
      if .folders then
        .folders |= map(select(. != $dir))
      else . end
    ' "$DICT_FILE" > "$tmpfile" && mv "$tmpfile" "$DICT_FILE"
    echo "Folder '$DIR_NAME' removed."
  fi
else
  echo "JSON file $DICT_FILE does not exist. (skip)"
fi

# -----------------------
# 清理 data/* 子目录
# -----------------------
DATA_DIR="$PROJECT_ROOT/data"
mkdir -p "$DATA_DIR"

if [ -z "$DIR_NAME" ]; then
  echo "No -d provided, clearing all subdirectories in $DATA_DIR ..."
  for subdir in "$DATA_DIR"/*; do
    [ -d "$subdir" ] || continue
    echo "Deleting $subdir ..."
    rm -rf "$subdir"
  done
  echo "Done clearing all $DATA_DIR subdirectories."
else
  echo "Deleting all folders named '$DIR_NAME' inside $DATA_DIR/*/ ..."
  MATCHED=0
  shopt -s nullglob
  for subdir in "$DATA_DIR"/*/"$DIR_NAME"; do
    if [ -d "$subdir" ]; then
      echo "Deleting $subdir ..."
      rm -rf "$subdir"
      MATCHED=1
    fi
  done
  shopt -u nullglob
  if [ $MATCHED -eq 0 ]; then
    echo "No matching directories named '$DIR_NAME' found inside $DATA_DIR/*/"
  fi
fi

# -----------------------
# 清空（或创建）关键文件
# -----------------------
file1="$PROJECT_ROOT/config/manifest.offset"
file2="$PROJECT_ROOT/config/manifest.queue"

for f in "$file1" "$file2"; do
  if [ -f "$f" ]; then
    : > "$f"
    echo "$f has been cleared."
  else
    echo "$f does not exist. (skip)"
  fi
done

# -----------------------
# 清理根目录下的 log/ 与 db/（整个目录删除）
# -----------------------
ROOT_LOG_DIR="$PIKA_ROOT/log"
ROOT_DB_DIR="$PIKA_ROOT/db"
rm -rf "$ROOT_LOG_DIR" "$ROOT_DB_DIR"
echo "Removed $ROOT_LOG_DIR and $ROOT_DB_DIR if existed."

# -----------------------
# 清理 conf 下的 ingest.conf.bak.*
# -----------------------
CONF_DIR="$PROJECT_ROOT/conf"
if [ -d "$CONF_DIR" ]; then
  echo "Cleaning old ingest.conf.bak.* files in $CONF_DIR ..."
  find "$CONF_DIR" -maxdepth 1 -type f -name "ingest.conf.bak.*" -exec rm -f {} +
  echo "Done cleaning ingest.conf.bak.* files."
else
  echo "Directory $CONF_DIR not found, skipping."
fi

echo "✅ 全部清理完成"
