Nemo is essentially a modification and encapsulation of RocksDB, enabling it to support multi-data-structure storage (RocksDB only supports KV storage). In total, nemo supports storage of five data structure types: KV key-value pairs (to distinguish them, nemo's key-value structure is written as uppercase "KV"), Hash structures, List structures, Set structures, and ZSet structures. Because RocksDB's storage format is only KV, all five data structures described above ultimately need to be stored as RocksDB KV pairs on disk.

## 1. KV Key-Value Storage
KV storage does not add extra metadata; it simply appends 8 bytes of additional information at the end of the value (the first 4 bytes represent the version, and the last 4 bytes represent the TTL) as the final on-disk value. Specifically:

![](http://ww2.sinaimg.cn/large/c2cd4307jw1f6nbojnvjjj20k204tq30.jpg)
The version field is used to mark the key-value pair for subsequent processing. For example, when deleting a key-value pair, the version can be marked so that the actual deletion can happen later. This reduces service blocking caused by deletion operations.


## 2. Hash Structure Storage
For each Hash storage, it includes a hash key, a field name (field) under the hash key, and the stored value. Nemo's storage approach combines the key and field into a new key, then forms the final on-disk KV pair with this new key and the value to be stored. Additionally, for each hash key, nemo also adds an on-disk KV for storing metadata, which saves the count of all field-value pairs under the corresponding hash key. Below is the specific implementation:
1. Mapping from each hash key, field, value to on-disk KV
![image](http://ww3.sinaimg.cn/large/c2cd4307gw1f6m742p5h5j20rg06iaad.jpg)

2. On-disk KV storage format for each hash key's metadata
![](http://ww2.sinaimg.cn/large/c2cd4307jw1f6nbsn0up3j20jx04yweq.jpg)

In diagram 1: the preceding bar represents the key part of the on-disk KV. From front to back: the first field is the character 'h', indicating a hash structure key; the second field is the string length of the hash key (1 byte, uint8_t type); the third field is the content of the hash key (since the second field is one byte, the maximum hash key string length is 254 bytes); the fourth field is the field content. The trailing bar represents the value part of the on-disk KV, which is the stored value plus an 8-byte version field and an 8-byte TTL field, similar to KV structure storage.
In diagram b: the preceding bar represents the key part of the on-disk KV storing metadata for each hash key. It consists of two fields: the first is the character 'H', indicating stored hash key metadata; the second is the hash key string content. The trailing bar represents the value of the metadata, indicating the count of field-value pairs in the corresponding hash key, 8 bytes in size (int64_t type).

## 3. List Structure Storage
As the name suggests, the underlying storage for each List structure also uses a linked list. For each List key, each element is stored as a KV pair on disk, as a node in the linked list, called an element node. Like Hash, each List key also has its own metadata.

a. On-disk KV storage format for each element node
![image](http://ww4.sinaimg.cn/large/c2cd4307gw1f6m74322gaj20qx06mweu.jpg)

b. On-disk KV storage format for each metadata entry
![image](http://ww2.sinaimg.cn/large/c2cd4307gw1f6m7422myxj20m806mdg6.jpg)

In diagram a: the preceding bar represents the key part of the final on-disk KV structure, consisting of 4 fields: the first three character fields are the character 'l' (indicating a List structure), the string length of the List key (1 byte), and the string content of the List key (up to 254 bytes). The fourth field is the index value of the element node (8 bytes, int64_t type). For each element node, this index (sequence) is unique and is the only means for other element nodes to access this one. When adding the first element node to an empty List key, the sequence is 1; the next is 2, incrementing sequentially. Even if an element in between is deleted, its sequence is not reused by later-inserted element nodes, ensuring uniqueness. The trailing bar in diagram a represents the value of the on-disk KV structure, with 5 fields: the last three are the stored value, version, and TTL, similar to hash structure storage; the first two are the sequence of the previous element node and the sequence of the next element node. Through these two sequences, you can determine the on-disk KV key content of the previous and next element nodes, thus implementing a doubly-linked list structure.

In diagram b: the preceding bar represents the key part of the on-disk KV storing metadata, similar to the hash structure; the trailing bar represents the metadata for the List key, consisting of four fields from front to back: the count of elements in the List key, the sequence of the leftmost element node (equivalent to the list head), the sequence of the rightmost element node (equivalent to the list tail), and the sequence to be used for the next element node to be inserted.

## 4. Set Structure Storage
a. On-disk KV storage format for each element node
![image](http://ww4.sinaimg.cn/large/c2cd4307gw1f6m741krdxj20kw06mjrk.jpg)

b. On-disk KV storage format for each Set key's metadata
![image](http://ww3.sinaimg.cn/large/c2cd4307gw1f6m742hinvj20lw06o3yo.jpg)

Set structure storage is basically the same as Hash structure storage, except that for each element's on-disk KV in a Set, the value part only contains version and TTL — there is no value field.

## 5. ZSet Structure Storage
ZSet is an ordered Set, so for each element, an additional on-disk KV is stored. In the key part of this additional KV, the element's score value is embedded, making it easy to sort by score (since data retrieved from RocksDB is sorted by key). Below are the on-disk KV storage formats.

a. On-disk KV format with score in the value part
![](http://ww1.sinaimg.cn/large/c2cd4307jw1f6nbzzqa64j20oa068jrk.jpg)

b. On-disk KV format with score in the key part
![](http://ww4.sinaimg.cn/large/c2cd4307jw1f6nc0td9l7j20oa068jrk.jpg)

c. On-disk KV format for metadata storage
![](http://ww2.sinaimg.cn/large/c2cd4307jw1f6nc19p731j20ii068t8u.jpg)

Diagrams a and c are similar to the other data structures and will not be elaborated further. In diagram b, the score is converted from `double` type to `int64_t` type, enabling the original floating-point score to participate directly in string sorting (the storage format of floating-point numbers is incompatible with string comparison).
