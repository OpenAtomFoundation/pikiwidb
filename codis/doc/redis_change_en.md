### Redis Modifications (Additional Commands)
--------------------------------

##### SLOTSINFO [start] [count]

+ Description: Returns slot metadata in Redis, including slot ID and key count for each slot.

+ Parameters: Defaults to querying `[0, MAX_SLOT_NUM)`.

  - `start`: starting slot ID (default `0`)
  - `count`: size of the range to query, i.e. `[start, start + count)` (default `MAX_SLOT_NUM`)

+ Return: an array of `slotinfo` items, where each `slotinfo` is `[slotnum, slotsize]`.

        response := []slotinfo{slot1, slot2, slot3, ...}
        slotinfo := []int{slotnum, slotsize}

        where:
            INT slotnum  : slot ID
            INT slotsize : number of keys in the slot

+ Example:

        localhost:6379> slotsinfo 0 128
            1) 1) (integer) 23
               2) (integer) 2
            2) 1) (integer) 29
               2) (integer) 1

##### SLOTSSCAN slotnum cursor [COUNT count]

+ Description: Scans keys in a specific slot.

+ Parameters: Similar to `SCAN`.

    - `slotnum`: slot ID to scan, in `[0, MAX_SLOT_NUM)`
    - `cursor`: same semantics as `SCAN`
    - `[COUNT count]`: same semantics as `SCAN`
        - `MATCH` is currently not supported

+ Return: same format as `SCAN`.

    - Returns updated cursor and a key list.

+ Example:

        localhost:6379> slotsscan 579 0 COUNT 10
            1) "10752"
            2)  1) "{a}7836"
                2) "{a}2167"
                3) "{a}5332"
                4) "{a}6292"
                5) "{a}600"
                6) "{a}6094"
                7) "{a}7754"
                8) "{a}4929"
                9) "{a}9211"
               10) "{a}6596"

##### SLOTSDEL slot1 [slot2 …]

+ Description: Deletes all key-value pairs in one or more slots.

+ Parameters: Accepts at least one slot ID.

+ Return: Same structure as `slotsinfo`; `slotsize` means remaining keys after deletion (usually `0`).

+ Example:

        localhost:6379> slotsdel 1013 990
            1) 1) (integer) 1013
               2) (integer) 0
            2) 1) (integer) 990
               2) (integer) 0

#### Data Migration
---------------

**The following 4 commands are a migration family:**

+ `SLOTSMGRTSLOT` - *O(1)*

    Randomly migrates one key-value pair from a slot to the target node.

+ `SLOTSMGRTONE` - *O(1)*

    Migrates the specified key-value pair to the target node.

+ `SLOTSMGRTTAGSLOT` - *O(log(n))*

    Randomly picks one key from a slot and migrates all key-value pairs with the same tag.

+ `SLOTSMGRTTAGONE` - *O(log(n))*

    Migrates all key-value pairs that share the same tag as the specified key.

##### SLOTSMGRTSLOT host port timeout slot

+ Description: Randomly migrates one key-value pair from the specified slot to the target node (synchronous I/O).

    - Returns `0` if the slot is empty or the selected key expired.
    - If the slot still has keys, one key is selected and migrated.
    - Also returns the remaining key count in the slot.
    - Migration triggers `slotsrestore` on the destination and **overwrites existing values**.

+ Parameters:

    - `host:port`: destination node

        Redis caches the connection to `host:port` for 30s, and closes it on timeout/error.

    - `timeout`: timeout in milliseconds

        The operation includes three synchronous stages:

        1. connect (may use cached connection)
        2. send key-value data
        3. receive response from destination

        Each stage is bounded by `timeout`.

    - `slot`: slot ID to migrate from

+ Return: integer array

        response := []int{succ,size}

        where:
            INT succ : migration result
                0 -> slot is empty (migrated keys = 0)
                1 -> one key migrated and removed locally (migrated keys = 1)
            INT size : remaining key count in the slot

+ Example:

        localhost:6379> set a 100            # set <a, 100>
            OK
        localhost:6379> slotsinfo            # slot size = 1
            1) 1) (integer) 579
               2) (integer) 1
        localhost:6379> slotsmgrt 127.0.0.1 6380 100 579
            (integer) 1                      # migrated successfully
        localhost:6379> slotsinfo
            (empty list or set)
        localhost:6379> slotsmgrt 127.0.0.1 6380 100 579 1
            (integer) 0                      # migrated count = 0; slot already empty

##### SLOTSMGRTONE host port timeout key

+ Description: Migrates the specified key to the destination. Semantics are similar to `slotsmgrtslot`.

+ Parameters: see `slotsmgrtslot`.

+ Return: integer

        response := int(succ)

        where:
            INT succ : same semantics as `slotsmgrtslot`

+ Example:

        localhost:6379> set a 100            # set <a, 100>
            OK
        localhost:6379> slotsinfo
            1) 1) (integer) 579
               2) (integer) 1
        localhost:6379> slotsmgrtone 127.0.0.1 6380 100 a
            (integer) 1                      # migration succeeded
        localhost:6379> slotsmgrtone 127.0.0.1 6380 100 a
            (integer) 0                      # skipped: key does not exist locally

##### SLOTSMGRTTAGONE host port timeout key

+ Description: Migrates all keys that share the same tag as `key`.

    - If `key` has no valid tag, it degrades to `slotsmgrtone`, with complexity ***O(1)***.
    - If `key` has a valid tag, it hashes the tag and finds all matching keys in skiplist, then migrates them atomically, with complexity ***O(log(n))***.
    - Note: In the modified Redis, tagged keys are organized in a skiplist by tag hash. Migration by tag may include more keys with the same hash. This design reduces string comparisons during tag migration and improves performance.

+ Parameters: see `slotsmgrtone`.

+ Return: integer

        response := int(succ)

        where:
            INT succ : number of keys migrated successfully

+ Example:

        localhost:6379> set a{tag} 100        # set <a{tag}, 100>
            OK
        localhost:6379> set b{tag} 100        # set <b{tag}, 100>
            OK
        localhost:6379> slotsmgrttag 127.0.0.1 6380 1000 {tag}
            (integer) 2
        localhost:6379> scan 0                # migrated, no local data
            1) "0"
            2) (empty list or set)
        localhost:6380> scan 0                # data appears on destination
            1) "0"
            2) 1) "a{tag}"
               2) "b{tag}"

##### SLOTSMGRTTAGSLOT host port timeout slot

+ Description: Tag-based migration counterpart of `slotsmgrtslot`.

    - Refer to `slotsmgrtslot` and `slotsmgrttagone` for behavior details.

##### SLOTSRESTORE key1 ttl1 val1 [key2 ttl2 val2 …]

+ Description: Extension of Redis 2.8 `restore` command.

    - Supports restoring multiple key-value pairs at once.
    - The operation is atomic.

+ Note: Unlike `restore`, `slotsrestore` only supports `replace`, i.e. it always **overwrites old values**. If old values already exist, it is usually a bug in redis-slots or proxy implementation, and Redis logs a conflict record.

#### Debug Commands
---------------

##### SLOTSHASHKEY key1 [key2 …]

+ Description: Computes and returns slot IDs of input keys.

+ Parameters: one or more keys.

+ Return: array

        response := []int{slot1, slot2...}

        where:
            INT slot : slot ID of the key, i.e. hash32(key) % NUM_OF_SLOTS

+ Example:

        localhost:6379> slotshashkey a b c   # compute slot IDs of <a,b,c>
            1) (integer) 579
            2) (integer) 1017
            3) (integer) 879

##### SLOTSCHECK

+ Description: Performs slot consistency checking in Redis. It verifies:

    - Every key recorded in a slot exists in DB.
    - Every DB key can be found in its corresponding slot.

+ Parameters: no arguments.

+ Return: string `OK` on success (or `ERR` with related key when check fails).

+ Example:

        localhost:6379> set a 100            # set <a, 100>
            OK
        localhost:6379> slotscheck
            OK                               # check passed
        …
        localhost:6379> slotscheck
            OK                               # check passed, took 1.07s
            (1.07s)

+ **Note**: This operation is relatively slow and should be used only as a development/debugging tool, not in production.
