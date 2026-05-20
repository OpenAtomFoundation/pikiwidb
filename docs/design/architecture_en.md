### Overview

Pika is an open-source high-performance persistent NoSQL product, compatible with the Redis protocol, with data persistently stored in the RocksDB storage engine. It has two operating modes: Classic mode and Sharding mode.
* Classic mode: A 1-master-N-slave synchronization mode. One master instance stores all the data; N slave instances fully mirror-sync the master's data. Each instance supports multiple DBs. DBs start from 0 by default, and the `databases` configuration item controls the maximum number of DBs. A DB's physical form on Pika is a file directory.
* Sharding mode: In Sharding mode, the user's stored data collection is called a Table. Each Table is split into multiple shards, each called a Slot. For a given KEY, the hash algorithm determines which Slot it belongs to. All Slots and their replicas are distributed across all Pika instances according to a certain strategy, with each Pika instance holding some primary Slots and some slave Slots. In Sharding mode, the master/slave relationship is at the Slot level rather than the Pika instance level. A Slot's physical form on Pika is a file directory.

Pika's operating mode can be configured via the `instance-mode` configuration item in the configuration file, setting it to `classic` or `sharding`.

### 1. Classic (Master-Slave) Mode

![](https://raw.githubusercontent.com/simpcl/simpcl.github.io/master/PikaClassic.png)


### 2. Sharding Mode

![](https://raw.githubusercontent.com/simpcl/simpcl.github.io/master/PikaCluster.png)
