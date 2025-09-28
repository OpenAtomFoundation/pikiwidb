#!/usr/bin/env bash
set -euo pipefail

########################################
# 用法
########################################
usage() {
  cat <<'USAGE'
Usage: check_keys.sh [options]

Options:
  -p, --port <PORT>      pika 启动端口 (默认 9221)
  -d, --dbpath <PATH>    pika 的 db-path (默认 $PROJECT_ROOT/data/db)
  -j, --json <DIR>       包含 keys 的 JSON 文件目录 (默认 $PROJECT_ROOT/data/mock)
  -l, --log <FILE>       pika 日志文件路径 (默认 master.log, 拼接到 $LOG_DICT)
  -h, --help             显示帮助
USAGE
}

########################################
# 默认参数
########################################
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PIKA_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"

PORT="9221"
DBPATH="$PIKA_ROOT/db/master"
JSON_DIR="$PROJECT_ROOT/data/mock"
LOG_DICT="$PROJECT_ROOT/data/tmp"
LOG_FILE="master.log"  # 默认日志文件名
JQ_BIN="${PROJECT_ROOT}/third/jq/jq"

########################################
# 参数解析
########################################
while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--port)   PORT="$2"; shift 2;;
    -d|--dbpath) DBPATH="$2"; shift 2;;
    -j|--json)   JSON_DIR="$2"; shift 2;;
    -l|--log)    LOG_FILE="$2"; shift 2;;
    -h|--help)   usage; exit 0;;
    *) echo "未知参数: $1" >&2; usage; exit 1;;
  esac
done

# 拼接日志目录和文件名
LOG_FILE="$LOG_DICT/$LOG_FILE"

########################################
# 函数定义
########################################
# 计算复制端口：服务端口 + 2000
repl_port() { echo $(( $1 + 2000 )); }

# 清理端口上的进程
clear_port() {
  local port="$1"
  local pids=""

  if command -v lsof >/dev/null 2>&1; then
    pids=$(lsof -ti tcp:"$port" 2>/dev/null || true)
  elif command -v ss >/dev/null 2>&1; then
    pids=$(ss -ltnp 2>/dev/null | awk -v p=":$port" '$4 ~ p {print $6}' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | sort -u)
  else
    pids=$(netstat -tunlp 2>/dev/null | awk -v p=":$port" '$4 ~ p {print $7}' | cut -d/ -f1 | sort -u)
  fi

  if [ -n "$pids" ]; then
    kill -9 $pids 2>/dev/null || true
    echo "[SUCCESS] 端口 $port 已释放。"
  else
    echo "[WARN] 端口 $port 没有找到对应的进程。"
  fi
}

# 等待端口空闲
wait_port_free() {
  local port="$1"; local tries="${2:-10}"
  for ((i=1;i<=tries;i++)); do
    if ! (echo > /dev/tcp/127.0.0.1/$port) >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.5
  done
  return 1
}

# 清理环境中的端口
clear_env() {
  local ports=("$PORT" "$(repl_port "$PORT")")
  for P in "${ports[@]}"; do
    clear_port "$P"
    if wait_port_free "$P" 10; then
      echo "[SUCCESS] Port $P 已释放。"
    else
      echo "[WARN] Port $P 仍在占用，请手动检查。"
    fi
  done
  echo "[INFO] 环境清理完成。"
}

########################################
# 启动 pika
########################################
clear_env

PIKA_BIN="${PIKA_ROOT}/output/pika"
REDIS_CLI_BIN="${REDIS_CLI_BIN:-${PROJECT_ROOT}/third/redis/src/redis-cli}"

if [[ ! -x "$PIKA_BIN" ]]; then
  echo "[ERROR] 未找到 pika 可执行文件: $PIKA_BIN" >&2
  exit 1
fi

mkdir -p "$DBPATH" "$(dirname "$LOG_FILE")"

echo "[INFO] 启动 pika on port $PORT ..."
"$PIKA_BIN" -c "$PROJECT_ROOT/conf/pika_master.conf" >> "$LOG_FILE" 2>&1 &
PIKA_PID=$!
sleep 3

########################################
# 查找 redis-cli
########################################
if [[ -x "$REDIS_CLI_BIN" ]]; then
  REDIS_CLI="$REDIS_CLI_BIN"
elif command -v redis-cli >/dev/null 2>&1; then
  REDIS_CLI=$(command -v redis-cli)
else
  echo "[ERROR] redis-cli 未找到" >&2
  kill $PIKA_PID || true
  exit 1
fi

########################################
# jq 检查 - 改进版本
########################################
if [ ! -x "$JQ_BIN" ]; then
  if mkdir -p "$PROJECT_ROOT/third/jq" 2>/dev/null; then
    echo "jq 未找到，正在下载到 third/jq..." >&2
    if curl -fsSL -o "$JQ_BIN" https://github.com/stedolan/jq/releases/download/jq-1.6/jq-linux64 2>/dev/null; then
      chmod +x "$JQ_BIN" || { echo "下载 jq 失败" >&2; }
    else
      echo "下载 jq 失败，跳过 jq 相关功能" >&2
    fi
  else
    echo "无法创建目录，跳过 jq 安装" >&2
  fi
fi
if [ -x "$JQ_BIN" ]; then
  PATH="$PROJECT_ROOT/third/jq:$PATH"
fi

########################################
# 校验 keys
########################################
if [[ ! -d "$JSON_DIR" ]]; then
  echo "[ERROR] JSON 目录不存在: $JSON_DIR" >&2
  kill $PIKA_PID || true
  exit 1
fi

TOTAL=0; OK=0; FAIL=0

echo "[DEBUG] 查找 JSON 文件: $JSON_DIR"
JSON_FILES=$(find "$JSON_DIR" -type f -name "*.json")
if [[ -z "$JSON_FILES" ]]; then
  echo "[ERROR] 没有找到 JSON 文件" >&2
  exit 1
fi

for JSON_FILE in $JSON_FILES; do
  echo "[INFO] 校验 JSON 文件: $JSON_FILE"
  if [[ ! -f "$JSON_FILE" ]]; then
    echo "[ERROR] JSON 文件不存在: $JSON_FILE" >&2
    kill $PIKA_PID || true
    exit 1
  fi

  # ======  关键修复：用进程替换避免子 Shell ======
  while read -r item; do
    key=$(echo "$item" | jq -r '.key')
    expect=$(echo "$item" | jq -r '.value')
    if [[ -z "$key" || -z "$expect" ]]; then
      echo "[ERROR] key 或 value 为空，跳过此项"
      continue
    fi
    echo "[DEBUG] key=$key, expect=$expect"

    val=$($REDIS_CLI -p "$PORT" --raw get "$key" || true)
    val=$(echo "$val" | xargs)
    if [[ -z "$val" ]]; then
      echo "[ERROR] 无法获取 Redis key=$key 的值" >&2
    fi

    if [[ "$val" == "$expect" ]]; then
      echo "[OK] key=$key value=$val"
      OK=$((OK + 1))
    else
      echo "[FAIL] key=$key 期望=$expect 实际=${val:-nil}"
      FAIL=$((FAIL + 1))
    fi
    TOTAL=$((TOTAL + 1))
  done < <(jq -c '.[]' "$JSON_FILE")
done

# 输出最终结果
echo "=========================================="
echo "Total=$TOTAL OK=$OK FAIL=$FAIL"

########################################
# 退出前清理
########################################
kill $PIKA_PID || true
wait $PIKA_PID 2>/dev/null || true