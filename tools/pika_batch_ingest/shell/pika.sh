#!/usr/bin/env bash
set -euo pipefail

########################################
# 参数区（按需修改）
########################################
# 每次检查系统/进程的间隔秒数（通用，kill 等操作后轮询）
PROCESS_CHECK_INTERVAL=1  

# 每次检查 Ingest 状态的间隔秒数（用于 wait_for_ingest_done 循环中）
INGEST_CHECK_INTERVAL=1   

# 每次检查 Compaction 状态的间隔秒数（用于 wait_for_compaction_quiet 或 RocksDB Idle 检测）
COMPACTION_CHECK_INTERVAL=1  

# 启动主从节点或 iagent 后，等待进程稳定启动的秒数
STARTUP_WAIT_TIME=3  

# iagent 最大等待超时时间（秒）；超过此时间视为 iagent 未完成
IAGENT_WAIT_TIMEOUT=120  

# Ingest 最大等待超时时间（秒）；超过此时间视为 ingest 未完成
INGEST_WAIT_TIMEOUT=120  

# Compaction 最大等待超时时间（秒）；超过此时间视为 compaction 未完成，为0即为无限等待模式
COMPACTION_WAIT_TIMEOUT=0 

# 用于日志或 RocksDB 状态检测的“静默时间”（秒）
# ——连续这个时间内无新增活动视为安静（用于 wait_for_compaction_quiet）
COMPACTION_QUIET_TIME=3  

# 日志稳定性检测参数（秒）：连续这个时间内日志大小无变化视为稳定
LOG_STABLE_QUIET_TIME=5

# 主节点服务端口（pika master 的监听端口）
MASTER_PORT=9221  

# 从节点服务端口（pika slave 的监听端口）
SLAVE_PORT=9231  

########################################
# 函数定义
########################################
abs_path() {
  local p="$1"
  [[ -n "${p:-}" ]] || { echo ""; return 1; }

  if command -v realpath >/dev/null 2>&1; then
    local out
    if out=$(realpath -m -- "$p" 2>/dev/null); then
      printf '%s\n' "$out"; return 0
    elif out=$(realpath -- "$p" 2>/dev/null); then
      printf '%s\n' "$out"; return 0
    fi
  fi

  [[ "$p" = /* ]] || p="$PWD/$p"
  while [[ "$p" == *'//'*
  ]]; do p="${p//\/\//\/}"; done

  local IFS='/'
  local -a parts=() stack=()
  read -r -a parts <<< "$p"

  for comp in "${parts[@]}"; do
    case "$comp" in
      ''|'.')  ;;                  
      '..')    [[ ${#stack[@]} -gt 0 ]] && unset 'stack[${#stack[@]}-1]' ;;
      *)       stack+=("$comp") ;;
    esac
  done

  local out="/"
  if (( ${#stack[@]} )); then
    out="/${stack[*]// /\/}"
  fi
  printf '%s\n' "$out"
}

# 从 conf 中抽取 log-path 与 db-path，并对主/从保存为全局变量
ensure_config_paths_exist() {
  local conf_path="$1"
  local role_name="$2"

  if [ ! -f "$conf_path" ]; then
    echo "[ERROR] 配置文件 $conf_path 不存在，无法启动 $role_name"
    exit 1
  fi

  local log_path db_path
  log_path=$(grep -E "^\s*log-path" "$conf_path" | sed -E 's/.*[:=]\s*//' | xargs || true)
  db_path=$(grep -E "^\s*db-path"  "$conf_path" | sed -E 's/.*[:=]\s*//' | xargs || true)

  if [ -z "${log_path:-}" ] || [ -z "${db_path:-}" ]; then
    echo "[ERROR] $role_name 的 log-path 或 db-path 没有正确配置"
    exit 1
  fi

  # 保存到全局，供 ldb 使用
  if [[ "$role_name" == "主节点" ]]; then
    MASTER_DB_PATH="$db_path"
    MASTER_LOG_PATH="$log_path"
  else
    SLAVE_DB_PATH="$db_path"
    SLAVE_LOG_PATH="$log_path"
  fi

  echo "[SUCCESS] $role_name 目录检查完成：log=$log_path db=$db_path"
}

# 计算复制端口：服务端口 + 2000
repl_port() { echo $(( $1 + 2000 )); }

kill_port() {
  local port="$1"
  local pids=""
  if command -v lsof >/dev/null 2>&1; then
    pids=$(lsof -ti tcp:"$port" 2>/dev/null || true)
  elif command -v ss >/dev/null 2>&1; then
    pids=$(ss -ltnp 2>/dev/null | awk -v p=":$port" '$4 ~ p {print $6}' | sed -n 's/.*pid=\([0-9]\+\).*/\1/p' | sort -u)
  else
    pids=$(netstat -tunlp 2>/dev/null | awk -v p=":$port" '$4 ~ p {print $7}' | cut -d/ -f1 | sort -u)
  fi
  [ -n "$pids" ] && kill -9 $pids 2>/dev/null || true
}

# 等待端口真正空闲（最多 N 次轮询）
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

clear_env(){
  rm -rf "${PIKA_ROOT}/log"
  mkdir -p "${PIKA_ROOT}/log/master" "${PIKA_ROOT}/log/slave"
  touch "${MASTER_LOG}" "${SLAVE_LOG}"

  local ports=()
  ports+=("$MASTER_PORT" "$(repl_port "$MASTER_PORT")")
  ports+=("$SLAVE_PORT"  "$(repl_port "$SLAVE_PORT")")

  for P in "${ports[@]}"; do
    kill_port "$P"
    if wait_port_free "$P" 10; then
      echo "[SUCCESS] Port $P freed."
    else
      echo "[WARN] Port $P still appears busy; please check manually."
    fi
  done

  echo "[0] 清理环境完成。"
}

wait_for_process_exit() {
  local pid="$1"; local timeout="${2:-60}"
  local start=$(date +%s)
  while kill -0 "$pid" 2>/dev/null; do
    local now=$(date +%s)
    if (( now - start >= timeout )); then
      echo "[WARN] 进程 $pid 未在 ${timeout}s 内退出"
      return 1
    fi
    sleep $PROCESS_CHECK_INTERVAL
  done
  echo "[SUCCESS] 进程 $pid 已自然退出"
  return 0
}

wait_for_ingest_done() {
  local role="$1"; local log_path="$2"; local timeout="${3:-120}"
  local start=$(date +%s)
  local pat_finish='Finish Ingest|Ingested .* SST files|SST Ingest successful|Do \(SST Ingest\) completed'
  while :; do
    if grep -qE "$pat_finish" "$log_path"; then
      echo "[SUCCESS] $role ingest 完成"
      return 0
    fi
    local now=$(date +%s)
    if (( now - start >= timeout )); then
      echo "[WARN] $role ingest 超时"
      return 1
    fi
    sleep $INGEST_CHECK_INTERVAL
  done
}

# 旧的“compaction 日志安静”兜底函数（可保留）
wait_for_compaction_quiet() {
  local log_path="$1"; local quiet_secs="${2:-10}"; local max_wait="${3:-180}"
  local start=$(date +%s)
  local last=$(grep -cE '[Cc]ompact|flush (start|finish)' "$log_path" 2>/dev/null || echo 0)
  local stable=0
  while :; do
    sleep $COMPACTION_CHECK_INTERVAL
    local cur=$(grep -cE '[Cc]ompact|flush (start|finish)' "$log_path" 2>/dev/null || echo 0)
    if [[ "$cur" == "$last" ]]; then
      stable=$((stable + COMPACTION_CHECK_INTERVAL))
      if (( stable >= quiet_secs )); then
        echo "[SUCCESS] $log_path 在 ${quiet_secs}s 内无新增活动"
        return 0
      fi
    else
      last=$cur; stable=0
    fi
    local now=$(date +%s)
    if (( now - start >= max_wait )); then
      echo "[WARN] $log_path compaction 未安静"
      return 1
    fi
  done
}

# RocksDB 空闲等待函数（支持无限等待和超时模式）
wait_for_rocksdb_idle() {
  # 连续空闲秒数 = COMPACTION_QUIET_TIME
  # 采样间隔 = COMPACTION_CHECK_INTERVAL
  # 超时 = ROCKSDB_IDLE_TIMEOUT（0 表示无限等待）
  local consecutive="${COMPACTION_QUIET_TIME:-10}"
  local interval="${COMPACTION_CHECK_INTERVAL:-1}"
  local timeout="${ROCKSDB_IDLE_TIMEOUT:-${COMPACTION_WAIT_TIMEOUT:-180}}"

  local start_ts now master_ok slave_ok
  start_ts="$(date +%s)"

  # 预检查辅助脚本是否存在
  if [[ ! -x "./shell/rocksdb_idle_check.sh" ]]; then
    echo "[ERROR] 缺少辅助脚本: ./shell/rocksdb_idle_check.sh（请 chmod +x）"
    return 2
  fi

  while :; do
    # --- master ---
    if [[ -x "$LDB_BIN" && -n "${MASTER_DB_PATH:-}" ]]; then
      ./shell/rocksdb_idle_check.sh --db "$MASTER_DB_PATH" --ldb "$LDB_BIN" \
        --consecutive "$consecutive" --interval "$interval" >/dev/null
      master_ok=$?
    else
      ./shell/rocksdb_idle_check.sh --port "$MASTER_PORT" \
        --consecutive "$consecutive" --interval "$interval" >/dev/null
      master_ok=$?
    fi

    # --- slave ---
    if [[ -x "$LDB_BIN" && -n "${SLAVE_DB_PATH:-}" ]]; then
      ./shell/rocksdb_idle_check.sh --db "$SLAVE_DB_PATH" --ldb "$LDB_BIN" \
        --consecutive "$consecutive" --interval "$interval" >/dev/null
      slave_ok=$?
    else
      ./shell/rocksdb_idle_check.sh --port "$SLAVE_PORT" \
        --consecutive "$consecutive" --interval "$interval" >/dev/null
      slave_ok=$?
    fi

    if [[ $master_ok -eq 0 && $slave_ok -eq 0 ]]; then
      echo "[SUCCESS] RocksDB(master/slave) 均已连续 ${consecutive}s 空闲"
      return 0
    fi

    now="$(date +%s)"
    if (( timeout > 0 && now - start_ts >= timeout )); then
      echo "[WARN] 等待 RocksDB 空闲超时 ${timeout}s（master_ok=$master_ok slave_ok=$slave_ok）"
      return 1
    fi

    sleep "$interval"
  done
}

# 旧“日志大小静默”兜底
wait_for_log_stable() {
  local log_path="$1"; local quiet_secs="${2:-30}"
  local last_size=$(wc -c < "$log_path" 2>/dev/null || echo 0)
  local stable=0
  while :; do
    sleep 2
    local cur_size=$(wc -c < "$log_path" 2>/dev/null || echo 0)
    if [[ "$cur_size" == "$last_size" ]]; then
      stable=$((stable+2))
      if (( stable >= quiet_secs )); then
        echo "[SUCCESS] $log_path 在 ${quiet_secs}s 内无增长"
        return 0
      fi
    else
      last_size=$cur_size; stable=0
    fi
  done
}

wait_for_iagent_send_done() {
  local log_path="$1"   # iagent 日志路径
  local wait_secs="$2"  # 最大等待秒数

  # 保证是数字
  local last_send
  last_send=$(grep -c "Send success" "$log_path" 2>/dev/null || echo 0)
  [[ "$last_send" =~ ^[0-9]+$ ]] || last_send=0

  for ((i=1; i<=wait_secs; i++)); do
    sleep 1
    local cur_send
    cur_send=$(grep -c "Send success" "$log_path" 2>/dev/null || echo 0)
    [[ "$cur_send" =~ ^[0-9]+$ ]] || cur_send=0

    if (( cur_send == last_send )); then
      echo "[SUCCESS] iagent 发送完成"
      return 0
    else
      last_send=$cur_send
    fi
  done

  echo "[FAIL] iagent 在 ${wait_secs}s 内仍有发送活动"
  return 1
}


########################################
# 主逻辑
########################################
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PIKA_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
LOG_ROOT="$PROJECT_ROOT/data/klog"
mkdir -p "$LOG_ROOT"

MASTER_CONF="$PROJECT_ROOT/conf/pika_master.conf"
SLAVE_CONF="$PROJECT_ROOT/conf/pika_slave.conf"
MASTER_LOG="$PIKA_ROOT/log/master/pika.INFO"
SLAVE_LOG="$PIKA_ROOT/log/slave/pika.INFO"
MASTER_RE_LOG="$LOG_ROOT/master.log"
SLAVE_RE_LOG="$LOG_ROOT/slave.log"
IAGENT_LOG="$LOG_ROOT/iagent.log"

: > "$MASTER_RE_LOG"; : > "$SLAVE_RE_LOG"; : > "$IAGENT_LOG"

clear_env

# 目录检查（同时拿到各自 db-path）
ensure_config_paths_exist "$MASTER_CONF" "主节点"
ensure_config_paths_exist "$SLAVE_CONF" "从节点"

# 脚本自身所在目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$SCRIPT_DIR/tune_l0.sh"

# 启动主从
echo "[1] 启动主节点... "
"$PIKA_ROOT/output/pika" -c "$MASTER_CONF" >> "$MASTER_RE_LOG" 2>&1 &
MASTER_PID=$!; sleep $STARTUP_WAIT_TIME

echo "[2] 启动从节点... "
"$PIKA_ROOT/output/pika" -c "$SLAVE_CONF" >> "$SLAVE_RE_LOG" 2>&1 &
SLAVE_PID=$!; sleep $STARTUP_WAIT_TIME

# 启动 iagent
echo "[3] 启动 iagent... "
./shell/iagent.sh >> "$IAGENT_LOG" 2>&1 &
IAGENT_PID=$!

# 检查 ingest
echo "[4] 检查 iagent 发送... "
wait_for_iagent_send_done "$IAGENT_LOG" $INGEST_WAIT_TIMEOUT
IAGENT_INGEST=$?

if [[ $IAGENT_INGEST -eq 0 ]]; then
  echo "[SUCCESS] iagent 已完成发送"
else
  echo "[FAIL] iagent 发送超时或失败"
fi

echo "[5] 等待 compaction稳定... "
wait_for_log_stable "$MASTER_RE_LOG" $LOG_STABLE_QUIET_TIME
wait_for_log_stable "$SLAVE_RE_LOG" $LOG_STABLE_QUIET_TIME

MASTER_DB_PATH="$(abs_path "$MASTER_DB_PATH")"
SLAVE_DB_PATH="$(abs_path "$SLAVE_DB_PATH")"

# 后续校验 
echo "[6] 等待 校验... "
DEBUG=1 ./shell/check_cli.sh -p $MASTER_PORT --db-path $MASTER_DB_PATH --log $MASTER_RE_LOG --mode key --size-threshold 0.8
DEBUG=1 ./shell/check_cli.sh -p $SLAVE_PORT --db-path $SLAVE_DB_PATH --log $SLAVE_RE_LOG --mode key --size-threshold 0.8

########################################
# 停止进程
########################################
[ -n "${IAGENT_PID:-}" ] && kill $IAGENT_PID 2>/dev/null || true
[ -n "${MASTER_PID:-}" ] && kill $MASTER_PID 2>/dev/null || true
[ -n "${SLAVE_PID:-}" ] && kill $SLAVE_PID 2>/dev/null || true
echo "[成功退出]"
