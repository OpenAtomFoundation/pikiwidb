# BigKey Analyzer

BigKey Analyzer 是一个离线分析工具，用于检测和分析 PikiwiDB 数据库中的大键(Big Keys)。该工具直接读取 RocksDB 数据文件，无需启动数据库服务，可以帮助运维人员识别可能影响系统性能的大键。

## 功能特性

- 直接读取 RocksDB 数据文件，无需服务端运行
- 支持所有 PikiwiDB 数据类型：strings, hashes, sets, zsets, lists
- 按键大小排序显示结果
- 支持设置最小键大小阈值，只显示超过阈值的键
- 支持限制显示结果数量（Top N）
- 按前缀分析键分布情况
- 可输出到文件

## 编译

```bash
# 确保在 PikiwiDB 根目录下
mkdir -p build && cd build

# 带上工具编译参数
cmake .. -DUSE_PIKA_TOOLS=ON
make bigkey_analyzer
```

## 使用方法

```bash
./bigkey_analyzer [OPTIONS] <db_path>
```

### 选项

- `--min-size=SIZE`：只显示大于 SIZE 字节的键
- `--top=N`：只显示前 N 个最大的键
- `--prefix-stat`：显示按前缀分组的统计信息
- `--prefix-delimiter=C`：设置前缀分隔符（默认为 ":"）
- `--type=TYPE`：只分析指定类型，可选值：strings|hashes|lists|sets|zsets|all
- `--output=FILE`：输出结果到文件
- `--help`：显示帮助信息

### 示例

分析数据库中所有键：
```bash
./bigkey_analyzer /path/to/db_directory
```

只显示大于 1MB 的键：
```bash
./bigkey_analyzer --min-size=1048576 /path/to/db_directory
```

只显示前 10 个最大的键：
```bash
./bigkey_analyzer --top=10 /path/to/db_directory
```

按前缀统计键分布：
```bash
./bigkey_analyzer --prefix-stat /path/to/db_directory
```

使用自定义前缀分隔符：
```bash
./bigkey_analyzer --prefix-stat --prefix-delimiter="." /path/to/db_directory
```

只分析字符串类型：
```bash
./bigkey_analyzer --type=strings /path/to/db_directory
```

输出到文件：
```bash
./bigkey_analyzer --output=result.txt /path/to/db_directory
```

## 输出格式

基本输出格式：
```
Type Size Key TTL
```

- `Type`：键类型（string, hash, list, set, zset）
- `Size`：键占用的总字节数（包括元数据）
- `Key`：键名称
- `TTL`：剩余生存时间（秒），-1 表示无过期时间

前缀统计输出格式：
```
Prefix Count TotalSize AvgSize
```

- `Prefix`：键前缀
- `Count`：该前缀下的键数量
- `TotalSize`：该前缀下所有键的总大小（字节）
- `AvgSize`：该前缀下键的平均大小（字节）

## 提示

1. 对于非常大的数据库，建议先使用 `--min-size` 设置一个较大的阈值（如 1MB）来过滤小键。
2. 使用 `--prefix-stat` 可以帮助识别特定前缀下的键分布情况，有助于发现问题模块。
3. 大键可能导致性能问题，可以考虑以下解决方案：
   - 拆分大的 hash, set, zset 为多个小的
   - 使用适当的过期策略
   - 使用压缩算法减小值的大小

## 常见问题

1. **"Error: Database directory does not exist"**
   - 确保提供了正确的数据库路径
   - 数据库路径通常包含 strings, hashes, sets, zsets, lists 子目录

2. **"Error opening X database"**
   - 确保数据库文件未被锁定（如数据库正在运行）
   - 检查是否有足够的文件访问权限

3. **显示的键大小与内存使用不匹配**
   - 此工具计算的是键在存储引擎中的总大小，包括元数据
   - 实际内存使用可能因内存分配和 RocksDB 缓存策略而有所不同
