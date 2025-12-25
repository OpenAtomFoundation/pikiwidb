# Big Key Analyzer

大key分析工具,用于分析PikiwiDB实例中的大key情况。本工具适用于unstable分支新的存储结构,支持单实例和多DB实例(db/0, db/1, db/2...)。

## 功能特点

- 支持分析各种数据类型(strings, hashes, lists, sets, zsets)的大key
- 可以按大小过滤key
- 可以限制输出结果数量(top N)
- 支持按key前缀统计
- 输出结果包含key类型、大小和过期时间(TTL)
- 可以将结果输出到文件

## 编译

在PikiwiDB根目录下执行:

```bash
mkdir -p build
cd build
cmake ..
make bigkey_analyzer
```

编译完成后,可执行文件会生成在build目录下。

## 使用方法

```
Usage: bigkey_analyzer [OPTIONS] <db_path>
Options:
  --min-size=SIZE       Only show keys larger than SIZE bytes
  --top=N               Only show top N largest keys
  --prefix-stat         Show statistics by key prefix
  --prefix-delimiter=C  Character used to delimit prefix (default: ':')
  --type=TYPE           Only analyze specific type (strings|hashes|lists|sets|zsets|all)
  --output=FILE         Write output to file instead of stdout
  --help                Display this help message
```

## 示例

1. 分析所有大key:

```bash
# 单实例
./bigkey_analyzer /path/to/pikiwidb/data

# 多DB实例(db/0, db/1, db/2...)
./bigkey_analyzer /path/to/pikiwidb
```

2. 只分析大于1MB的key:

```bash
./bigkey_analyzer --min-size=1048576 /path/to/pikiwidb/data
```

3. 只显示前10个最大的key:

```bash
./bigkey_analyzer --top=10 /path/to/pikiwidb/data
```

4. 只分析hash类型的key:

```bash
./bigkey_analyzer --type=hashes /path/to/pikiwidb/data
```

5. 分析并按前缀统计:

```bash
./bigkey_analyzer --prefix-stat /path/to/pikiwidb/data
```

6. 输出结果到文件:

```bash
./bigkey_analyzer --output=result.txt /path/to/pikiwidb/data
```

## 输出格式

工具输出包括三部分:

1. 大key列表 - 按大小降序排列
2. 按前缀统计(如果使用--prefix-stat选项)
3. 总结统计信息

示例输出:

```
===== Big Key Analysis =====
Type    Size    Key     TTL
hash    1048576 user:profile:1001    -1
zset    524288  ranking:global       3600
string  262144  config:settings      -1
...

===== Key Prefix Statistics =====
Prefix  Count   Total Size      Avg Size
user    100     10485760        104857.6
ranking 50      2621440         52428.8
config  10      524288          52428.8
...

===== Summary =====
Total keys analyzed: 160
Keys by type:
  hash: 50 keys, 25.0 MB total, 524288.0 bytes avg
  zset: 30 keys, 15.0 MB total, 524288.0 bytes avg
  string: 80 keys, 10.0 MB total, 131072.0 bytes avg
```

## 注意事项

- 工具只读取数据库,不会进行任何写操作
- 大key的大小包括key和value的总大小
- 已过期的key不会被包含在分析结果中
