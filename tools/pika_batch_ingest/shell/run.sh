#!/usr/bin/env bash
set -euo pipefail

# =======================
# 路径与通用函数
# =======================
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$PROJECT_ROOT"

log_time() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*"
}

# =======================
# 确保 jq 可用（下载到 third/jq/jq）
# =======================
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

# =======================
# 计时
# =======================
start_ts=$(date +%s)
log_time "开始执行脚本"

# =======================
# 入参
# =======================
FOLDER_NAME="${1-}"
if [ -z "$FOLDER_NAME" ]; then
  echo "Usage: $0 <folder_name>"
  exit 1
fi

# =======================
# 调用清理脚本（寻找 shell/clear.sh 或根目录 clear.sh）
# =======================
CLEAR_SH="$PROJECT_ROOT/shell/clear.sh"
[ -x "$CLEAR_SH" ] || CLEAR_SH="$PROJECT_ROOT/clear.sh"
if [ -x "$CLEAR_SH" ]; then
  log_time "执行 $(realpath "$CLEAR_SH")"
  "$CLEAR_SH"
else
  log_time "未找到可执行的 clear.sh，跳过"
fi

# =======================
# 修改 config/config.json 中的 directory
# =======================
CONFIG_FILE="$PROJECT_ROOT/config/config.json"
if [ -f "$CONFIG_FILE" ]; then
  log_time "修改 $CONFIG_FILE 中的 directory 为 $FOLDER_NAME"
  tmpfile="$(mktemp)"
  jq --arg folder_name "$FOLDER_NAME" '.directory = $folder_name' "$CONFIG_FILE" > "$tmpfile" && mv "$tmpfile" "$CONFIG_FILE"
else
  echo "❌ 未找到 $CONFIG_FILE"
  exit 1
fi

# =======================
# 修改 S3 配置里的 dict（兼容两种命名：s3_config.json / s3config.json）
# =======================
S3CONFIG_CANDIDATES=(
  "$PROJECT_ROOT/config/s3_config.json"
  "$PROJECT_ROOT/config/s3config.json"
)
S3CONFIG_FILE=""
for f in "${S3CONFIG_CANDIDATES[@]}"; do
  if [ -f "$f" ]; then
    S3CONFIG_FILE="$f"
    break
  fi
done

if [ -n "$S3CONFIG_FILE" ]; then
  log_time "修改 $S3CONFIG_FILE 中的 dict 为 sst/$FOLDER_NAME"
  tmpfile="$(mktemp)"
  jq --arg folder_name "sst/$FOLDER_NAME" '.dict = $folder_name' "$S3CONFIG_FILE" > "$tmpfile" && mv "$tmpfile" "$S3CONFIG_FILE"
else
  echo "❌ 未找到 S3 配置文件（尝试了 config/s3_config.json 与 config/s3config.json）"
  exit 1
fi

# =======================
# 调整 L0 阈值
# =======================
TUNE_L0_SH="$PROJECT_ROOT/shell/tune_l0.sh"
if [ -x "$TUNE_L0_SH" ]; then
  log_time "执行 $(realpath "$TUNE_L0_SH")"
  "$TUNE_L0_SH"
else
  log_time "未找到或不可执行：$TUNE_L0_SH（跳过）"
fi

# =======================
# 执行 mock
# =======================
MOCK_SH="$PROJECT_ROOT/shell/mock.sh"
if [ -x "$MOCK_SH" ]; then
  log_time "执行 $(realpath "$MOCK_SH")"
  "$MOCK_SH"
else
  log_time "未找到或不可执行：$MOCK_SH（跳过）"
fi

# =======================
# 执行 exchange -d <folder>
# =======================
EXCHANGE_SH="$PROJECT_ROOT/shell/exchange.sh"
if [ -x "$EXCHANGE_SH" ]; then
  log_time "执行 $(realpath "$EXCHANGE_SH") -d $FOLDER_NAME"
  "$EXCHANGE_SH" -d "$FOLDER_NAME"
else
  log_time "未找到或不可执行：$EXCHANGE_SH（跳过）"
fi

# =======================
# 执行 s3put
# =======================
S3PUT_SH="$PROJECT_ROOT/shell/s3put.sh"
if [ -x "$S3PUT_SH" ]; then
  log_time "执行 $(realpath "$S3PUT_SH")"
  "$S3PUT_SH"
else
  log_time "未找到或不可执行：$S3PUT_SH（跳过）"
fi

# =======================
# 执行 pika（ms_pika.sh）
# =======================
PIKA_SH="$PROJECT_ROOT/shell/pika.sh"
if [ -x "$PIKA_SH" ]; then
  log_time "执行 $(realpath "$PIKA_SH")"
  "$PIKA_SH"
else
  log_time "未找到或不可执行：$PIKA_SH（跳过）"
fi

# =======================
# 结束
# =======================
end_ts=$(date +%s)
elapsed=$(( end_ts - start_ts ))
log_time "脚本执行完成，总耗时 ${elapsed} 秒"
