Pika uses a multi-threaded model, employing multiple worker threads for read and write operations. Thread safety is guaranteed by the underlying blackwidow engine. There are 12 types of threads:

- PikaServer: the main thread
- DispatchThread: listens on 1 port, accepts user connection requests
- WorkerThread: multiple exist (user-configured); each thread handles several client connections, receives user commands, packages commands into Tasks and throws them into the ThreadPool for execution; after task execution, this thread returns the reply to the user
- ThreadPool: the number of threads in the thread pool is user-configured; executes Tasks dispatched by WorkerThread; Tasks mainly involve writing to DB and writing Binlog
- PikaAuxiliaryThread: auxiliary thread; handles state machine transitions during the sync process, heartbeat sending between master and slave, and timeout checking
- PikaReplClient: essentially an Epoll thread (communicating with the PikaReplServer of other Pika instances) plus a thread array (asynchronously handles tasks for writing Binlog and writing DB)
- PikaReplServer: essentially an Epoll thread (communicating with the PikaReplClient of other Pika instances) plus a thread pool (handles sync requests and updates the Binlog sliding window based on Ack responses from slaves)
- MonitorThread: clients that have executed the Monitor command are assigned to this thread; this thread returns the commands currently being processed by Pika to clients attached to it
- KeyScanThread: executes the key count task triggered by `info keyspace 1`
- BgSaveThread: performs the Dump operation on a specified DB, and during full sync, sends Dump data to slaves (a full sync for one DB involves sequentially pushing BgSave and DBSync tasks into the thread to guarantee ordering)
- PurgeThread: used to clean up expired Binlog files
- PubSubThread: used to support PubSub-related features
