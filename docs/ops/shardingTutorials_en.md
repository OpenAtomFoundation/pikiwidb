Regarding sharding mode, Pika's underlying layer provides the concept of slots. Pika hashes keys modulo the number of slots and distributes them to each slot for processing. Sharding mode can be applied to a single Pika instance or to multiple Pika instances forming a Pika cluster, depending on the actual production scenario. This tutorial mainly introduces concepts you need to understand when enabling sharding mode, as well as configuration parameters that need to be adjusted.

#### 0. Choosing a Mode
Currently, Pika has two modes: Classic and Sharding. These two modes are incompatible, so please determine which mode to use based on your business needs first.

The Classic mode supports the vast majority of business workloads and allows concurrent reads/writes across 8 databases (db0–db7). It is recommended that general business users benchmark this mode first.

If Classic mode cannot meet the extremely high pressure of your production environment, you can try Sharding mode. Compared to Classic mode, Sharding mode provides higher QPS but also consumes more hardware resources. Using Codis with Pika as the backend storage as an example: Codis defaults to 1024 total cluster slots, and each slot provides read/write for five data structures. For each data structure, Pika starts a RocksDB instance. This means the cluster needs to start 1024 × 5 = 5120 RocksDB instances. The point of cluster mode is to distribute these 5120 RocksDB instances across the Pika instances on various machines. Therefore, we recommend having a certain scale of physical machines in the cluster so that each machine doesn't host too many RocksDB instances. Of course, in our original design, the operator decides how many physical machines to distribute the 5120 RocksDB instances across. Why can't they all be on one machine? The reasons are:

1. Too many RocksDB instances increase the probability of concurrent compaction, putting excessive pressure on disk.

2. Each RocksDB instance uses a certain number of file descriptors and memory. Multiplied by 5120, this can easily exhaust system resources.

In summary, more hardware resources always provide more performance. We recommend new Pika users start with Classic mode. If it completely fails to meet your current needs, consider using more hardware resources and switching to Sharding mode.

For specific performance benchmarks, refer to [3.2.x Performance](https://github.com/Qihoo360/pika/wiki/3.2.x-Performance).


#### 1. Required Version

Pika supports sharding mode starting from version 3.2.0. It is recommended to use the latest release.

#### 2. Basic Operations

For basic slot operations, see [slot commands](https://github.com/Qihoo360/pika/wiki/Pika%E5%88%86%E7%89%87%E5%91%BD%E4%BB%A4).

#### 3. Configuration File Notes

Relevant configuration parameters:

```
# default slot number each table in sharding mode
  default-slot-num : 1024

# if this option is set to 'classic', that means pika support multiple DB, in
# this mode, option databases enable
# if this option is set to 'sharding', that means pika support multiple Table, you
# can specify slot num for each table, in this mode, option default-slot-num enable
# Pika instance mode [classic | sharding]
  instance-mode : sharding

# Pika write-buffer-size
write-buffer-size : 67108864

# If the total size of all live memtables of all the DBs exceeds
# the limit, a flush will be triggered in the next DB to which the next write
# is issued.
max-write-buffer-size : 10737418240

# maximum value of Rocksdb cached open file descriptors
max-cache-files : 100
```

Configuration explanations can be found in the [Configuration Documentation](https://github.com/Qihoo360/pika/wiki/pika-%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%AF%B4%E6%98%8E). Note in particular that `write-buffer-size` represents the size of each memtable for each RocksDB instance. The total memtable size limit for all RocksDB instances is controlled by `max-write-buffer-size`. When the total reaches `max-write-buffer-size`, each RocksDB instance will attempt to flush its current memtable to reduce memory usage.

#### 4. Compatibility with Codis and Twemproxy

The current distributed framework relies on open-source projects. Pika is currently compatible with Codis and Twemproxy.

For the specific compatibility solution, see [Support Cluster Slots](https://github.com/Qihoo360/pika/wiki/Support-Cluster-Slots).
