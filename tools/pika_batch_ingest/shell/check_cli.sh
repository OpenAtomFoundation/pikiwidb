#!/usr/bin/env bash
# Fixed version of check_cli.sh
# Main issues fixed:
# 1. Missing closing quote in REDIS_CLI_BIN variable
# 2. Better error handling for missing files and directories
# 3. Graceful fallback when dependencies are missing

set -uo pipefail
# set -x
trap 'ec=$?; 
      if [[ $ec -ne 0 && $ec -ne 1 && $ec -ne 2 && $ec -ne 3 ]]; then
        echo "[DEBUG] Error at line $LINENO, exit code $ec"
      fi' ERR

########################################
# 用法
########################################
usage() {
  cat <<'USAGE'
Usage: check_cli.sh [options]

Options:
  -p, --port <PORT>       Redis/Pika 端口（默认 9221 或环境变量 REDIS_PORT）
  -a, --auth <PASS>       访问密码（默认读取环境变量 REDIS_AUTH）
      --db-path <PATH>    Pika db-path（用于标题显示）
      --log <FILE>        覆盖日志路径（默认 $PROJECT_ROOT/data/klog/master.log）
      --cmd-tag <TAG>     日志命令标签（默认 ManifestIngestCmd）
      --stable-secs <N>   日志稳定性检测窗口秒数（默认 3）
      --machine           机器友好：只输出 1(通过)/0(失败)，并以 0 退出
  -h, --help              显示帮助
USAGE
}

########################################
# 路径与默认值
########################################
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

REDIS_PORT="${REDIS_PORT:-9221}"
REDIS_AUTH="${REDIS_AUTH:-}"
DB_PATH=""

LOG_FILE_DEFAULT="${PROJECT_ROOT}/data/klog/master.log"
LOG_FILE="$LOG_FILE_DEFAULT"

CMD_TAG="${CMD_TAG:-ManifestIngestCmd}"
STABLE_SECS="${STABLE_SECS:-3}"
MACHINE_ONLY=0
MODE="size"                 # 判定模式 keys|size
SIZE_THRESHOLD="0.15"       # 大小判定的阈值比例，默认 85%
QUEUE_FILE="${PROJECT_ROOT}/config/manifest.queue"
CONFIG_PATH="${PROJECT_ROOT}/config/config.json"

# FIXED: Added missing closing quote
REDIS_CLI_BIN="${REDIS_CLI_BIN:-${PROJECT_ROOT}/third/redis/src/redis-cli}"
JQ_BIN="${PROJECT_ROOT}/third/jq/jq"

RECOVERED_TIME="${RECOVERED_TIME:-10}"
########################################
# 参数解析
########################################
while [[ $# -gt 0 ]]; do
  case "$1" in
    -p|--port)        REDIS_PORT="$2"; shift 2;;
    --port=*)         REDIS_PORT="${1#*=}"; shift;;
    -a|--auth)        REDIS_AUTH="$2"; shift 2;;
    --auth=*)         REDIS_AUTH="${1#*=}"; shift;;
    --db-path)        DB_PATH="$2"; shift 2;;
    --db-path=*)      DB_PATH="${1#*=}"; shift;;
    --log)            LOG_FILE="$2"; shift 2;;
    --log=*)          LOG_FILE="${1#*=}"; shift;;
    --cmd-tag)        CMD_TAG="$2"; shift 2;;
    --cmd-tag=*)      CMD_TAG="${1#*=}"; shift;;
    --stable-secs)    STABLE_SECS="$2"; shift 2;;
    --stable-secs=*)  STABLE_SECS="${1#*=}"; shift;;
    --final-delay)    RECOVERED_TIME="$2"; shift 2;;
    --final-delay=*)  RECOVERED_TIME="${1#*=}"; shift;;
    --mode)           MODE="$2"; shift 2;;
    --mode=*)         MODE="${1#*=}"; shift;;
    --size-threshold) SIZE_THRESHOLD="$2"; shift 2;;
    --size-threshold=*) SIZE_THRESHOLD="${1#*=}"; shift;;
    --machine)        MACHINE_ONLY=1; shift;;
    -h|--help)        usage; exit 0;;
    *)                if [[ "$LOG_FILE" == "$LOG_FILE_DEFAULT" ]]; then
                        LOG_FILE="$1"; shift
                      else
                        echo "未知参数: $1" >&2; usage; exit 1
                      fi;;
  esac
done

########################################
# 颜色
########################################
if [[ -t 1 ]]; then
  GREEN=$'\033[32m'; RED=$'\033[31m'; YELLOW=$'\033[33m'; NC=$'\033[0m'
else
  GREEN=; RED=; YELLOW=; NC=
fi

hr() {
  printf '%s\n' "=========================================================================="
}

title() {
  hr
  printf " Ingest 校验摘要 (port %s, db-path %s, mode %s)\n" "$REDIS_PORT" "${DB_PATH:-auto}" "$MODE"
  hr
}


########################################
# jq 检查 - 改进版本
########################################
if [ ! -x "$JQ_BIN" ]; then
  # 尝试创建目录，但如果失败则继续
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
# redis-cli 封装
########################################
choose_redis_cli() {
  if [[ -x "$REDIS_CLI_BIN" ]]; then
    echo "$REDIS_CLI_BIN"
  elif command -v redis-cli >/dev/null 2>&1; then
    command -v redis-cli
  else
    echo ""
  fi
}
REDIS_CLI="$(choose_redis_cli)"
if [[ -z "$REDIS_CLI" ]]; then
  echo "[WARN] 未找到 redis-cli，部分检查将跳过。" >&2
fi

redis_cmd() {
  local subcmd=("$@")
  if [[ -z "$REDIS_CLI" ]]; then return 0; fi
  if [[ -n "$REDIS_AUTH" ]]; then
    base_cmd=("$REDIS_CLI" --raw --no-auth-warning -p "$REDIS_PORT" -a "$REDIS_AUTH")
  else
    base_cmd=("$REDIS_CLI" --raw --no-auth-warning -p "$REDIS_PORT")
  fi
  "${base_cmd[@]}" "${subcmd[@]}" 2>&1
}

########################################
# 日志稳定性（可按需启用）
########################################
log_stable_check() {
  local file="$1" secs="$2"
  if [[ ! -f "$file" ]]; then
    echo "[WARN] 日志文件不存在：$file" >&2
    return 1
  fi
  local s1 s2
  s1=$(wc -c < "$file" 2>/dev/null || echo 0)
  sleep "$secs"
  s2=$(wc -c < "$file" 2>/dev/null || echo 0)
  if [[ "$s1" == "$s2" ]]; then
    echo "${GREEN}[SUCCESS]${NC} $file 在 ${secs}s 内无增长"
  else
    echo "${YELLOW}[WARN]${NC} $file 在 ${secs}s 内仍在增长（$s1 -> $s2）"
  fi
}

########################################
# 队列缺失检查
########################################
queue_files=""; queue_count=0
if [[ -f "$QUEUE_FILE" ]]; then
  queue_files="$(sort -u "$QUEUE_FILE")"
  queue_count="$(printf "%s\n" "$queue_files" | wc -l)"
fi

matched_files=""; matched_count=0
if [[ -f "$LOG_FILE" ]]; then
  matched_files="$(grep -oP 'manifest_\d+_part\d+\.proto' "$LOG_FILE" | sort -u || true)"
  matched_count="$(printf "%s\n" "$matched_files" | sed '/^$/d' | wc -l)"
fi

missing_files=""; missing_count=0
if [[ -n "${queue_files:-}" ]]; then
  missing_files=$(comm -23 <(printf "%s\n" "$queue_files") <(printf "%s\n" "$matched_files"))
  missing_count="$(printf "%s\n" "$missing_files" | sed '/^$/d' | wc -l || true)"
fi

########################################
# RocksDB INFO
########################################
TOTAL_SST=0
if [[ -n "$REDIS_CLI" ]]; then
  TOTAL_SST="$(redis_cmd INFO rocksdb | awk -F: '/total_sst_files_size/{gsub("\r","",$2); total+=$2} END{print total+0}' || echo 0)"
fi

########################################
# 抽样检查 (重试 + 多次抽样统计)
########################################
MOCK_DIR="${PROJECT_ROOT}/data/mock"
SAMPLE_TOTAL=5      # 尝试抽样次数
RETRY_MAX=0         # 每个 key 最多重试
RETRY_DELAY=0       # 重试间隔秒

# 统计与明细
SAMPLE_OK=0; SAMPLE_FAIL=0; SAMPLE_ERR=0
SAMPLE_KEYS_OK=()
SAMPLE_KEYS_FAIL=()
SAMPLE_KEYS_ERR=()
SAMPLE_KEYS_RETRY=()
SAMPLE_KEYS_RECOVERED=()

pick_and_check_sample() {
  local sample_file sample_key
  sample_file="$(find "$MOCK_DIR" -type f -name '*.json' | shuf -n 1 || true)"
  [[ -z "$sample_file" ]] && { echo "ERR::no_sample_file"; return 2; }

  # 检查 jq 是否可用
  if [[ ! -x "$JQ_BIN" ]]; then
    echo "ERR::jq_not_available"; return 2
  fi

  sample_key="$("$JQ_BIN" -r '.[].key' "$sample_file" | shuf -n 1 || true)"
  [[ -z "$sample_key" || "$sample_key" == "null" ]] && { echo "ERR::invalid_key"; return 2; }

  exists_out="$(redis_cmd EXISTS "$sample_key" | sed -E 's/[^0-9]//g' || echo 0)"
  if [[ "$exists_out" == "1" ]]; then
    redis_cmd GET "$sample_key" >/dev/null || true
    [[ "${DEBUG:-0}" -eq 1 ]] && echo "[DEBUG] found key=$sample_key at first check" >&2
    echo "OK::$sample_key"
    return 0
  else
    [[ "${DEBUG:-0}" -eq 1 ]] && echo "[DEBUG] will retry later: $sample_key" >&2
    echo "FAIL::$sample_key"
    return 3
  fi
}


if [[ -n "$REDIS_CLI" && -d "$MOCK_DIR" ]]; then
  for ((i=1; i<=SAMPLE_TOTAL; i++)); do
    output="$(pick_and_check_sample)"
    rc=$?
    key="${output#*::}"
    status="${output%%::*}"

    case "$rc" in
      0) ((SAMPLE_OK++));   SAMPLE_KEYS_OK+=("$key") ;;
      2) ((SAMPLE_ERR++));  SAMPLE_KEYS_ERR+=("$key") ;;
      3) ((SAMPLE_FAIL++)); SAMPLE_KEYS_FAIL+=("$key"); SAMPLE_KEYS_RETRY+=("$key") ;;
    esac
  done
fi


########################################
# sum 摘要函数（按 CMD_TAG 抓取首尾并计算耗时）
########################################
print_ingest_sum() {
  local log_file="$1" cmd_tag="$2" status="$3"
  # 检查日志文件是否存在
  if [[ ! -f "$log_file" ]]; then
    echo "命令   : $cmd_tag"
    echo "开始   : -"
    echo "结束   : -"
    echo "耗时   : -"
    echo "备注   : 日志文件不存在"
    return
  fi
  
  awk -v cmd_tag="$cmd_tag" -v status="$status" '
  $0 ~ "\\[" cmd_tag "\\]" {
    # 例：I20250926 08:42:54.651342 876540 pika_kv.cc:223] [ManifestIngestCmd] msg...
    if (match($0, /^I([0-9]{8})[[:space:]]+([0-9]{2}):([0-9]{2}):([0-9]{2})\.([0-9]{3,6})[[:space:]]+[0-9]+[[:space:]]+[^ ]+\] (.*)$/, m)) {
      ymd=m[1]; HH=m[2]; MM=m[3]; SS=m[4]; us=m[5]; msg=m[6];
      if (length(us)==3) us=us "000";
      yyyy=substr(ymd,1,4); mon=substr(ymd,5,2); day=substr(ymd,7,2);
      sec = mktime(sprintf("%s %s %s %s %s %s", yyyy, mon, day, HH, MM, SS));
      t_us = sec*1000000 + us + 0;
      ts_ms = sprintf("%s-%s-%s %s:%s:%s.%s", yyyy, mon, day, HH, MM, SS, substr(us,1,3));
      if (cnt==0) { first_us=t_us; first_ts=ts_ms; first_msg=msg; }
      last_us=t_us; last_ts=ts_ms; last_msg=msg; cnt++;
    }
  }
  END{
    if (cnt==0) {
      printf("命令   : %s\n开始   : -\n结束   : -\n耗时   : -\n备注   : 无匹配日志\n", cmd_tag); exit 0;
    }
    diff_us = last_us - first_us;
    printf("命令   : %s\n", cmd_tag);
    printf("开始   : %s\n", first_ts);
    printf("结束   : %s\n", last_ts);
    printf("耗时   : %.0f ms (%.3f s)\n", diff_us/1000.0, diff_us/1e6);
    printf("备注   : %s\n", last_msg);
  }
  ' "$log_file"
}

########################################
# 二次延时重查
########################################
rocksdb_metric_sum() {
  local pattern="$1"
  local value
  value="$(redis_cmd INFO rocksdb \
    | awk -F: -v pat="$pattern" '
      $1 ~ pat {
        gsub("\r","",$2);
        sum += $2
      }
      END { print sum+0 }')"
  echo "$value"
}

wait_for_rocksdb_stable() {
  # local retries="${STABLE_RETRIES:-10}"     # 默认重试 10 次
  # local delay="${STABLE_DELAY:-3}"          # 默认间隔 3 秒
  # local threshold="${PENDING_THRESHOLD:-0}" # 默认 pending 必须为 0
  # local stable_count=0 prev_sst=0

  # local start_time end_time elapsed

  # start_time=$(date +%s)

  # for ((i=1; i<=retries; i++)); do
  #   flush="$(rocksdb_metric_sum '_mem_table_flush_pending')"
  #   comp="$(rocksdb_metric_sum '_num_running_compactions')"
  #   sst="$(rocksdb_metric_sum '_total_sst_files_size')"
  #   pending="$(rocksdb_metric_sum '_estimate_pending_compaction_bytes')"

  #   # 条件统计
  #   ok_count=0
  #   total_count=4
  #   (( flush==0 )) && ((ok_count++))
  #   (( comp==0 )) && ((ok_count++))
  #   (( pending<=threshold )) && ((ok_count++))
  #   (( sst==prev_sst && sst>0 )) && ((ok_count++))

  #   [[ "${DEBUG:-0}" -eq 1 ]] && \
  #     echo "[DEBUG] [Check#$i] flush=$flush (ok? $((flush==0))) | comp=$comp (ok? $((comp==0))) | pending=$pending<=${threshold} (ok? $((pending<=threshold))) | sst=$sst prev=$prev_sst (ok? $((sst==prev_sst && sst>0))) | stable_count=$stable_count | 条件满足=$ok_count/$total_count"

  #   if (( flush==0 && comp==0 && pending<=threshold && sst==prev_sst && sst>0 )); then
  #     ((stable_count++))
  #     if (( stable_count >= 2 )); then
  #       end_time=$(date +%s)
  #       elapsed=$(( end_time - start_time ))
  #       echo "[INFO] RocksDB 状态稳定 (pending=$pending ≤ threshold=$threshold)，耗时 ${elapsed}s"
  #       return 0
  #     fi
  #   else
  #     stable_count=0
  #   fi

  #   prev_sst=$sst
  #   sleep "$delay"
  # done

  # end_time=$(date +%s)
  # elapsed=$(( end_time - start_time ))
  # echo "[WARN] RocksDB 在 ${retries} 次检查后仍未稳定 (pending=$pending threshold=$threshold)，总耗时 ${elapsed}s"
  return 1
}


[[ "${DEBUG:-0}" -eq 1 ]] && echo "[DEBUG] SAMPLE_KEYS_RETRY 数量=${#SAMPLE_KEYS_RETRY[@]}"

if (( ${#SAMPLE_KEYS_RETRY[@]} > 0 )); then
  echo "[INFO] 检测到 ${#SAMPLE_KEYS_RETRY[@]} 个 notfound keys，等待 RocksDB 稳定后再次检查..."
  
  if wait_for_rocksdb_stable; then
    echo "[INFO] 开始重查 notfound keys..."

    for key in "${SAMPLE_KEYS_RETRY[@]}"; do
      recovered=0
      for ((i=1; i<=RETRY_MAX; i++)); do
        exists_out="$(redis_cmd EXISTS "$key" | sed -E 's/[^0-9]//g' || echo 0)"
        [[ "${DEBUG:-0}" -eq 1 ]] && echo "[DEBUG] retry $i for $key -> exists_out=$exists_out" >&2

        if [[ "$exists_out" == "1" ]]; then
          echo "[RECOVERED] $key 在延时检查中出现了" >&2
          SAMPLE_OK=$((SAMPLE_OK+1))
          SAMPLE_FAIL=$((SAMPLE_FAIL-1))
          SAMPLE_KEYS_OK+=("$key")
          SAMPLE_KEYS_RECOVERED+=("$key")
          SAMPLE_KEYS_FAIL=($(printf "%s\n" "${SAMPLE_KEYS_FAIL[@]}" | grep -vx "$key"))
          recovered=1
          break
        fi

        sleep "$RETRY_DELAY"
      done
      [[ $recovered -eq 0 && "${DEBUG:-0}" -eq 1 ]] && echo "[DEBUG] key=$key still not found after ${RETRY_MAX} retries" >&2
    done
  fi
fi


########################################
# 统一判定 + 失败原因
########################################
FAIL_REASONS=""

cond_queue="OK"
if [[ -f "$QUEUE_FILE" && $queue_count -gt 0 ]]; then
  if [[ $missing_count -gt 0 ]]; then
    cond_queue="NO"
    FAIL_REASONS+="队列缺失(${missing_count} 个); "
  fi
fi

if [[ "$TOTAL_SST" == "0" ]]; then
  FAIL_REASONS+="RocksDB total_sst_files_size 异常为 0; "
fi


# 统一判定：根据 MODE 切换（keys / size）
STATUS_TXT="FAIL"; RESULT_CODE=0

if [[ "$cond_queue" != "OK" ]]; then
  FAIL_REASONS+="队列缺失检查未通过; "
fi

if [[ "$MODE" == "size" ]]; then
  # —— 按 RocksDB 大小判定（±比例区间） ——
  if [[ ! -f "$CONFIG_PATH" ]]; then
    FAIL_REASONS+="未找到配置文件: $CONFIG_PATH; "
  elif [[ ! -x "$JQ_BIN" ]]; then
    FAIL_REASONS+="jq 不可用，无法读取 targetSizeMB; "
  else
    TARGET_MB="$("$JQ_BIN" -r '.targetSizeMB // 0' "$CONFIG_PATH" 2>/dev/null || echo 0)"
    [[ "$TARGET_MB" =~ ^[0-9]+(\.[0-9]+)?$ ]] || TARGET_MB=0

    # 计算当前、上下界（MB 与 Bytes）
    CURRENT_MB=$(awk -v b="$TOTAL_SST" 'BEGIN{printf "%.3f", b/1024/1024}')
    DELTA=$(awk -v r="$SIZE_THRESHOLD" 'BEGIN{printf "%.3f", 1-r}')              # 例如 r=0.85 => Δ=0.15
    LOWER_MB=$(awk -v mb="$TARGET_MB" -v r="$SIZE_THRESHOLD" 'BEGIN{printf "%.3f", mb*r}')
    UPPER_MB=$(awk -v mb="$TARGET_MB" -v r="$SIZE_THRESHOLD" 'BEGIN{printf "%.3f", mb*(1+(1-r))}')  # = mb*(2-r)

    LOWER_BYTES=$(awk -v mb="$LOWER_MB" 'BEGIN{printf "%.0f", mb*1024*1024}')
    UPPER_BYTES=$(awk -v mb="$UPPER_MB" 'BEGIN{printf "%.0f", mb*1024*1024}')

    if (( TOTAL_SST >= LOWER_BYTES && TOTAL_SST <= UPPER_BYTES && LOWER_BYTES > 0 )); then
      STATUS_TXT="SUCCESS"; RESULT_CODE=1
      SIZE_PASS_REASON=$(
        printf '目标=%.3f MB，允许区间=[%.3f MB, %.3f MB]，当前=%.3f MB ∈ 区间' \
              "$TARGET_MB" "$LOWER_MB" "$UPPER_MB" "$CURRENT_MB"
      )
    else
      STATUS_TXT="FAIL"; RESULT_CODE=0
      if (( TOTAL_SST < LOWER_BYTES )); then
        FAIL_REASONS+=$(
          printf '当前=%.3f MB < 下限=%.3f MB（目标=%.3f MB，±%.0f%%）; ' \
                "$CURRENT_MB" "$LOWER_MB" "$TARGET_MB" "$(awk -v d="$DELTA" 'BEGIN{printf "%.0f", d*100}')"
        )
      elif (( TOTAL_SST > UPPER_BYTES )); then
        FAIL_REASONS+=$(
          printf '当前=%.3f MB > 上限=%.3f MB（目标=%.3f MB，±%.0f%%）; ' \
                "$CURRENT_MB" "$UPPER_MB" "$TARGET_MB" "$(awk -v d="$DELTA" 'BEGIN{printf "%.0f", d*100}')"
        )
      else
        FAIL_REASONS+="阈值/配置异常; "
      fi
    fi
  fi
  else
  # 抽样判断逻辑
  STATUS_TXT="FAIL"; RESULT_CODE=0
  if [[ "$cond_queue" == "OK" ]]; then
    if [[ $SAMPLE_OK -eq $SAMPLE_TOTAL ]]; then
      STATUS_TXT="SUCCESS"; RESULT_CODE=1
    elif [[ $SAMPLE_OK -gt 0 ]]; then
      STATUS_TXT="WARN"; RESULT_CODE=0
      FAIL_REASONS+="部分抽样 key 不存在(${SAMPLE_FAIL}/${SAMPLE_TOTAL}); "

      # 根据 RocksDB 状态增加可能原因
      pending_bytes="$(redis_cmd INFO rocksdb | awk -F: '/estimate_pending_compaction_bytes/{gsub("\r","",$2); sum+=$2} END{print sum+0}')"
      running_comp="$(redis_cmd INFO rocksdb | awk -F: '/num_running_compactions/{gsub("\r","",$2); sum+=$2} END{print sum+0}')"

      if (( pending_bytes > 0 || running_comp > 0 )); then
        FAIL_REASONS+="可能原因: RocksDB 存在延迟压缩 (pending=${pending_bytes}, comp=${running_comp}), 部分 key 暂时不可见; "
      fi

    elif [[ $SAMPLE_ERR -gt 0 ]]; then
      STATUS_TXT="FAIL"; RESULT_CODE=0
      FAIL_REASONS+="抽样过程中出现错误(${SAMPLE_ERR} 次); "
    else
      STATUS_TXT="FAIL"; RESULT_CODE=0
      FAIL_REASONS+="所有抽样 key 均不存在; "
    fi
  else
    FAIL_REASONS+="队列缺失检查未通过; "
  fi
fi


########################################
# 输出
########################################
if [[ $MACHINE_ONLY -eq 1 ]]; then
  echo "$RESULT_CODE"; exit 0
fi

title
# 如需启用日志稳定性检测，取消下一行注释
# log_stable_check "$LOG_FILE" "$STABLE_SECS" || true

if [[ "$STATUS_TXT" == "SUCCESS" ]]; then
  printf "状态   : %s%s%s (%d)\n" "$GREEN" "$STATUS_TXT" "$NC" "$RESULT_CODE"
  if [[ "$MODE" == "size" && -n "${SIZE_PASS_REASON:-}" ]]; then
    echo "原因   : $SIZE_PASS_REASON"
  fi
else
  printf "状态   : %s%s%s (%d)\n" "$RED" "$STATUS_TXT" "$NC" "$RESULT_CODE"
  echo "原因   : $FAIL_REASONS"
fi

# —— sum 摘要（在队列/抽样信息之前打印）
print_ingest_sum "$LOG_FILE" "$CMD_TAG" "$STATUS_TXT"

printf "队列   : 总 %s | 已处理 %s | 缺失 %s → %s\n" \
  "$queue_count" "$matched_count" "$missing_count" "$cond_queue"

if [[ "$MODE" == "key" ]]; then
    printf "抽样   : 成功 %d | 缺失 %d | 错误 %d / 总 %d\n" \
    "$SAMPLE_OK" "$SAMPLE_FAIL" "$SAMPLE_ERR" "$SAMPLE_TOTAL"
fi

printf "RocksDB: SST %s bytes\n" "$TOTAL_SST"

# —— 抽样 Key 明细（可视化）
if (( SAMPLE_OK > 0 )); then
  echo "OK Keys:"
  printf '  - %s\n' "${SAMPLE_KEYS_OK[@]}"
fi
if (( SAMPLE_FAIL > 0 )); then
  echo "NOTFOUND Keys:"
  printf '  - %s\n' "${SAMPLE_KEYS_FAIL[@]}"
fi
if (( SAMPLE_ERR > 0 )); then
  echo "ERROR Keys:"
  printf '  - %s\n' "${SAMPLE_KEYS_ERR[@]}"
fi
if (( SAMPLE_KEYS_RECOVERED > 0 )); then
  echo "RECOVERED Keys:"
  printf '  - %s\n' "${SAMPLE_KEYS_RECOVERED[@]}"
fi
hr
exit 0