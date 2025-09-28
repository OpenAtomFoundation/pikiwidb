#!/usr/bin/env bash
# shell/tune_l0_thresholds.sh
# Auto-tune L0 thresholds for ingest.aggr based on targetSizeMB & maxFileSizeMB.
# Default policy = balanced (ratio 1:3:6). Requires jq, otherwise falls back to python.

set -euo pipefail

# --- Paths (relative to repo root; script is under shell/) ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
PIKA_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
CONFIG_JSON="${REPO_ROOT}/config/config.json"
INGEST_CONF="${REPO_ROOT}/conf/ingest.conf"

# --- Tuning knobs via env ---
POLICY="${POLICY:-balanced}"        # early | balanced | heavy
SLOWDOWN_FACTOR="${SLOWDOWN_FACTOR:-3}"
STOP_FACTOR="${STOP_FACTOR:-6}"
DRY_RUN="${DRY_RUN:-0}"

# --- Helpers ---
need() { command -v "$1" >/dev/null 2>&1; }
round_to_nearest_10() { local n="$1"; echo $(( ((n + 5) / 10) * 10 )); }
clamp() { local v="$1" lo="$2" hi="$3"; (( v < lo )) && v="$lo"; (( v > hi )) && v="$hi"; echo "$v"; }

read_json_key() {
  local key="$1"
  if need jq; then
    jq -r --arg k "$key" '.[$k]' "$CONFIG_JSON"
  else
    # Fallback to python if jq is not available
    python3 - "$CONFIG_JSON" "$key" <<'PY'
import json,sys
p, k = sys.argv[1], sys.argv[2]
with open(p,'r',encoding='utf-8') as f:
    d=json.load(f)
v = d.get(k)
if isinstance(v, bool): print("true" if v else "false")
else: print(v)
PY
  fi
}

# --- Sanity checks ---
[[ -f "$CONFIG_JSON" ]] || { echo "[ERR] Not found: $CONFIG_JSON"; exit 1; }
[[ -f "$INGEST_CONF" ]] || { echo "[ERR] Not found: $INGEST_CONF"; exit 1; }

target_mb_raw="$(read_json_key targetSizeMB)"
file_mb_raw="$(read_json_key maxFileSizeMB)"

# Validate numbers
[[ "$target_mb_raw" =~ ^[0-9]+$ ]] || { echo "[ERR] targetSizeMB not int: $target_mb_raw"; exit 1; }
[[ "$file_mb_raw" =~ ^[0-9]+$ ]]   || { echo "[ERR] maxFileSizeMB not int: $file_mb_raw"; exit 1; }

target_mb="$target_mb_raw"
file_mb="$file_mb_raw"
(( target_mb > 0 && file_mb > 0 )) || { echo "[ERR] targetSizeMB/maxFileSizeMB must be > 0"; exit 1; }

# --- Derive N = number of files in target window (ceil) ---
n_files=$(( (target_mb + file_mb - 1) / file_mb ))
# Cap N at a reasonable floor/ceiling to avoid extremes
n_files_capped="$(clamp "$n_files" 4 2000)"

# --- Decide compaction-trigger (base) by POLICY ---
# Rationale:
# - early: 约 0.5*N，提前触发，追求更低 L0 backlog、低尾延迟
# - balanced: 约 1.0*N（与“1GB/10MB→100”一致），更接近你之前用的经验值
# - heavy: 约 1.5*N，在吞吐优先/后台较强时减少过度 compaction 启动
case "$POLICY" in
  early)
    base_raw=$(( (n_files_capped + 1) / 2 ))
    base="$(round_to_nearest_10 "$(clamp "$base_raw" 20 200)")"
    ;;
  balanced)
    base="$(round_to_nearest_10 "$(clamp "$n_files_capped" 20 200)")"
    ;;
  heavy)
    base_raw=$(( (n_files_capped * 3) / 2 ))
    base="$(round_to_nearest_10 "$(clamp "$base_raw" 50 300)")"
    ;;
  *)
    echo "[WARN] Unknown POLICY='$POLICY', fallback to 'balanced'"
    base="$(round_to_nearest_10 "$(clamp "$n_files_capped" 20 200)")"
    ;;
esac

# Derive slowdown/stop
slowdown=$(( base * SLOWDOWN_FACTOR ))
stop=$(( base * STOP_FACTOR ))

echo "[info] targetSizeMB=${target_mb}MB, maxFileSizeMB=${file_mb}MB -> N≈${n_files}"
echo "[info] POLICY=${POLICY}, base=${base}, slowdown=${slowdown}, stop=${stop}"

# --- DRY RUN? ---
if [[ "$DRY_RUN" == "1" ]]; then
  echo "[dry-run] Will NOT modify: $INGEST_CONF"
  exit 0
fi

# --- Backup then rewrite in place (preserve comments) ---
ts="$(date +%Y%m%d-%H%M%S)"
backup="${INGEST_CONF}.bak.${ts}"
cp -f "$INGEST_CONF" "$backup"
echo "[info] Backup created: $backup"

tmp="$(mktemp)"
# Only replace the numeric literal after the colon; keep spacing and trailing comments
sed -E \
  -e "s/^(ingest\.aggr\.level0-file-num-compaction-trigger\s*:\s*)[0-9]+/\1${base}/" \
  -e "s/^(ingest\.aggr\.level0-slowdown-writes-trigger\s*:\s*)[0-9]+/\1${slowdown}/" \
  -e "s/^(ingest\.aggr\.level0-stop-writes-trigger\s*:\s*)[0-9]+/\1${stop}/" \
  "$INGEST_CONF" > "$tmp"

mv "$tmp" "$INGEST_CONF"
echo "[ok] Updated $INGEST_CONF"
