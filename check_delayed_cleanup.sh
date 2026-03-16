#!/bin/bash
# 分析日志，找出被添加到延迟删除队列但实际没有执行删除的文件

LOG_FILE="${1:-/app/pika4/pika-9454/log/pika.INFO}"

echo "=========================================="
echo "分析延迟清理队列执行情况"
echo "日志文件: $LOG_FILE"
echo "=========================================="

# 提取所有 Scheduled 的文件（延迟删除调度）
echo ""
echo "步骤 1: 提取所有被调度的文件..."
grep "Scheduled file for delayed cleanup" "$LOG_FILE" 2>/dev/null | \
    sed 's/.*cleanup: //g' | \
    awk '{print $1}' | \
    grep "^/" | \
    sort -u > /tmp/scheduled_files.txt

scheduled_count=$(wc -l < /tmp/scheduled_files.txt 2>/dev/null | tr -d ' ')
echo "  被调度的文件数: $scheduled_count"

# 提取所有 Deleted 的文件（实际删除）
echo ""
echo "步骤 2: 提取所有实际删除的文件..."
grep "Deleted delayed cleanup file" "$LOG_FILE" 2>/dev/null | \
    sed 's/.*Deleted delayed cleanup file: //g' | \
    awk '{print $1}' | \
    grep "^/" | \
    sort -u > /tmp/deleted_files.txt

deleted_count=$(wc -l < /tmp/deleted_files.txt 2>/dev/null | tr -d ' ')
echo "  实际删除的文件数: $deleted_count"

# 如果没有任何记录，直接退出
if [ "$scheduled_count" -eq 0 ] && [ "$deleted_count" -eq 0 ]; then
    echo ""
    echo "未找到任何延迟清理相关日志"
    rm -f /tmp/scheduled_files.txt /tmp/deleted_files.txt
    exit 0
fi

# 找出被调度但未删除的文件
echo ""
echo "步骤 3: 找出被调度但未删除的文件..."
comm -23 /tmp/scheduled_files.txt /tmp/deleted_files.txt > /tmp/missing_files.txt

missing_count=$(wc -l < /tmp/missing_files.txt 2>/dev/null | tr -d ' ')
echo "  被调度但未删除的文件数: $missing_count"

if [ "$missing_count" -gt 0 ]; then
    echo ""
    echo "=========================================="
    echo "被调度但未删除的文件列表:"
    echo "=========================================="

    while IFS= read -r filepath; do
        if [ -n "$filepath" ]; then
            # 检查文件是否仍然存在
            if [ -f "$filepath" ]; then
                # 获取文件大小和 nlink
                size=$(stat -c %s "$filepath" 2>/dev/null || stat -f %z "$filepath" 2>/dev/null)
                nlink=$(stat -c %h "$filepath" 2>/dev/null || stat -f %l "$filepath" 2>/dev/null)

                if command -v numfmt >/dev/null 2>&1; then
                    human_size=$(numfmt --to=iec-i --suffix=B "$size" 2>/dev/null)
                else
                    human_size="${size} bytes"
                fi

                echo "  [仍存在] $filepath"
                echo "           大小: $human_size, 硬链接数: $nlink"

                # 查找该文件的调度时间
                scheduled_time=$(grep "Scheduled file for delayed cleanup.*$filepath" "$LOG_FILE" | tail -1 | awk '{print $1}')
                if [ -n "$scheduled_time" ]; then
                    echo "           调度时间: $scheduled_time"
                fi
                echo ""
            else
                echo "  [已消失] $filepath (可能已被其他方式删除)"
            fi
        fi
    done < /tmp/missing_files.txt
fi

# 清理临时文件
rm -f /tmp/scheduled_files.txt /tmp/deleted_files.txt /tmp/missing_files.txt

echo ""
echo "=========================================="
echo "总结:"
echo "  调度文件: $scheduled_count"
echo "  删除文件: $deleted_count"
echo "  未删除文件: $missing_count"
if [ "$missing_count" -gt 0 ]; then
    echo ""
    echo "⚠️  发现 $missing_count 个文件被调度但未删除！"
    echo "   可能原因:"
    echo "   1. 延迟时间未到 (600秒)"
    echo "   2. 文件在删除时 nlink != 1 (不再是孤儿文件)"
    echo "   3. ProcessPendingCleanupFiles 未执行或执行失败"
fi
echo "=========================================="
