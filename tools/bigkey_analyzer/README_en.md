# Big Key Analyzer

A big key analysis tool for analyzing large keys in PikiwiDB instances. This tool is designed for the new storage structure in the unstable branch and supports multiple directory structures:
- Single-instance RocksDB
- Multi-DB instances (db/0, db/1, db/2...)
- Direct partition directories (0/, 1/, 2/...)
- **New**: Three-level nested dbN/M structure (db0/0, db0/1, db1/0...)

## Features

- Supports analysis of large keys across various data types (strings, hashes, lists, sets, zsets)
- Ability to filter keys by size
- Ability to limit the number of output results (top N)
- Supports key prefix statistics
- Output includes key type, size, and TTL
- Results can be written to a file

## Build

Run the following commands from the PikiwiDB root directory:

```bash
mkdir -p build
cd build
cmake ..
make bigkey_analyzer
```

After compilation, the executable will be generated in the build directory.

## Usage

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

## Examples

1. Analyze all large keys:

```bash
# Single instance
./bigkey_analyzer /path/to/pikiwidb/data

# Multi-DB instance (db/0, db/1, db/2...)
./bigkey_analyzer /path/to/pikiwidb
```

2. Only analyze keys larger than 1 MB:

```bash
./bigkey_analyzer --min-size=1048576 /path/to/pikiwidb/data
```

3. Only show the top 10 largest keys:

```bash
./bigkey_analyzer --top=10 /path/to/pikiwidb/data
```

4. Only analyze hash-type keys:

```bash
./bigkey_analyzer --type=hashes /path/to/pikiwidb/data
```

5. Analyze and show statistics by prefix:

```bash
./bigkey_analyzer --prefix-stat /path/to/pikiwidb/data
```

6. Write results to a file:

```bash
./bigkey_analyzer --output=result.txt /path/to/pikiwidb/data
```

## Output Format

The tool output consists of three sections:

1. Large key list — sorted in descending order by size
2. Prefix statistics (if the `--prefix-stat` option is used)
3. Summary statistics

Example output:

```
===== Big Key Analysis =====
DB      Partition       Type    Size    Key     TTL
db0     1               hash    1048576 user:profile:1001    -1
db0     2               zset    524288  ranking:global       3600
db1     0               string  262144  config:settings      -1
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

## Notes

- The tool only reads the database and does not perform any write operations.
- The size of a large key includes the total size of both key and value.
- Expired keys are not included in the analysis results.
