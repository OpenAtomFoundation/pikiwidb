
# How to Upgrade to Pika 3.0


## Upgrade Preparation:  
In version 2.3.3, Pika added server-id verification to ensure replication reliability. Therefore, Pika 2.3.3–2.3.6 cannot sync with versions before 2.3.3.

* If your Pika version is < 2.3.3: You need to prepare both the pika 2.3.6 and pika 3.0.16 binaries. Note that for 3.0.x, you need version 3.0.16 or later (or version 3.0.6). Other 3.0.x versions can no longer sync with older versions (2.3.X), so it is recommended to use the latest 3.0.x version. The instructions below use 3.0.16 as the example.
* If your Pika version is >= 2.3.3: You only need to prepare the pika 3.0.16 binary.
* If your Pika version >= 2.3.3, start from step 5 below; otherwise start from step 1.
* If your Pika is non-clustered (standalone) and cannot be taken offline for the upgrade, add a slave to the Pika instance before proceeding.

## Upgrade Steps:
1. Add the `masterauth` parameter to the slave's configuration file. Note: this value must match the `requirepass` parameter on the master, otherwise authentication will fail.
2. Replace the old Pika binary directory with the new one (recommended: 2.3.6).
3. Shut down both the master and slave, then start them with the new binary files.
4. Log into the slave and restore the master-slave relationship (skip this step if `slaveof` is already configured in the slave's config file). Confirm the sync status is "up".
5. Deploy pika 3.0.16 on the server.
6. Log into the slave and execute `bgsave`. You will find a fresh backup in the slave's dump directory. Ensure the `info` file in the backup directory is not lost — you may want to copy its contents somewhere safe.
7. Use the `nemo_to_blackwidow` tool in pika 3.0.16's `tools` directory to read the backup data and generate data files compatible with pika 3.0.16's new engine. Usage:
```
./nemo_to_blackwidow nemo_db_path(path to the backup directory to read) blackwidow_db_path(path to generate the new data files) -n (number of threads; configure based on server performance to avoid excessive resource consumption)

Example: If the old pika directory is pika-demo and the 3.0.16 pika directory is pika-new30:
./nemo_to_blackwidow /data/pika-demo/dump/backup-20180730 /data/pika-new30/new_db -n 6
```
8. Update the configuration file: set the converted directory (`/data/pika-new30/new_db`) as the startup directory of the new Pika (the `db-path` parameter), and set `identify-binlog-type` to `old` to ensure 3.0.16 can parse sync data from the old version. If the config file contains `slaveof` information, comment it out. Leave all other settings unchanged.
9. Shut down the current slave on this machine (`/data/pika-demo`) and start the new Pika (`/data/pika-new30/new_db`) with pika 3.0.16's binary.
10. Log into pika 3.0.16 and establish a sync relationship with the master. Open the `info` file from the previously saved backup directory — lines 4 and 5 contain the binlog position when the backup was taken. Use this information in the `slaveof` command for incremental sync (ensure the master's corresponding binlog file still exists before executing):
```
Example: Suppose the info file contains the following, and the master IP is 192.168.1.1, port 6666:
3s
192.168.1.2
6666
300
17055479
Then the slaveof command should be:
slaveof 192.168.1.1 6666 300 17055479
```
11. Observe whether the sync status is "up". Once confirmed and the slave has caught up with zero lag, the slave upgrade is complete. You can then proceed with the master cutover:
```
a. Disable the slave's slave-read-only parameter to allow writes to the slave.
b. On the application side, change the connection IP to the slave's address to redirect all traffic to the slave.
c. Disconnect the slave from the master (slaveof no one) to complete the master cutover.
```
12. Use `config set` to set `identify-binlog-type` to `new`, and update the configuration file accordingly. If `config set` reports an error, it means you skipped step 11c.
13. At this point, the upgrade is complete. You now have a standalone pika 3.0.16 instance. To add a slave, simply start a new empty pika 3.0.16 (or later) instance and use `slaveof ip:port force` to easily create a master-slave cluster. This command will perform a full sync and then automatically switch to incremental sync.

## Notes:
* Since Pika 3.0 redesigned the data storage format in its engine, and the new engine is more space-efficient than the old one, it is normal to find that the Pika 3.0 DB is smaller than the original after upgrade.
* For large data volumes, the `nemo_to_blackwidow` tool may take a long time to convert the DB from Nemo format to blackwidow format. During this time, the binlog position corresponding to the master's dump may have been purged, preventing incremental sync afterward. To avoid this, increase the master's binlog retention period before upgrading (modify `expire-logs-days` and `expire-logs-nums`).
* In Pika 3.0, the binlog format was modified. To maintain backward compatibility, the `identify-binlog-type` option is provided. This option only takes effect when the Pika instance is acting as a slave. When set to `new`, the binlog received from the master is parsed in the new format (pika 3.0+). When set to `old`, it is parsed in the old format (pika 2.3.3 – pika 2.3.6).
* Pika 3.0 changed the binlog format. The new version records more data while being more disk-efficient. When determining whether the slave has caught up during the migration from pika 2.3.6 to pika 3.0, do not compare `binlog_offset` between master and slave. Instead, check whether the `lag` for the slave in the `Replication` section of `info` on the master is close to 0.


# How to Upgrade to Pika 3.1 or 3.2

# Migration Tools Introduction
## Manifest Generation Tool
* Tool path: `./tools/manifest_generator`
* Purpose: Generate a manifest file.
## Incremental Sync Tool
* Tool path: `./tools/pika_port`
* Purpose: Perform incremental data sync between Pika 3.0 and the new Pika 3.1 or 3.2.

# Notes
1. To improve Pika's single-node performance, starting from Pika 3.1, we implemented Redis's multi-DB mode inside Pika. As a result, the directories for the underlying storage DB and logs have changed. If the old version Pika's db-path is `/data/pika9221/db`, all single-DB data was stored directly in that directory. Since we now support multiple DBs, there is an extra directory level, so you need to manually move the old single-DB data to `/data/pika9221/db/db0` during migration.
2. To improve multi-DB sync efficiency, the new version of Pika uses the PB protocol for inter-instance communication. This means the new version cannot directly establish a master-slave relationship with the old version. Therefore, you need [pika_port](https://github.com/Qihoo360/pika/wiki/pika%E5%88%B0pika%E3%80%81redis%E8%BF%81%E7%A7%BB%E5%B7%A5%E5%85%B7) to incrementally sync data from the old Pika to the new Pika.

# Upgrade Steps
1. Configure the new version Pika's configuration file based on your scenario (the `databases` item specifies how many DBs to enable).
2. Log into the master and execute `bgsave`, then copy the dumped data to the `db0` subdirectory under the new version Pika's `db-path`.
```
Example:
    Old Pika dump path: /data/pika_old/dump/20190517/
    New Pika db-path: /data/pika_new/db/
    Execute: cp -r /data/pika_old/dump/20190517/ /data/pika_new/db/db0/
```
3. Use the `manifest_generator` tool to generate a manifest file in the `log_db0` subdirectory of the new Pika's configured log directory. This allows the new Pika to have the same binlog offset as the old Pika. Specify the `db-path/db0` directory and `$log-path/log_db0` directory (essentially merging the old DB and log into the new db0):
```
Example:
    New Pika db-path: /data/pika_new/db/
    New Pika log-path: /data/pika_new/log/
    Execute: ./manifest_generator -d /data/pika_new/db/db0 -l /data/pika_new/log/log_db0
```
4. Start Pika with the v3.1.0 binary and corresponding configuration file. Use `info log` to check db0's binlog offset (filenum and offset).
5. Use the pika_port tool to incrementally sync data from the old Pika to the new Pika:
```
Example:
    Old Pika IP: 192.168.1.1, Port: 9221
    New Pika IP: 192.168.1.2, Port: 9222
    Local IP of the machine running pika_port: 192.168.1.3, port to use: 9223
    filenum: 100, offset: 999

    Execute: ./pika_port -t 192.168.1.3 -p 9223 -i 192.168.1.1 -o 9221 -m 192.168.1.2 -n 9222 -f 100 -s 999 -e
```
6. While syncing with pika_port, log into the master and run `info replication` — you will see a new slave entry, which is the pika_port tool acting as a slave. You can also check the lag to monitor latency.

7. When we perform incremental sync to pika 3.1 or pika 3.2, we can add slave instances to pika 3.1 or pika 3.2. Once all slaves have caught up and the lag from the source is 0 or very small, we can replace the entire cluster, taking the old cluster offline and bringing the new cluster online.


# Important Notes
1. When copying the dump directory, it is best to rename it first (mv), then do the remote sync. This prevents data inconsistency caused by the dump directory being overwritten during copying.
2. The `manifest_generator` tool requires the `info` file generated during the dump. Make sure the `info` file is also moved to the specified directory when copying the dump directory.
3. When using `manifest_generator`, if the `$log-path/db0` directory already exists, it will report an error. Do not manually create the db0 directory — the script will create it automatically.
4. The `pika_port` incremental sync tool needs the IP, port, and file offset from the `info` file to sync.
5. `pika_port` simulates a slave interacting with the source. It will perform a trysync; if the requested offset has expired on the source, it will trigger a full sync and automatically use the directory specified by the `-r` parameter to store the rsync full sync data. If you don't want automatic full sync, use the `-e` parameter — it will return -1 when a full sync is triggered, but rsync will still continue. You need to kill the local rsync process to stop the full sync on the source side.
6. `pika_port`'s incremental sync is continuous and will not stop on its own. During this time, you can log into the source and use `info replication` to check the slave's lag.
7. `pika_port` supports multi-threaded operation. Keys are hashed to threads; the same key always goes to the same thread to ensure ordering per key.
8. For large data volumes, copying the dump directory may take a long time, causing the binlog position corresponding to the master's dump to be purged (also watch out for this when adding slaves to pika 3.1 or pika 3.2), making incremental sync impossible. To avoid this, increase the master's binlog retention period before upgrading (modify `expire-logs-days` and `expire-logs-nums`).
9. If you do not use `manifest_generator` to generate a manifest file, that is also fine, but the pika 3.1 or pika 3.2 instance's offset will start at 0 0. If you later attach slaves to pika 3.1 or pika 3.2, you will need to run `slaveof IP PORT force` on the slave; otherwise the slave may end up with missing data.
