Blackwidow is essentially a wrapper around RocksDB, enabling the KV-only RocksDB to support multiple data structures. Currently, Blackwidow supports five data structure types: String (essentially storing key-value), Hash, List, Set, and ZSet. Since RocksDB's storage format is only KV, all five data structures are ultimately stored on disk as RocksDB KV pairs. Below we show the relationship between Blackwidow and RocksDB and explain how KV is used to simulate multiple data structures.
![](https://i.imgur.com/nqeliuv.png)

## 1. String Structure Storage
String is essentially a key-value pair. We know RocksDB natively supports KV storage. To implement Redis's `expire` functionality, we append 4 Bytes at the end of the value to store the timestamp. The final RocksDB on-disk KV format is as follows:

![](https://i.imgur.com/KnA707a.png)

If we have not set an expiration time for the String object, the timestamp stores the default value 0; otherwise it stores the timestamp of the object's expiration time. Each time we retrieve a String object, we first parse the last four bytes of the Value part to get the timestamp and make a determination before returning the result.

## 2. Hash Structure Storage
A hash table in blackwidow consists of two parts: metadata (meta_key, meta_value) and actual data (data_key, data_value). The metadata mainly stores information about the hash table, such as the number of fields in the current hash table, as well as the current hash table's version and expiration time (for instant-delete functionality). The actual data refers to the one-to-one corresponding fields and values within the same hash table. The final on-disk RocksDB KV format is as follows:

1. The on-disk format for meta_key and meta_value of each hash table:
![](https://i.imgur.com/YLP48rg.png)

meta_key is essentially the key of the hash table, while meta_value consists of three parts: 4 Bytes of Hash size (stores the current hash table size) + 4 Bytes of Version (for instant-delete functionality) + 4 Bytes of Timestamp (records the expiration timestamp set for this hash table, defaulting to 0).

2. The on-disk format for data_key and data_value in a hash table:
![](https://i.imgur.com/phiBsqd.png)

data_key consists of four parts: 4 Bytes of Key size (records the length of the key appended after, for parsing purposes) + key content + 4 Bytes of Version + Field content. data_value is the value corresponding to a field in the hash table.

3. To look up the value corresponding to a field in a hash table, we first get the meta_value and parse the timestamp to check if the hash table has expired. If not, we retrieve the version, then construct the data_key using key, version, and field, and thereby find the corresponding data_value (if it exists).

## 3. List Structure Storage
A list in blackwidow consists of two parts: metadata (meta_key, meta_value) and actual data (data_key, data_value). The metadata stores information about the list, such as the number of nodes in the list, the version and expiration time (for instant-delete functionality), and the left and right boundaries of the list (since nemo's linked list implementation was criticized for low lrange efficiency, blackwidow uses an array to simulate the linked list at the bottom, making lrange much faster because nodes are stored in order). The actual data refers to the data in each node of the list. The final on-disk RocksDB KV format is as follows:

1. The on-disk format for meta_key and meta_value of each list:
![](https://i.imgur.com/083SjIc.png)

meta_key is essentially the key of the list, while meta_value consists of five parts: 8 Bytes of List size (stores the total number of nodes in the current list) + 4 Bytes of Version (for instant-delete functionality) + 4 Bytes of Timestamp (records the expiration timestamp set for this list, defaulting to 0) + 8 Bytes of Left Index (left boundary of the array) + 8 Bytes of Right Index (right boundary of the array).

2. The on-disk format for data_key and data_value in a list:
![](https://i.imgur.com/FBBn6kd.png)

data_key consists of four parts: 4 Bytes of Key size (records the length of the key appended after, for parsing purposes) + key content + 4 Bytes of Version + 8 Bytes of Index (records the index of the current node in the list). data_value is the value stored in that list node.

## 4. Set Structure Storage
A set in blackwidow consists of two parts: metadata (meta_key, meta_value) and actual data (data_key, data_value). The metadata stores information about the set, such as the number of members in the current set, the version and expiration time (for instant-delete functionality). The actual data refers to the members in the set. The final on-disk RocksDB KV format is as follows:

1. The on-disk format for meta_key and meta_value of each set:
![](https://i.imgur.com/bQeVvSj.png)

meta_key is essentially the key of the set, while meta_value consists of three parts: 4 Bytes of Set size (stores the current set size) + 4 Bytes of Version (for instant-delete functionality) + 4 Bytes of Timestamp (records the expiration timestamp set for this set, defaulting to 0).

2. The on-disk format for data_key and data_value in a set:
![](https://i.imgur.com/d2ctqPo.png)

data_key consists of four parts: 4 Bytes of Key size (records the length of the key appended after, for parsing purposes) + key content + 4 Bytes of Version + member content. Since sets only need to store members, data_value is actually an empty string.

## 5. ZSet Structure Storage
A zset in blackwidow consists of two parts: metadata (meta_key, meta_value) and actual data (data_key, data_value). The metadata stores information about the zset, such as the number of members in the current zset, the version and expiration time (for instant-delete functionality). The actual data refers to each member and its corresponding score in the zset. Since this data structure requires sorting by both member and score, for each zset we store two copies of the actual data in different formats — referred to as member-to-score and score-to-member. The final on-disk RocksDB KV format is as follows:

1. The on-disk format for meta_key and meta_value of each zset:
![](https://i.imgur.com/RhZ8KMw.png)

meta_key is essentially the key of the zset, while meta_value consists of three parts: 4 Bytes of ZSet size (stores the current ZSet size) + 4 Bytes of Version (for instant-delete functionality) + 4 Bytes of Timestamp (records the expiration timestamp set for this Zset, defaulting to 0).

2. The on-disk format for data_key and data_value for each zset (member to score):
![](https://i.imgur.com/C85Ba5Z.png)

The member-to-score data_key consists of four parts: 4 Bytes of Key size (records the length of the key appended after, for parsing purposes) + key content + 4 Bytes of Version + member content. data_value stores the score corresponding to the member, 8 bytes in size. Since RocksDB defaults to lexicographic ordering, different members within the same zset are ordered by member's dictionary order (the key size, key, and version, i.e., the prefix of the same zset, are the same; only the trailing member differs).

3. The on-disk format for data_key and data_value for each zset (score to member):
![](https://i.imgur.com/QV9XHEk.png)

The score-to-member data_key consists of five parts: 4 Bytes of Key size (records the length of the key appended after, for parsing purposes) + key content + 4 Bytes of Version + 8 Bytes of Score + member content. Since both score and member are already stored in data_key, data_value is an empty string. For the score-to-member data_key, we implement a custom RocksDB comparator: within the same zset, score-to-member data_keys are sorted first by score, and then by member when scores are equal.


## Advantages of Blackwidow over Nemo
1. Blackwidow uses RocksDB's column families feature to store metadata and actual data separately (corresponding to the meta data and data data above). This is more reasonable than Nemo, which mixes meta and data together, and can improve lookup efficiency (e.g., the efficiency of `info keyspace` is greatly improved).
2. Blackwidow passes parameters using Slice extensively, while Nemo uses `std::string`. Nemo incurs many unnecessary constructor and destructor calls for string objects, causing extra resource consumption. Blackwidow does not have this problem.
3. Blackwidow redesigned the storage format for KV-simulated multi-data structures (compared to the Nemo engine data storage format described in this article), resolving some performance problems that could not be fixed in Nemo. Therefore, in certain scenarios, Blackwidow's multi-data-structure performance far exceeds Nemo's.
4. The original Nemo only supported multi-data-structure Key lengths up to 256 Bytes. Blackwidow, after redesign, removes this limitation on multi-data-structure Key length.
5. Blackwidow is more space-efficient than Nemo. Nemo requires nemo-rocksdb support, so it appends version and timestamp information in both meta and data parts. It also adds 's' and 'S' prefixes to distinguish meta_key and data_key (taking Set data structure as an example). Blackwidow optimizes this — for the same data volume, Blackwidow uses less space than Nemo (for example, one List structure node in Blackwidow uses 16 Bytes less space than one node in Nemo).
6. Blackwidow's lock implementation references RocksDB transaction lock implementation, abandoning Nemo's previous row lock approach. This improves performance in multi-thread lock-contention scenarios.
