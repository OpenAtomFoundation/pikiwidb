# Pika Currently Supported Redis Interfaces

Pika supports the interfaces for Redis's five types (string, hash, list, set, zset). Below is a compatibility summary for the five Redis data structures.

#### Legend for the table:

|  Icon  |             Meaning             |
| :--: |:--------------------------:|
|  o   |  Fully supported; usage is identical to Redis  |
|  !   | Functionally supported, but usage or output has some differences from Redis; please note |
|  ×   |           Not yet supported           |


---

## Keys
|Interface|DEL|DUMP|EXISTS|EXPIRE|EXPIREAT|KEYS|MIGRATE|MOVE|OBJECT|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|x|o|o|o|o|x|x|x|
|Interface|PERSIST|PEXPIRE|PEXPIREAT|PTTL|RANDOMKEY|RENAME|RENAMENX|RESTORE|SORT|
|Status|o|!|!|o|x|x|x|x|x|
|Interface|TOUCH|TTL|TYPE|UNLINK|WAIT|SCAN|
|Status|x|o|!|o|x|!|


**Notes:**

* PEXPIRE: Cannot be precise to milliseconds; the underlying layer automatically truncates to second-level precision.
* PEXPIREAT: Cannot be precise to milliseconds; the underlying layer automatically truncates to second-level precision.
* SCAN: Iterates through a snapshot of the current DB sequentially. Since Pika allows duplicate names five times, scan has a priority output order: string -> hash -> list -> zset -> set.
* TYPE: Since Pika allows duplicate names five times, type has a priority output order: string -> hash -> list -> zset -> set. If this key exists in string, only "string" is output; if not, hash is output, and so on.
* KEYS: The KEYS command supports scanning specific data types via a parameter, e.g., `keys * [string, hash, list, zset, set]`.

## Strings

|Interface|APPEND|BITCOUNT|BITFIELD|BITOP|BITPOS|DECR|DECRBY|GET|GETBIT|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|x|!|o|o|o|o|!|
|Interface|GETRANGE|GETSET|INCR|INCRBY|INCRBYFLOAT|MGET|MSET|MSETNX|STRLEN|
|Status|o|o|o|o|o|o|o|o|o|o|
|Interface|PSETEX|SET|SETBIT|SETEX|SETNX|SETRANGE|
|Status|o|o|!|o|o|o|


**Notes:**

* BIT operations: Unlike Redis, Pika's bit operation range is 2^21, with a maximum bitmap size of 256 KB. Redis `setbit` only updates the key's value. But Pika uses RocksDB as the storage engine, which only writes new data and only deletes old data from disk during compaction. If Pika's bit operation range matched Redis's 2^32, each `setbit` on the same key might store a 512 MB value in RocksDB. This creates a serious performance risk, so Pika's bit operation range has been reduced.

## Hashes

|Interface|HDEL|HEXISTS|HGET|HGETALL|HINCRBY|HINCRBYFLOAT|HKEYS|HLEN|HMGET|HMSET|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|o|o|o|o|o|
|Interface|HSET|HSETNX|HVALS|HSCAN|HSTRLEN|
|Status|!|o|o|o|o|

**Notes:**

* HSET operation: Setting multiple field-value pairs with a single command is temporarily not supported. Please use HMSET instead.

## Lists

|Interface|LINDEX|LINSERT|LLEN|LPOP| LPUSH | LPUSHX |LRANGE|LREM|LSET|LTRIM|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|x|o|o|o|o|
|Interface|RPOP|RPOPLPUSH|RPUSH|RPUSHX| BLPOP | BRPOP  |BRPOPLPUSH|
|Status|o|o|o|o|o|o|x|

## Sets

|Interface|SADD|SCARD|SDIFF|SDIFFSTORE|SINTER|SINTERSTORE|SISMEMBER|SMEMBERS|SMOVE|SPOP|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|o|o|o|o|o|
|Interface|SRANDMEMBER|SREM|SUNION|SUNIONSTORE|SSCAN|
|Status|!|o|o|o|o|

**Notes:**

* SRANDMEMBER: Time complexity O(n), relatively slow.

## Sorted Sets

|Interface|ZADD|ZCARD|ZCOUNT|ZINCRBY|ZRANGE|ZRANGEBYSCORE|ZRANK|ZREM|ZREMRANGEBYRANK|ZREMRANGEBYSCORE|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|o|o|o|o|o|
|Interface|ZREVRANGE|ZREVRANGEBYSCORE|ZREVRANK|ZSCORE|ZUNIONSTORE|ZINTERSTORE|ZSCAN|ZRANGEBYLEX|ZLEXCOUNT|ZREMRANGEBYLEX|
|Status|o|o|o|o|o|o|o|o|o|o|
|Interface|ZPOPMAX|ZPOPMIN|ZREVERANGEBYLEX||||||||
|Status|o|o|o||||||||

* ZADD options [NX|XX] [CH] [INCR] are temporarily not supported.

## HyperLogLog

|Interface|PFADD|PFCOUNT|PFMERGE|
|:-:|:-:|:-:|:-:|
|Status|o|o|o|

**Notes:**

* Error within 1% for under 500k, less than 3% for under 1 million, but at the cost of additional time.

## GEO

|Interface|GEOADD|GEODIST|GEOHASH|GEOPOS|GEORADIUS|GEORADIUSBYMEMBER|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|o|

## BitMap

|Interface|SETBIT|GETBIT|BITPOS|BITOP|BITCOUNT|
|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|

## Pub/Sub

|Interface|PSUBSCRIBE|PUBSUB|PUBLISH|PUNSUBSCRIBE|SUBSCRIBE|UNSUBSCRIBE|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|o|o|o|o|o|o|

**Notes:**

* Keyspace notifications are temporarily not supported.

## Admin Commands (only those compatible with Pika are listed)

|Interface|INFO|CONFIG|CLIENT|PING|BGSAVE|SHUTDOWN|SELECT|TYPE|HELLO|
|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|:-:|
|Status|!|o|!|o|o|o|!|!|o|

**Notes:**

* info: Supports full output and pattern-matched output. For example, `info stats` shows statistics. Note that key space differs from Redis — Pika displays key space by type rather than by DB (since Pika has no concept of databases). Pika's key space statistics are passive and must be manually triggered. Pika's key space statistics are accurate. To trigger: execute `keyspace` command, then Pika will count in the background. You can use `keyspace readonly` to view without re-triggering. If current data is 0, it's still being counted. `info commandstats` can query call count and time statistics (average time) for each command. Unlike Redis, the time unit we use is milliseconds.

* client: The current client command supports `client list`, `client kill`, and `client setname`. `client list` shows less information than Redis.

* select: This command had no effect before version 3.1.0; since 3.1.0 it is consistent with Redis.

* ping: This command only supports use without parameters, i.e., `PING`, and the client returns `PONG`.
---

## Pika Pub/Sub Documentation

Available version: >= 2.3.0

Note: Keyspace notification functionality is not currently supported.


## Pika Publish/Subscribe Commands
The following Pub/Sub commands are fully compatible with Redis:

* PUBSUB subcommand [argument [argument ...]]
* PUBLISH channel message
* SUBSCRIBE channel [channel ...]
* PSUBSCRIBE pattern [pattern ...]
* UNSUBSCRIBE [channel [channel ...]]
* PUNSUBSCRIBE [pattern [pattern ...]]

For usage details, refer to Redis's [Pub/Sub documentation](http://redisdoc.com/topic/pubsub.html).


## Important Notes

* Key duplication: Since each Pika type operates independently, duplicate keys are allowed. For example, key `abc` can exist in both string and hash at the same time. A key can be duplicated at most 5 times (for the 5 major types), but cannot be duplicated within the same interface. Therefore, it is recommended to avoid using exactly the same key across different types.

* Multi-DB: Pika supports multiple databases since version 3.1.0. For changes to related commands and parameters, refer to the [Pika 3.1.0 multi-DB version command/parameter change reference document](multiDB.md).

* Data display: Pika displays key space by type rather than by DB (since Pika has no concept of databases). Key statistics in Pika are passive and must be manually triggered; they do not output immediately. The command is: `info keyspace [0|1]`, defaulting to 0 (no trigger). Pika's keyspace statistics are accurate.
