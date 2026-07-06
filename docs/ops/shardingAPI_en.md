## Starting from Pika version 3.1.2, Pika has added a series of sharding support features, and a series of commands have been added for sharding mode.

In the Pika sharding version, we introduce the concept of Table. A Table can have multiple slots (the number of slots is limited by `default-slot-num` in the configuration file). Read/write requests from clients are mapped based on the key; if this Pika is responsible for that slot, the key is executed on the corresponding slot.

When started in sharding mode, Pika creates a default Table (named db0). Users can execute the `addslots` command to add slots to this Table; Pika records the Table's slot information in the `meta` file under the `db-path` directory.

Slots between different Pika instances can sync data. A slot's role can be master, slave, or both. Once a slot has a slave role, it is not writable.

### 1. `pkcluster info` command:
**Purpose:** Display sync information for slots (including Binlog offset, master/slave role, etc.)

`pkcluster info slot`: View sync info for all slots in the default table.

`pkcluster info slot db0:0-6,7,8`: View sync info for slots with IDs 0–6, 7, 8 in table0.

`pkcluster info table 1`: View info for table1, including QPS, number of table shards, etc.


### 2. `pkcluster addslots` command:

**Purpose:** Add slots with the specified IDs in the default table. IDs are in range [0, default-slot-num - 1]. Supports three ID specification syntaxes:

`pkcluster addslots 0-2`: Add slots with IDs 0, 1, 2 to the default table.

`pkcluster addslots 0-2,3`: Add slots with IDs 0, 1, 2, 3 to the default table.

`pkcluster addslots 0,1,2,3,4`: Add slots with IDs 0, 1, 2, 3, 4 to the default table.


### 3. `pkcluster delslots` command:

**Purpose:** Delete slots with the specified IDs in the default table. IDs are in range [0, default-slot-num - 1]. Supports three ID specification syntaxes:

`pkcluster delslots 0-2`: Delete slots with IDs 0, 1, 2 from the default table.

`pkcluster delslots 0-2,3`: Delete slots with IDs 0, 1, 2, 3 from the default table.

`pkcluster delslots 0,1,2,3,4`: Delete slots with IDs 0, 1, 2, 3, 4 from the default table.


### 4. `pkcluster slotsslaveof` command:

**Purpose:** Used to initiate or cancel data sync requests from certain slots in the default table to the corresponding slots on another Pika instance. The slot specification syntax is similar to addslots/delslots above, but this command also supports `all` to represent all slots in the default table.

`pkcluster slotsslaveof no one  [0-3,8-11 | all]` — Disconnect master-slave sync for the specified slots.

`pkcluster slotsslaveof ip port [0-3,8,9,10,11 | all]` — Establish master-slave sync for the specified slots.

`pkcluster slotsslaveof ip port [0,2,4,6,7,8,9 | all] force` — Perform full sync for the specified slots.

### 5. `slaveof` command

Equivalent to `pkcluster slotsslave ip port all`.

## Starting from Pika 3.3, sharding mode supports dynamic table creation. For backward compatibility and to reduce the learning curve for users not using multi-table, Pika automatically creates table 0 with the slot count from the configuration. To use other tables, they must be manually created.

### 1. `pkcluster addtable` command:

**Purpose:** Create a table. Specify table-id and default-slot-num when creating. Default table-id is 0.

`pkcluster addtable 1 64`: Create a table with table-id=1 and default-slot-num=64.

### 2. `pkcluster deltable` command:

**Purpose:** Delete the table with the specified table-id and all its slots.

`pkcluster deltable 1`: Delete the table with table-id=1 and all its slots.

### 3. `pkcluster addslots` command:

**Purpose:** Add slots with the specified IDs to the table with the given table-id. IDs are in range [0, default-slot-num - 1]. If table-id is not specified, adds to the default table.

`pkcluster addslots 0-2 1`: Add slots with IDs 0, 1, 2 to the table with table-id=1.

`pkcluster addslots 0-2,3 1`: Add slots with IDs 0, 1, 2, 3 to the table with table-id=1.

`pkcluster addslots 0,1,2,3,4 1`: Add slots with IDs 0, 1, 2, 3, 4 to the table with table-id=1.

### 4. `pkcluster delslots` command:

**Purpose:** Delete slots with the specified IDs from the table with the given table-id. IDs are in range [0, default-slot-num - 1].

`pkcluster delslots 0-2 1`: Delete slots with IDs 0, 1, 2 from the table with table-id=1.

`pkcluster delslots 0-2,3 1`: Delete slots with IDs 0, 1, 2, 3 from the table with table-id=1.

`pkcluster delslots 0,1,2,3,4 1`: Delete slots with IDs 0, 1, 2, 3, 4 from the table with table-id=1.

### 5. `pkcluster slotsslaveof` command:

**Purpose:** Initiate or cancel data sync requests from certain slots in the given table-id to the corresponding slots on another Pika instance. Also supports `all` to represent all slots in the specified table.

`pkcluster slotsslaveof no one  [0-3,8-11 | all] 1` — Disconnect master-slave sync for the specified slots in the table with table-id=1.

`pkcluster slotsslaveof ip port [0-3,8,9,10,11 | all] 1` — Establish master-slave sync for the specified slots in the table with table-id=1.

`pkcluster slotsslaveof ip port [0,2,4,6,7,8,9 | all] force 1` — Perform full sync for the specified slots in the table with table-id=1.


<br/>

## Notes:
### In sharding mode, Pika fully supports commands with a single key as input parameter.
### For commands that can accept multiple keys as input, partial support is provided in sharding mode.
<br/>

#### Commands currently not supported in sharding mode

| —           | —                 | —            |
| ----------- | ----------------- | ------------ |
| Msetnx      | Scan              | Keys         |
| Scanx       | PKScanRange       | PKRScanRange |
| RPopLPush   | ZUnionstore       | ZInterstore  |
| SUnion      | SUnionstore       | SInter       |
| SInterstore | SDiff             | SDiffstore   |
| SMove       | BitOp             | PfAdd        |
| PfCount     | PfMerge           | GeoAdd       |
| GeoPos      | GeoDist           | GeoHash      |
| GeoRadius   | GeoRadiusByMember |              |

#### Sharding mode command support plan
Infrastructure: Support [hash tags](https://redis.io/topics/cluster-spec) data sharding. Users can control data distribution across the cluster for their specific use cases.

Basic principle: For commands involving multiple keys, all keys must be on the same shard; only intra-shard operations are supported. Global-space commands will not be supported for now.

Example: For `RPOPLPUSH src dst`, both `src` and `dst` lists must be on the same shard. Users can achieve this by using hash tags.
