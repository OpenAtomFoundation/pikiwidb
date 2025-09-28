#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DATA_DIR="$(cd "${PROJECT_ROOT}/data" && pwd)"
KEYS_FILE="${DATA_DIR}/mock/test-1M/data_0.json"
DUMP_FILE="${DATA_DIR}/mock/test-1M/dump_0.txt"
PIKA_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
cd "$PROJECT_ROOT"

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

export PATH="$JQ_DIR:$PATH"

# === 导出 DB 数据 ===
echo "=== Dumping DB with pika_to_txt ==="
"${PIKA_ROOT}/output/pika_to_txt" /home/ospp/work/pikiwidb/db/master/db0 "$DUMP_FILE"

# === 遍历 JSON 里的前 1000 个 key ===
found=0
notfound=0
total=0

while read -r k; do
  total=$((total+1))
  if grep -q "$k" "$DUMP_FILE"; then
    echo "[FOUND] $k"
    found=$((found+1))
  else
    echo "[NOTFOUND] $k"
    notfound=$((notfound+1))
  fi
done < <(jq -r '.[].key' "$KEYS_FILE" | head -1000)

# === 汇总 ===
echo "=== Summary ==="
echo "Total keys   : $total"
echo "Keys FOUND   : $found"
echo "Keys NOTFOUND: $notfound"
