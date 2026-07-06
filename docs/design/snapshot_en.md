# Pika Snapshot Backup Solution

## Principle
Unlike Redis, Pika's data is primarily stored on disk, giving it a natural advantage for data backup — backups can be accomplished directly through file copying.

Implementation

![](http://ww4.sinaimg.cn/large/c2cd4307gw1f6m745csxsj20fl0iojss.jpg)
 
## Process
- Take a snapshot: Block writes (prevent clients from writing to the DB), and obtain the snapshot contents during this process.
- Asynchronous thread copies files: Copies snapshot files using a modified RocksDB BackupEngine, which prevents file deletion during this process.

## Snapshot Contents
- All file names of the current DB
- Size of the manifest file
- sequence_number
- Sync point
    - binlog filenum
    - offset
