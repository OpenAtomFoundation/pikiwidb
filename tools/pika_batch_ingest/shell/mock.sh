# #!/bin/bash

# # 默认参数值
# DEFAULT_SIZE="10M"
# DEFAULT_DICT="kvdict"

# # 解析命令行参数
# while getopts ":n:d:" opt; do
#   case $opt in
#     n)
#       size="$OPTARG"
#       ;;
#     d)
#       dict="$OPTARG"
#       ;;
#     \?)
#       echo "无效选项: -$OPTARG" >&2
#       exit 1
#       ;;
#     :)
#       echo "选项 -$OPTARG 需要参数值." >&2
#       exit 1
#       ;;
#   esac
# done

# # 使用默认值（如果用户未输入）
# size=${size:-$DEFAULT_SIZE}
# dict=${dict:-$DEFAULT_DICT}

# # 执行构建和运行命令
# source "$(dirname "$0")/env.sh"
# cd_build_bin

# ./mock -n "$size" -d "$dict"

source "$(dirname "$0")/env.sh"
cd_build_bin

start_time=$(date +%s%3N)

./mock

end_time=$(date +%s%3N)
elapsed=$((end_time - start_time))  # 毫秒总数

# 转换
hours=$((elapsed / 3600000))
minutes=$(((elapsed % 3600000) / 60000))
seconds=$(((elapsed % 60000) / 1000))
millis=$((elapsed % 1000))

echo "[MOCK] 执行耗时: ${hours}小时 ${minutes}分钟 ${seconds}秒 ${millis}毫秒"
