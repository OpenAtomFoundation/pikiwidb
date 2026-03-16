#!/bin/bash
# 检查 dump 目录下的所有孤儿文件（nlink=1 的 SST 文件）及其大小

DUMP_DIR="${1:-/app/pika4/pika-9454/dump}"

echo "=========================================="
echo "扫描 Dump 目录下的孤儿文件 (nlink=1)"
echo "目录: $DUMP_DIR"
echo "=========================================="

# 统计总文件数、孤儿文件数、总大小
total_files=0
orphan_files=0
total_orphan_size=0

# 遍历所有 dump 子目录
for dump_subdir in "$DUMP_DIR"/dump-*/; do
    if [ -d "$dump_subdir" ]; then
        echo ""
        echo "检查目录: $dump_subdir"
        echo "----------------------------------------"

        # 查找所有 .sst 文件并检查 nlink
        find "$dump_subdir" -name "*.sst" -type f 2>/dev/null | while read -r file; do
            total_files=$((total_files + 1))

            # 获取硬链接数
            nlink=$(stat -c %h "$file" 2>/dev/null || stat -f %l "$file" 2>/dev/null)

            if [ "$nlink" -eq 1 ]; then
                # 获取文件大小
                size=$(stat -c %s "$file" 2>/dev/null || stat -f %z "$file" 2>/dev/null)

                if command -v numfmt >/dev/null 2>&1; then
                    human_size=$(numfmt --to=iec-i --suffix=B "$size" 2>/dev/null)
                else
                    human_size="${size} bytes"
                fi

                echo "[孤儿文件] $file (大小: $human_size)"
            fi
        done
    fi
done

echo ""
echo "=========================================="
echo "正在统计总数..."
echo "=========================================="

# 重新统计
total_files=0
orphan_files=0
total_orphan_size=0

for dump_subdir in "$DUMP_DIR"/dump-*/; do
    if [ -d "$dump_subdir" ]; then
        find "$dump_subdir" -name "*.sst" -type f 2>/dev/null | while read -r file; do
            total_files=$((total_files + 1))
            nlink=$(stat -c %h "$file" 2>/dev/null || stat -f %l "$file" 2>/dev/null)
            if [ "$nlink" -eq 1 ]; then
                size=$(stat -c %s "$file" 2>/dev/null || stat -f %z "$file" 2>/dev/null)
                orphan_files=$((orphan_files + 1))
                total_orphan_size=$((total_orphan_size + size))
                echo "$size $file"
            fi
        done
    fi
done > /tmp/orphan_list.txt

orphan_files=$(wc -l < /tmp/orphan_list.txt 2>/dev/null || echo 0)
total_orphan_size=$(awk '{sum+=$1} END {print sum}' /tmp/orphan_list.txt 2>/dev/null || echo 0)

echo "统计结果:"
echo "  孤儿文件数: $orphan_files"
if command -v numfmt >/dev/null 2>&1; then
    echo "  孤儿文件总大小: $(numfmt --to=iec-i --suffix=B $total_orphan_size 2>/dev/null || echo ${total_orphan_size}bytes)"
else
    echo "  孤儿文件总大小: $total_orphan_size bytes"
fi

rm -f /tmp/orphan_list.txt
echo "=========================================="
