     
As a Redis-like storage system, Pika makes extensive use of [multi-threaded structures](https://github.com/Qihoo360/pika/wiki/pika-%E7%BA%BF%E7%A8%8B%E6%A8%A1%E5%9E%8B) throughout the system to compensate for performance limitations. With multi-threaded programming, locking is essential to ensure data access consistency and validity. Three main types of locks are used:

1. Mutex lock
1. Read-write lock
2. Row lock (record lock)

## Read-Write Lock
### Use Case
The suspended-command instruction requires adding a write lock during execution to ensure no other commands can execute simultaneously. Other regular commands add a read lock, allowing parallel access.
The suspended commands include:
1. trysync
2. bgsave
3. flushall
4. readonly

#### Purpose and Significance
Ensures that when the server is executing a suspended command, it blocks all writes.

## Row Lock (Record Lock)
A row lock is used to lock a single key, ensuring only one thread operates on a given key at a time.

### Purpose and Significance:
All data stored in Pika is key-value type data. Data corresponding to different keys is completely independent, so locking only by key can ensure data consistency under concurrent access. Row locks have a smaller lock granularity, which also ensures high efficiency in data access.

### Use Case
In the Pika system, all database operations require a row lock. Row locks are mainly applied in two places: during the upper-level command processing stage and at the data engine layer. For write commands (which change data state, such as SET, HSET), in addition to updating the database state, there is also [incremental sync](https://github.com/Qihoo360/pika/wiki/pika-%E4%B8%BB%E4%BB%8E%E5%90%8C%E6%AD%A5%E5%8A%9F%E8%83%BD) — the executed write command must be added to the binlog to ensure consistent database state between master and slave. Therefore, executing a write command involves two main parts:

1. Updating the database state
2. Adding the command to the binlog

The locking scenario is as shown below:
![](http://ww4.sinaimg.cn/large/c2cd4307jw1f6no7d5557j20fa0ma74x.jpg)

#### Design Balance
As can be seen in the diagram, the same key is locked twice with a row lock. In practice, the lock at the Pika layer is sufficient to ensure correctness of data access. If only serving Pika's needs, the row lock at the blackwidow layer would be redundant. However, the [design of blackwidow](https://github.com/Qihoo360/pika/wiki/pika-blackwidow引擎数据存储格式) was intended to provide a complete Redis-like data access solution through modification and encapsulation of RocksDB — not just to serve as a database engine for Pika. This design philosophy follows the Unix principle: Write programs that do one thing and do it well.

This design greatly reduces the coupling between Pika and blackwidow, and allows blackwidow to be independently tested and used. The [data migration tool](https://github.com/Qihoo360/pika/wiki/pika%E5%88%B0redis%E8%BF%81%E7%A7%BB%E5%B7%A5%E5%85%B7) in Pika uses blackwidow entirely without depending on any Pika-related code. Additionally, teams interested in or needing blackwidow can use it directly as a database engine without modifying any code to get full data access functionality.


### Concrete Implementation
In the Pika system, a single row lock can maintain all keys. The implementation binds a key to a mutex lock and maintains it in a hash table, ensuring only one thread accesses a key at a time. It is neither necessary nor possible to keep one mutex lock for every key; a lock is only needed when multiple threads access the same key. After all threads finish accessing, the mutex lock bound to that key can be destroyed and resources released. The specific implementation is as follows:
``` C++
class RecordLock {
 public:
  RecordLock(port::RecordMutex *mu, const std::string &key)
      : mu_(mu), key_(key) {
        mu_->Lock(key_);
      }
  ~RecordLock() { mu_->Unlock(key_); }

 private:
  port::RecordMutex *const mu_;
  std::string key_;

  // No copying allowed
  RecordLock(const RecordLock&);
  void operator=(const RecordLock&);
};

void RecordMutex::Lock(const std::string &key) {
  mutex_.Lock();
  std::unordered_map<std::string, RefMutex *>::const_iterator it = records_.find(key);

  if (it != records_.end()) {
    //log_info ("tid=(%u) >Lock key=(%s) exist, map_size=%u", pthread_self(), key.c_str(), records_.size());
    RefMutex *ref_mutex = it->second;
    ref_mutex->Ref();
    mutex_.Unlock();

    ref_mutex->Lock();
    //log_info ("tid=(%u) <Lock key=(%s) exist", pthread_self(), key.c_str());
  } else {
    //log_info ("tid=(%u) >Lock key=(%s) new, map_size=%u ++", pthread_self(), key.c_str(), records_.size());
    RefMutex *ref_mutex = new RefMutex();

    records_.insert(std::make_pair(key, ref_mutex));
    ref_mutex->Ref();
    mutex_.Unlock();

    ref_mutex->Lock();
    //log_info ("tid=(%u) <Lock key=(%s) new", pthread_self(), key.c_str());
  }
}

```
Full code reference: [pstd_mutex.cc](https://github.com/OpenAtomFoundation/pika/blob/unstable/src/pstd/src/pstd_mutex.cc) [pstd_mutex.h](https://github.com/OpenAtomFoundation/pika/blob/unstable/src/pstd/include/pstd_mutex.h)
