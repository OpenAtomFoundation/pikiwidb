### Pika Memory Usage
1. RocksDB memory usage

#### 1. RocksDB Memory Usage
Command: `info data`

used_memory_human = db_memtable_usage + db_tablereader_usage

Relevant configuration parameters and their impact:

write-buffer-size          => db_memtable_usage

max-write-buffer-size      => db_memtable_usage

max-cache-files            => db_tablereader_usage

Corresponding RocksDB configuration documentation:

https://github.com/facebook/rocksdb/wiki/Setup-Options-and-Basic-Tuning

https://github.com/facebook/rocksdb/wiki/Memory-usage-in-RocksDB
