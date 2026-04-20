# Full Sync
## Background
### 1. Pika Replicate
- Pika supports master/slave replication, triggered by the `slaveof` command on the slave side.
- The slave processes the `slaveof` command, changes its state to slave, and changes the connection state.
- The slave sends a MetaSync request to the master, ensuring its own DB topology is consistent with the master before syncing.
- Each partition under the slave independently sends a trysync request to the corresponding partition on the master to establish a sync relationship.
### 2. Binlog
- Pika's sync relies on binlog.
- Binlog files are automatically or manually deleted.
- When the binlog file corresponding to a sync point no longer exists, a full sync is needed to synchronize data.
## Full Sync
### 1. Introduction
- When a full sync is needed, the master dumps the DB files and sends them to the slave.
- DB file transfer is implemented via rsync daemon mode.
- By default, pika port+1000 is used as the rsync transfer port.
### 2. Implementation Logic
1. When a Pika instance starts, the Rsync service is also started.
2. When the master finds that a partition needs a full sync, it checks whether a backup file is available. If not, it dumps one first.
3. The master sends the dump files for the corresponding partition to the slave via rsync.
4. The corresponding partition on the slave replaces its DB with the received files.
5. The slave's corresponding partition re-sends a trysync with the latest offset.
6. Sync is complete.

Establishing sync for a partition on the Slave:
![slave's partition](https://i.imgur.com/flnOyeZ.png)

Master handling the sync request:
![master execution process](https://i.imgur.com/Beclo9c.png)
### 3. Slave Connection States
- No Connect: Does not attempt to become a slave of any other node.
- ShouldMetaSync: Requests DB topology information from the master to ensure consistency with itself.
- TryConnect: Resets the state machine for each partition, putting it into a ready-to-sync state.
- Connecting: Remains in the connecting state until all partitions have established sync relationships.
- EstablishSucces: All partitions have successfully established sync relationships.
- Error: An exception has occurred.


# Incremental Sync

## Background:
<br/></br>
The slave Pika obtains the full DB structure from the master, then performs Trysync at the partition level. If the slave confirms incremental sync is possible, it will proceed with incremental sync at the partition level. By default, pika port+2000 is used for incremental sync.

### Binlog Structure:
<br/></br>
Pika's master-slave sync uses Binlog. In a 1-master-N-slave structure, the master node can reuse a single Binlog for multiple slaves, with each slave having its own offset in the binlog. After the master executes a write command, the command is appended to the Binlog. Pika's sync module reads the corresponding binlog and sends it to the slave. The slave receives the binlog, executes it, and appends it to its own Binlog. Since master and slave offsets are the same, upon network or node failure requiring reconnection, the slave only needs to send its current Binlog offset to the master. The master then starts syncing subsequent commands from that offset. Theoretically, commands could be appended to a file one by one without processing, but this format is very error-prone — if one byte is wrong, the entire file becomes unusable. Therefore, Pika uses a format similar to leveldb log for storage, as follows:

![image](http://ww2.sinaimg.cn/large/c2cd4307gw1f6m74717b3j20rm0gjwgw.jpg)

### Interaction Process:
<br/></br>
1. The slave sends a BinlogSyncRequest packet, specifying the BinlogOffset it has already received.

2. The master receives the BinlogSyncRequest and sends a batch of BinlogSyncResponses starting from the sync point.

3. After receiving BinlogSyncResponse, the slave writes to the local binlog and then repeats step 1.

![image](https://i.imgur.com/JVfTV22.png)

## Sync Module:
<br/></br>
![image](https://i.imgur.com/5ByKpsA.png )

Pika's sync is handled by the ReplicaManager (RM) module. RM has a two-layer structure: the logic layer handles sync logic, and the transport layer handles connection management, data parsing, and transmission.

The minimum unit of data sync is a Partition. Each Pika instance maintains its own primary partitions (MasterPartitions) and slave partitions (SlavePartitions). For MasterPartitions, it records the sync information of following slaves, and the logic layer syncs information to slaves based on this. For SlavePartitions, it records master information, and the logic layer sends sync requests to the master as needed.

The logic layer maintains two data structures: one is MasterPartitions, which records following SlaveNode information (mainly the slave's sync state and current sessionId); the other is SlavePartitions, which records master information.

The transport layer is divided into two sub-modules: ReplicationClient is responsible for initiating connections, and ReplicationServer is responsible for response packets. All partitions between any two instances share a single connection.



## Sync Process:
<br/></br>
![image](https://i.imgur.com/1Q8PbjF.png )


### MasterPartition Sync Event
The logic layer handles MasterPartition sync events, syncing data to corresponding slaves.

1. After reading MasterPartition Binlog information, the BinlogOffsetInfo is recorded in the SlaveNode's own window.

2. The Binlog is temporarily stored in a pending send queue.

3. The auxiliary thread (Auxiliary thread) periodically sends data from the temporary pending send queue to the corresponding slave node via RM's transport layer.

4. Upon receiving the slave's BinlogSyncResponse, obtaining the slave's received BinlogOffset information, updating the SlaveNode window, and repeating step 1 to continue syncing.

To control each SlaveNode's sync speed and avoid a few SlaveNodes consuming too many resources, a window is set for each SlaveNode. As shown below, Pika received ack responses for BinlogOffset 100 to 200, removes elements with BinlogOffset 100 to 200 from the window, then continues sending BinlogOffset 1100 and 1200, adding the corresponding BinlogOffset to the window.



![image](https://i.imgur.com/0GtOhk4.png)


### SlavePartition Sync Event
The logic layer handles SlavePartition sync events, receives sync data sent by the master, and sends corresponding response information to the master.

1. Based on parsed Partition information, binlog write tasks are assigned to corresponding threads.

2. After the thread writes binlog, it calls the transport layer to send BinlogSyncResponse.

3. Based on the binlog's key, write-to-DB tasks are assigned to corresponding threads.
