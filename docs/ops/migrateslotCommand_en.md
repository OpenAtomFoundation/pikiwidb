# Pika Support for Codis Sync Slot Migration Design

Starting from Pika 3.5.0, Pika supports the Codis + Pika cluster mode. The implementation replaces redis-server in Codis with pika-server and adds the concept of slot keys (default 1024, configurable via the configuration file). Using sets as an example: when a user modifies a key, the slot is calculated as `slot = crc32(key) % 1024`, and the key is added to the corresponding slot. During migration, a specified slot is used to `spop` one key at a time and migrate it to the target pika-server. The advantage of this implementation is that data can be migrated without any extra processing. The drawbacks are reduced performance (approximately 10%–20%, so we recommend disabling it when migration is not needed) and increased storage (approximately 30%). Pika supports a toggle for slot migration; when the toggle is off, slot migration operations are not supported; when it is on, slot migration is supported.

**In `conf/pika.conf`, you can set `slotmigrate` which defaults to `no`. Set it to `yes` when migration is needed:**

```bash
# slotmigrate [yes | no]
slotmigrate : no
```

**You can also configure it dynamically via the command line with `config set`:**

```bash
$ redis-cli -h 127.0.0.1 -p 9221 config set slotmigrate yes
```

## Pika Codis Migration Commands

##### slotsinfo

* **Description:** Get the number of slots and the size of each slot in Pika.

* **Parameters:** `slotsinfo [start] [count]`

  1. Default: query [0, MAX_SLOTNUM)
  2. start - Starting slot number
  3. count - Range size, i.e., the query range is [start, start + count)

* **Return:** Returns an array of slotinfo. The first row is `slotnum` (slot number); the second row is `slotsize` (number of data items in the slot).

  ```bash
  localhost:9221> slotsinfo 0 128
     1) 1) (integer) 23  
        2) (integer) 2
     2) 1) (integer) 29
        2) (integer) 1
  ```

##### slotsdel

* **Description:** Delete all key-values under specified slots in Pika.

* **Parameters:** `slotsdel slot1 [slot2 ...]` — accepts at least 1 slot number.

* **Return:** Returns the remaining size after deletion, typically 0.

  ```bash
  localhost:9221> slotsdel 1013 990
     1) 1) (integer) 1013
        2) (integer) 0
     2) 1) (integer) 990
        2) (integer) 0
  ```

##### slotsscan

* **Description:** Get the keys in a slot key; usage is similar to `sscan`. Available when `slotmigrate` is `yes`.

* **Parameters:** Similar to the `SCAN` command.

  1. slotnum - Slot number to query, [0, MAX_SLOT_NUM)
  2. cursor - See SCAN command
  3. [COUNT count] - See SCAN command

* **Return:** See SCAN command.

  ```bash
  localhost:9221> slotsscan 579 0 COUNT 10
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
  ```

##### slotsreload

* **Description:** Recalculate the slot for all keys.

* **Parameters:** No extra parameters needed.

  ```bash
  localhost:9221> slotsreload
       20230727234255 : 0
  ```

##### slotsreloadoff

* **Description:** Available when `slotmigrate` is `yes`. Stops the execution of the `slotsreload` command.

* **Parameters:** No extra parameters needed.

* **Return:** Returns OK.

  ```bash
  localhost:9221> slotsreloadoff
       OK
  ```

##### slotscleanup

* **Description:** Delete keys corresponding to the given slotIDs.

* **Parameters:** `slotscleanup slotID [multiple IDs allowed]`

  ```bash
  localhost:9221> slotscleanup 10 11 12 13 14 15
       1) (integer) 10
       2) (integer) 11
       3) (integer) 12
       4) (integer) 13
       5) (integer) 14
       6) (integer) 15
  ```

##### slotscleanupoff

* **Description:** Stop an ongoing background slot cleanup operation.

* **Parameters:** `slotscleanupoff`

* **Return:** Returns OK.

  ```bash
  localhost:9221> slotscleanupoff
       OK
  ```

## Data Migration

##### slotsmgrtslot

* **Description:** Randomly select 1 key-value from a slot and migrate it to the target machine.

* **Parameters:** `slotsmgrtslot host port timeout slot`

  1. host port - Target machine
  2. timeout - Operation timeout in ms
  3. slot - The slot number to migrate

* **Return:** Returns an integer.

  ```bash
  localhost:9221> set a 100            
     OK
  localhost:9221> slotsinfo           
     1) 1) (integer) 579
        2) (integer) 1
  localhost:9221> slotsmgrtslot 127.0.0.1 6380 100 579
       (integer) 1                      # Migration succeeded
  localhost:9221> slotsinfo
     (empty list or set)
  localhost:9221> slotsmgrtslot 127.0.0.1 6380 100 579 1
     (integer) 0                     # Migration failed; key no longer exists locally
  ```

##### slotsmgrttagslot

- **Description:** Randomly select 1 key-value from a slot and all key-values with the same tag as that key, and migrate them to the target machine.

- **Parameters:** `slotsmgrttagslot host port timeout slot`

  1. host port - Target machine
  2. timeout - Operation timeout in ms
  3. slot - The slot number to migrate

- **Return:** Returns an integer.

  ```bash
  localhost:9221> set a 100            
     OK
  localhost:9221> slotsinfo           
     1) 1) (integer) 579
        2) (integer) 1
  localhost:9221> slotsmgrttagslot 127.0.0.1 6380 100 579
       (integer) 1                      # Migration succeeded
  localhost:9221> slotsinfo
     (empty list or set)
  localhost:9221> slotsmgrttagslot 127.0.0.1 6380 100 579 1
     (integer) 0                     # Migration failed; key no longer exists locally
  ```

##### slotsmgrtone

- **Description:** Migrate a key to the target machine.

- **Parameters:** `slotsmgrtone host port timeout key`

  1. host port - Target machine
  2. timeout - Operation timeout in ms
  3. key - The specified key

- **Return:** Returns an integer.

  ```bash
  localhost:9221> set a{tag} 100          
       OK
  localhost:9221> slotsinfo
       1) 1) (integer) 579
          2) (integer) 1
  localhost:9221> slotsmgrtone 127.0.0.1 6380 100 a
       (integer) 1                      # Migration succeeded
  localhost:9221> slotsmgrtone 127.0.0.1 6380 100 a
       (integer) 0                      # Migration failed; key no longer exists locally
  ```

##### slotsmgrttagone

- **Description:** Migrate all keys with the same tag as the specified key to the target machine.

- **Parameters:** `slotsmgrttagone host port timeout key`

- **Return:** Returns an integer.

  ```bash
  localhost:9221> set a{tag} 100        # set <a{tag}, 100>
       OK
  localhost:9221> set b{tag} 100        # set <b{tag}, 100>
       OK
  localhost:9221> slotsmgrttagone 127.0.0.1 6380 1000 {tag}
       (integer) 2
  localhost:9221> scan 0                # Migration succeeded; keys no longer exist locally
       1) "0"
       2) (empty list or set)
  localhost:6380> scan 0                # Data successfully migrated to target machine
       1) "0"
       2) 1) "a{tag}"
          2) "b{tag}"
  ```

**slotsmgrttagslot-async**

- **Description:** Semi-async mode; asynchronously migrates a specified number of keys from a specified slot to the target machine.
- **Parameters:** `slotsmgrttagslot-async host port timeout maxbulks maxbytes slot numkeys`

```bash
localhost:9221> slotsmgrttagslot-async pika 9221 5000 200 33554432 518 500
     1) (integer) 0
     2) (integer) 0
```

**slotsmgrt-exec-wrapper**

- **Description:** Semi-async mode; handles requests whose key belongs to a slot that is currently being migrated.
- **Parameters:** `slotsmgrt-exec-wrapper $hashkey $command [$arg1 ...]`
- **Return:**
  1. If the key does not exist, returns 0 and info indicating the key does not exist.
  2. If the key is currently being migrated, returns 1 and info indicating the key is being migrated.

```bash
localhost:9221> slotsmgrt-exec-wrapper my-hash set my-hash 100
```

**slotsmgrt-async-status**

- **Description:** Semi-async mode; view migration status.
- **Parameters:** `slotsmgrt-async-status`
- **Return:**
  1. Target machine
  2. Slot number
  3. Migration status
  4. Moved keys
  5. Remaining keys

```bash
localhost:9221> slotsmgrt-async-status
     1) "dest server: :-1"
     2) "slot number: -1"
     3) "migrating  : no"
     4) "moved keys : -1"
     5) "remain keys: -1"
```

**slotsmgrt-async-cancel**

- **Description:** Semi-async mode; stop an ongoing migration at the Pika layer.
- **Parameters:** `slotsmgrt-async-cancel`
- **Return:** Returns OK.

```bash
localhost:9221> slotsmgrt-async-cancel
     OK
```

## Debugging

##### slotshashkey

- **Description:** Calculate and return the slot number for the given keys.

- **Parameters:** `slotshashkey key1 [key2 ...]`

- **Return:** Returns an array.

  ```bash
  localhost:9221> slotshashkey a b c   # Calculate slot numbers for <a, b, c>
       1) (integer) 579                # Slot number for the corresponding key
       2) (integer) 1017
       3) (integer) 879
  ```
