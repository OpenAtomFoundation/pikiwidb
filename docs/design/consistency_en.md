### Current Thread Model

Non-consistency scenario

1. Client requests lock, writes to DB and binlog file.

2. Return result to client.

3. Send BinlogSync request to sync to slave.

4. Slave returns BinlogSyncAck reporting sync status.

![](https://s1.ax1x.com/2020/03/27/GPVfBj.png)

Consistency scenario

1. Client request first writes to binlog file.

2. Send BinlogSync request to sync to slave.

3. Slave returns BinlogSyncAck reporting sync status.

4. Write the corresponding request to DB.

5. Return result to client.

![](https://s1.ax1x.com/2020/03/27/GPVINq.png)



<br>

### Binlog Header Changes

```
/*
* *****************Type First Binlog Item Format******************
* |<Type>|<CreateTime>|<Term Id>|<Logic Id>|<File Num>|<Offset>|<Content Length>|<Content>|
* |  2   |      4     |    4    |     8    |    4     |   8    |       4        |   ...   |
* |----------------------------------------- 34 Bytes ------------------------------------|
*/
```

TermId and Logic Id use the concepts of term and log index from the [Raft](https://raft.github.io/raft.pdf) paper. See the paper for details.

File Num and Offset are the offsets of this binlog entry within the file.

Pika's Binlog exists to ensure incremental sync between master and slave, while Raft Log exists to ensure data consistency between Leader and Follower. In a sense, these two "Logs" are the same concept, so in implementation, binlog and Raft Log are reused as a single log. The current binlog header's Term Id and Logic Id belong to Raft Log (abbreviated as log) information, while File Num and Offset belong to Binlog information.

<br>

### Three Phases of the Consistency Protocol

Log recovery and replication broadly follow the operations described in the Raft paper; detailed explanation is omitted here. In implementation, there are three phases: log replication, log recovery, and log negotiation.

Readers familiar with the Raft protocol may notice that these three phases are not entirely the same as Raft log replication. In Pika's implementation, based on the existing code structure, the logic for rolling back log positions between Leader and Follower is extracted separately, forming the Pika Trysync state. Any error in log replication causes Pika to terminate the current log replication (BinlogSync) state and transitions the state machine to the Trysync state. Leader and Follower then enter log negotiation logic. After successful negotiation, they transition to log replication logic.

![](https://s1.ax1x.com/2020/03/27/GPVl7R.png)
<br>
### Log Replication

The logical structure of the log is as follows: the upper part shows possible log positions for the Leader; the lower part shows possible log positions for the Follower.

![](https://s1.ax1x.com/2020/03/27/GPVH3T.png)

1. The logic of log replication can be referenced from the Raft protocol. Here we illustrate the flow of a log entry from client request to response.

Leader Status:

Committed Index : 10

Applied Index：8

Last Index: 15


<br>
Follower Status:

Committed Index : 7

Applied Index：5

LastIndex: 12

2. When the Leader sends logs 13-15 to the Follower, the Follower's status is updated as follows:

Follower Status:

Committed Index : 10

Applied Index：5

LastIndex: 15

At this point, logs 6-10 can be applied to the state machine. However, logs 11-15 can only be updated when the next Leader Committed Index greater than 15 is received. If the client has no more writes, the Follower's Committed Index can be updated via ping messages (which carry the Leader's committed index).

3. When the Leader receives the ack information from the Follower, the Leader's status is updated as follows:

Leader Status:

Committed Index : 15

Applied Index: 8

Last Index: 15

At this point, logs 9-15 can all be applied to the state machine (written to DB). After log 9 is written to DB, it is returned to the client, and the Applied Index is updated to 9. Log 9 can then be returned to the client.

For slaves, the overall log replication logic still follows the Raft paper. The only difference is that the log rollback logic in the paper is handled in the log negotiation phase.


<br>

### Log Recovery:

When restarting Pika, the previous consistency state is restored based on persisted consistency information (such as applied index).


<br>

### Log Negotiation:

In this phase, the Follower node proactively initiates the Trysync process, carrying Last Index, and sends a negotiation sync position request to the Leader. The negotiation process is as follows:

The Follower sends its last_index to the Leader. The Leader uses the Follower's last_index to determine whether it can find the corresponding log at that position. If found and the two logs are consistent, the Leader returns OK and negotiation ends. If not found, or if the logs are inconsistent, the Leader sends hints to the Follower. Hints are the most recent logs in the Leader's local storage. The Follower uses the hints to roll back its local logs, updates its last_index, and re-negotiates with the master. Eventually, the Leader and Follower reach agreement, ending the TrySync process, and entering the log replication process.

![](https://s1.ax1x.com/2020/03/27/GPVbgU.png)

Pseudocode for Leader log negotiation:

```c++
Status LeaderNegotiate() {
  reject = true
  if (follower.last_index > last_index) {
    send[last_index - 100, last_index]
  } else if (follower.last_index < first_index) {
    need dbsync
  }
  if (follower.last_index not found) {
    need dbsync
  }
  if (follower.last_index found but term not equal) {
    send[found_index - 100, found_index]
  }
  reject = false
  return ok;
}
```



Pseudocode for Follower log negotiation:

```c++
Status FollowerNegotiate() {
  if last_index > hints[hints.size() - 1] {
    TruncateTo(hints[hints.size() - 1]);
  }
  for (reverse loop hints) {
    if (hint.index exist && hint.term == term) {
      TruncateTo(hint.index)
      send trysync with log_index = hint.index
      return ok;
    }
  }
  // cant find any match
  TruncateTo(hints[0])
  send trysync with log_index = last_index
}
```



The above describes the three specific phases of the log. The overall logic follows the Raft paper design, with implementation adjustments made according to Pika's current code structure.



### On Leader Election and Membership Changes

Currently, leader election requires manual administrator intervention. See the [Replica Consistency Usage Documentation](https://github.com/Qihoo360/pika/wiki/%E5%89%AF%E6%9C%AC%E4%B8%80%E8%87%B4%E6%80%A7%E4%BD%BF%E7%94%A8%E6%96%87%E6%A1%A3) for details.

Membership change functionality is not currently supported.
