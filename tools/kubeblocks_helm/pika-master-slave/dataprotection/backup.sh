#!/bin/bash
set -o pipefail
function handle_exit() {
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        echo "failed with exit code $exit_code"
        exit 1
    fi
}
trap handle_exit EXIT
export PATH="$PATH:$DP_DATASAFED_BIN_PATH"
export DATASAFED_BACKEND_BASE_PATH="$DP_BACKUP_BASE_PATH"

connect_url="redis-cli -h ${DP_DB_HOST} -p ${DP_DB_PORT}"
last_save=$(${connect_url} LASTSAVE)

echo "INFO: start BGSAVE"
${connect_url} BGSAVE

echo "INFO: wait for saving dump successfully"
while true; do
  end_save=$(${connect_url} LASTSAVE)
  if [ $end_save -ne $last_save ];then
     break
  fi
  sleep 1
done

cd ${DATA_DIR}

if [ -d "log" ] || [ -d "db" ]; then
    tar -cvf - ./log ./db | datasafed push -z zstd-fastest - "${DP_BACKUP_NAME}.tar.zst"
else
    echo "no log db"
    exit 1
fi
echo "INFO: save data file successfully"
TOTAL_SIZE=$(datasafed stat / | grep TotalSize | awk '{print $2}')
echo "{\"totalSize\":\"$TOTAL_SIZE\"}" >"${DP_BACKUP_INFO_FILE}" && sync
