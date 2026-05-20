# codis2pika

codis2pika is a tool for migrating data from Codis to Pika. Its main purpose is to support migrating from Codis sharding mode to Pika classic mode.

## Acknowledgements

codis2pika is inspired by and based on the open-source redis-shake project from Alibaba, with customized modifications. Therefore, the basic feature set is consistent with the original tool, but there are functional differences.

## Features

Features shared with the original:

* 🤗 Supports custom filtering rules using Lua (this part was not modified and thus not actually tested, but is theoretically supported)
* 💪 Supports large instance migration

Features specific to codis2pika:
* 🌐 Supports standalone source instance to standalone target instance
* 🌲 Only supports Redis's 5 basic data structures
* ✅ Tested against Codis server running Redis 3.2
* ⏰ Supports long-running real-time data synchronization with a few seconds of latency
* ✊ Agnostic to the underlying storage mode of the instance

### Change Notes
* Cluster mode not supported: Because data distribution in the source instance (Codis sharding mode) and target instance (Pika classic mode) differs at the underlying level, allocation must be based on actual business requirements. If needed, the corresponding algorithm can be added to restore the cluster write interface.
* For Redis-to-Redis migrations, it is recommended to use [RedisShake](https://github.com/alibaba/RedisShake), which has more comprehensive functionality. This project is primarily intended to support migration from sharding-mode instances to Pika classic instances.

# Documentation

## Installation

### Download from Release

Release: [https://github.com/GetuiLaboratory/codis2pika/releases](https://github.com/GetuiLaboratory/codis2pika/releases)

### Build from Source

After downloading the source code, run `sh build.sh` to compile.

```shell
sh build.sh
```

## Running

1. Edit `codis2pika.toml` and modify the `source` and `target` configuration items.
2. Start codis2pika:

```shell
./bin/codis2pika codis2pika.toml
```

3. Monitor the data synchronization status.

## Configuration

Refer to `codis2pika.toml` for the configuration file. To avoid ambiguity, every configuration item in the file is required to have a value; otherwise an error will be reported.

## Data Filtering

codis2pika supports custom filtering rules using Lua scripts to filter data. When using a Lua script, start codis2pika with:

```shell
./bin/codis2pika codis2pika.toml filter/xxx.lua
```
The Lua data filtering feature has not been verified. If needed, please refer to the redis-shake project.

## Notes
* Extra-large keys are not supported.
* The Codis instance being migrated needs to have `client-output-buffer-limit` configured to remove restrictions; otherwise the connection will be dropped.
* The node being migrated needs to reserve memory headroom equal to the amount of data being migrated.
* It is recommended to perform a compaction before and after the data migration.
* Multiple DBs are not supported (not considered necessary enough to implement at this time).
* The expiration time of some keys may be pushed back.

## Migration Process Visualization
It is recommended to configure a monitoring dashboard in advance to have a visual overview of the migration process.

## Verifying Migration Results

It is recommended to use Alibaba's open-source `redis-full-check` tool.
