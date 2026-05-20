# Quick Start

## Instance Information

### Instance A

- Address: codis-server-host:6379
- Port: 6379
- Password: xxxxx

### Pika Instance B

- Address: 192.168.0.1:6379
- Port: 6379
- Password: xxxxx

### Instance C — Codis Cluster Instance

- Addresses:
    - codis-server-host-1:6379
    - codis-server-host-2:6379
- Password: xxxxx

## Working Directory

```
.
├── codis2pika # binary executable
└── codis2pika.toml # configuration file
```

## Getting Started

## A -> B Sync

Edit `codis2pika.toml` with the following configuration:

```toml
[source]
type = "sync"
address = "codis-server-host:6379"
password = "xxxxx"

[target]
type = "standalone"
address = "192.168.0.1:6379"
password = "xxxxx"
```

Start codis2pika:

```bash
./codis2pika codis2pika.toml
```

## C -> B Sync

Edit `codis2pika.toml` with the following configuration:

```toml
[source]
type = "sync"
address = "codis-server-host-1:6379"
password = "xxxxx"

[target]
type = "standalone"
address = "192.168.0.1:6379" # any node address in the cluster
password = "xxxxx"

[advanced]
dir = "data" # must be unique
```
Copy for the second instance: `cp codis2pika.toml codis2pika2.toml`
```toml
[source]
type = "sync"
address = "codis-server-host-2:6379"
password = "xxxxx"

[target]
type = "standalone"
address = "192.168.0.1:6379" # any node address in the cluster
password = "xxxxx"

[advanced]
dir = "data2" # must be unique
```

Start codis2pika:

```bash
nohup ./codis2pika codis2pika.toml &
nohup ./codis2pika codis2pika2.toml &
```

Similarly, multiple instances of cluster C can be started to achieve migration from a Codis cluster to a Pika cluster.
