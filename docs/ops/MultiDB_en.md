## Pika has supported multiple DBs since version 3.1.0. For compatibility, some commands and configuration parameters changed. The changes are as follows:

### 1. `info keyspace` command:

**Retained:**

`info keyspace [1|0]`: Trigger statistics and display, or only display key information for all DBs.

**Added:**

`info keyspace [1|0] db0`: Trigger statistics and display, or only display key information for db0.

`info keyspace [1|0] db0,db2`: Trigger statistics and display, or only display key information for db0 and db2.

Note: DB names must be specified using `db[0-7]`; multiple DB names are separated by commas.



## 2. `compact` command:

**Retained:**

`compact`: Compact all DBs.

`compact [string/hash/set/zset/list/all]`: Compact a specific data structure, or all data structures, for all DBs.

**Added:**

`compact db0 all`: Compact all data structures for db0 only.

`compact db0,db2 all`: Compact all data structures for db0 and db2.

`compact db1 string`: Compact the string data structure for db1 only.

`compact db1,db3 hash`: Compact the hash data structure for db1 and db3.

Note: DB names must be specified using `db[0-7]`; multiple DB names are separated by commas.


## 3. `slaveof` command:

**Retained:**

`slaveof 192.168.1.1 6236 [force]`: Creates a sync relationship for the pika instance, affecting all DBs. The `force` parameter performs an instance-level full sync.

**Removed:**

`slaveof 192.168.1.1 6236 1234 111222333`: Specifying a write2file file number and write2file file offset when creating a global sync relationship is no longer supported.


## 4. `bgsave` command:

**Retained:**

`bgsave`: Perform a snapshot backup of all DBs.

**Added:**

`bgsave db0`: Back up db0 only.

`bgsave db0,db3`: Back up db0 and db3 only.

Note: DB names must be specified using `db[0-7]`; multiple DB names are separated by commas.


## 5. `purgelogsto` command:

**Retained:**

`purgelogsto write2file1000`: Deletes all write2file files in db0 before write2file1000.

**Added:**

`purgelogsto write2file1000 db1`: Deletes all write2file files in db1 before write2file1000. Only one DB can be operated on at a time.

Note: DB names must be specified using `db[0-7]`.


## 6. `flushdb` command:

**Retained:**

`flushdb [string/hash/set/zset/list]`: Deletes a specific data structure in a DB.

**Added:**

`flushdb`: Deletes all data structures in a DB.

Note: Consistent with Redis, please `select` the correct DB before executing `flushdb` in Pika to avoid accidentally deleting data.


## 7. `dbslaveof` command:

`dbslaveof db[0 ~ 7]`: Sync a specific DB.

`dbslaveof db[0 ~ 7] force`: Fully sync a specific DB.

`dbslaveof db[0 ~ 7] no one`: Stop syncing a specific DB.

`dbslaveof db[0 ~ 7] filenum offset`: Sync a specific DB starting from a specified offset.

Note: This command requires a master-slave relationship to have been established between two Pika instances before it can control the sync state of a single DB.
