## Parameters
All command-line parameters are as follows:
```
  -command (command to execute, eg: generate/get/set/zadd) type: string default: "generate"
  -compare_value (whether compare result or not) type: bool default: false
  -count (request counts per thread) type: int32 default: 100000
  -dbs (dbs name, eg: 0,1,2) type: string default: "0"
  -element_count (elements number in hash/list/set/zset) type: int32 default: 1
  -host (target server's host) type: string default: "127.0.0.1"
  -key_size (key size int bytes) type: int32 default: 50
  -password (password) type: string default: ""
  -port (target server's listen port) type: int32 default: 9221
  -thread_num (concurrent thread num) type: int32 default: 10
  -timeout (request timeout) type: int32 default: 1000
  -value_size (value size in bytes) type: int32 default: 100
```

compare_value: Whether to perform data verification. Both the set and get operations need to be set to true for verification. When `compare_value` is true, the value used in the set command is derived from the concatenation of the key; when `compare_value` is false, the value is random.

count: Number of keys requested per thread.

element_count: Number of members per primary key (pkey) for list/zset/set/hash.

Currently supported commands include: get, set, hset, hgetall, sadd, smembers, lpush, lrange, zadd, zrange.

## Usage
First, run the `generate` command to generate the keys to be requested, for example:
```
./benchmark_client --command=generate --count=2 --element_count=10 --port=9271 --thread_num=2 --key_size=10 --value_size=25 --host=127.0.0.1 --compare_value=1
```
After execution, two `benchmark_keyfile_*` files (one per thread) will be generated in the current directory. Each file has 20 lines (`element_count * count`), and each key is 10 characters long.

You can then run Redis API commands. When a command is executed, it first reads the keys from the `benchmark_keyfile_*` files and sets the value based on the `compare_value` setting.

set command:
```
./benchmark_client --command=set --count=2 --port=9271 --thread_num=2 --key_size=10 --value_size=25 --host=127.0.0.1 --compare_value=1
```

get command:
```
./benchmark_client --command=get --count=2 --port=9271 --thread_num=2 --key_size=10 --value_size=25 --host=127.0.0.1 --compare_value=1
```

hset command:
```
// This will write a total of 4 hash pkeys to Pika, each containing 10 members.
./benchmark_client --command=set --count=2 --element_count=10 --port=9271 --thread_num=2 --key_size=10 --value_size=25 --host=127.0.0.1 --compare_value=1
```

hgetall command:
```
./benchmark_client --command=hgetall --count=2 --element_count=10 --port=9271 --thread_num=2 --key_size=10 --value_size=25 --host=127.0.0.1 --compare_value=1
```

## Output
```
=================== Benchmark Client ===================
Server host name: 127.0.0.1
Server port: 9271
command: set
Thread num : 2
Payload size : 25
Number of request : 2
Transmit mode: No Pipeline
Collection of dbs: 0
Elements num: 1
CompareValue : 1
Startup Time : Tue Dec 19 20:21:44 2023

========================================================
Table 0 Thread 0 Select DB Success, start to write data...
Table 0 Thread 1 Select DB Success, start to write data...
finish 0 request
finish 0 request
Finish Time : Tue Dec 19 20:21:44 2023
Total Time Cost : 0 hours 0 minutes 0 seconds
Timeout Count: 0 Error Count: 0
stats: Count: 4 Average: 166.5000  StdDev: 24.64
Min: 0  Median: 202.0000  Max: 202
Percentiles: P50: 202.00 P75: 202.00 P99: 202.00 P99.9: 202.00 P99.99: 202.00
------------------------------------------------------

```
The output consists of two parts. The first part is a description of the current benchmark run. The second part is statistical information, including the number of timed-out requests, error requests, and request latency statistics.
