# Pika Dual Master Documentation
Available versions: 2.3.0 ~ 3.0.11

## Introduction
To avoid extra maintenance costs, Pika supports a dual-master architecture based on the existing master-slave logic, configured via the `conf` file. After a successful dual-master setup, you can still add `slave` nodes to either `master` using `slaveof`.

## Usage
* Configure dual-master mode via the `pika.conf` file.

  Modify the `pika.conf` of dual-master instance A:

  ```
  ...
  # server-id for hub
  server-id : 1
  ...
  # The peer-master config
  double-master-ip :    192.168.10.2    # IP of the other master
  double-master-port : 9220             # Port of the other master
  double-master-server-id : 2           # Server ID of the other master (must not conflict with local server-id or any connected slave's sid)
  ```

  Modify the `pika.conf` of dual-master instance B:

  ```
  ...
  # server-id for hub
  server-id : 2
  ...
  # The peer-master config
  double-master-ip :    192.168.10.1    # IP of the other master
  double-master-port : 9220             # Port of the other master
  double-master-server-id : 1           # Server ID of the other master (must not conflict with local server-id or any connected slave's sid)
  ```

* Start both Pika instances and use `info` to check their status.

  Check `info` on `192.168.10.1`:

  ```
  ...
  # DoubleMaster(DOUBLEMASTER)
  role:double_master
  the peer-master host:192.168.10.2
  the peer-master port:9220
  double_master_mode: True
  repl_state: 3
  double_master_server_id:2
  double_master_recv_info: filenum 0 offset 0
  ```

  `repl_state` of `3` indicates the dual-master connection was established successfully.

## Binlog Offset Verification Failure at Dual-Master Startup
* When both instances start, they verify each other's `binlog` offsets. If the offsets differ too much, the dual-master connection will fail to establish.

  An `INFO`-level log will report:

  ```
  pika_admin.cc:169] Because the invalid filenum and offset, close the connection between the peer-masters
  ```

  Use the `info` command to check the instance status:

  ```
  # DoubleMaster(MASTER)
  role:master
  the peer-master host:192.168.10.2
  the peer-master port:9220
  double_master_mode: True
  repl_state: 0
  double_master_server_id:2
  double_master_recv_info: filenum 0 offset 0
  ```
  `repl_state` of `0` indicates the dual-master setup failed.

* If the dual-master setup fails, you can recover without downtime by using `slaveof` to perform a full sync from one instance to the other.

  For example, to make `192.168.10.2` do a full sync from `192.168.10.1`:

  ```
  slaveof 192.168.10.1
  ```

## Disconnecting the Dual-Master Connection
Run `slaveof no one` on both dual-master nodes to disconnect the relationship.

## Test Scenarios and Results

|-|Scenario|Test Result|Notes|
|:-:|:-:|:-:|:-:|
|**Correctness Tests**|
|1|Single-sided writes in dual-master|Tested twice, using 12 and 64 threads respectively, with 300k–1M keys; no key loss found; values verified equal|Pass|
|2|Simultaneous writes from both sides (different keys)|Tested 3 times, ~600k keys; both sides have the same key count; values verified equal|Pass|
|3|Simultaneous writes from both sides (same keys)|Tested 2 times, ~400k keys; both sides have the same key count; values verified equal|Pass|
|4|Simultaneous writes (different keys) with iptables simulating network delay|Tested 3 times, ~600k keys; both sides have the same key count; values verified equal|Pass|
|5|Simultaneous writes (same keys) with iptables simulating network delay|Tested 2 times, ~400k keys; both sides have the same key count, but some key values differ|Pass|
|6|Simultaneous writes (same keys) with two slave nodes mounted|~200k keys; both sides have the same key count and values|Pass|
|**Operations Tests**|
|7|Single-sided writes, shutdown one master, then attach a new slave and rebuild dual master|~1M keys; both sides have the same key count and values|Pass|
|8|Single-sided writes, restart one master|~1M keys; both sides have the same key count and values|Pass|

## FAQ
* **How to avoid data looping in dual-master mode?**
> Each dual-master instance internally maintains information about the other `master`. It uses the `server id` field in the `binlog` to determine whether data needs to be forwarded to the other `master`.

* **How to disable dual-master mode?**
> The dual-master relationship can be cancelled at any time by stopping one of the nodes.

* **Is dual-write supported in dual-master mode?**
> Yes, dual-write is supported.

* **Can both instances simultaneously operate on the same key?**
> In dual-master mode, operating on the same key from both sides does not guarantee eventual value consistency.

* **How to add `slave` nodes in dual-master mode?**
> Use the normal master-slave configuration method.

* **Dual-master A and B: instance B has been down for too long, and after restart, the binlog offset difference is too large so the dual-master fails to re-establish. How to recover?**
> Use instance A as the baseline, perform a full sync via the `slaveof` command, and the dual-master connection will automatically recover afterward.
