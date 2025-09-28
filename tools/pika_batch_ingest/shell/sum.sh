#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

LOG_FILE="${1:-$PROJECT_ROOT/data/klog/master.log}"   # 可传参覆盖
CMD_TAG="${2:-ManifestIngestCmd}"                     # 可传参覆盖
STATUS="${3:-SUCCESS}"                                # SUCCESS/FAIL

awk -v cmd_tag="$CMD_TAG" -v status="$STATUS" '
$0 ~ "\\[" cmd_tag "\\]" {
  # I20250926 08:42:54.651342 876540 pika_kv.cc:223] [ManifestIngestCmd] msg...
  if (match($0, /^.([0-9]{8})[[:space:]]+([0-9]{2}):([0-9]{2}):([0-9]{2})\.([0-9]{3,6})[[:space:]]+([0-9]+)[[:space:]]+([^ ]+)\] (.*)$/, m)) {
    ymd=m[1]; HH=m[2]; MM=m[3]; SS=m[4]; us=m[5]; msg=m[8];

    if (length(us)==3) us=us "000";                 # 统一成微秒 6 位
    yyyy=substr(ymd,1,4); mon=substr(ymd,5,2); day=substr(ymd,7,2);
    sec = mktime(sprintf("%s %s %s %s %s %s", yyyy, mon, day, HH, MM, SS));
    t_us = sec*1000000 + us + 0;

    # 去掉前缀 "[ManifestIngestCmd] "
    prefix="[" cmd_tag "] ";
    if (index(msg, prefix)==1) msg=substr(msg, length(prefix)+1);

    ts_ms = sprintf("%s-%s-%s %s:%s:%s.%s", yyyy, mon, day, HH, MM, SS, substr(us,1,3));

    if (cnt==0) { first_us=t_us; first_ts=ts_ms; first_msg=msg; }
    last_us=t_us; last_ts=ts_ms; last_msg=msg;
    cnt++;
  }
}
END{
  if (cnt==0) { printf("No [%s] lines found.\n", cmd_tag); exit 0; }

  diff_us = last_us - first_us;
  printf("=============== Ingest 摘要 ===============\n");
  printf("状态   : %s\n", status);
  printf("命令   : %s\n", cmd_tag);
  printf("开始   : %s\n", first_ts);
  printf("结束   : %s\n", last_ts);
  printf("耗时   : %.0f ms (%.3f s)\n", diff_us/1000.0, diff_us/1e6);
  printf("备注   : %s\n", last_msg);
  printf("==========================================\n");
}
' "$LOG_FILE"
