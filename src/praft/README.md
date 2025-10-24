# Pika Praft 模块

该模块使用 Braft (百度 Raft) 库为 Pika 提供分布式共识支持。

## 概述

Praft 模块是一个独立组件，提供以下功能：
- 使用 Raft 协议的分布式共识
- 通过自动 leader 选举实现高可用
- 跨集群节点的强一致性保证
- 与现有主从复制机制共存

## 结构

```
praft/
├── CMakeLists.txt          # 构建配置
├── README.md               # 本文件
├── include/
│   └── praft/
│       └── praft.h         # 核心 Praft 类（RaftManager、PikaRaftNode、PikaStateMachine）
├── src/
│   ├── praft.cc            # 核心实现
│   └── pika_raft.proto     # Raft 日志条目的 Protobuf 定义
└── tests/
    └── raft_cluster_test.sh # Raft 命令测试脚本

根目录的 Raft 命令文件：
├── include/pika_raft.h     # Raft 命令类（RAFT.CLUSTER、RAFT.NODE、RAFT.CONFIG）
└── src/pika_raft.cc        # Raft 命令实现
```

## 核心组件

### RaftManager
系统中所有 Raft 节点的全局管理器。负责：
- 集群初始化和配置
- 节点管理（添加/删除）
- 生命周期管理（init/start/shutdown）

### PikaRaftNode
每个数据库的 `braft::Node` 封装器。管理：
- Raft 组成员关系
- Leader 选举和日志复制
- 节点间通信

### PikaStateMachine
实现 `braft::StateMachine` 接口。处理：
- 将已提交的日志条目应用到数据库
- 快照创建和加载
- Leader/Follower 状态转换

### Raft 命令
- **RAFT.CLUSTER [INIT|JOIN|INFO]** - 集群管理
- **RAFT.NODE [ADD|REMOVE]** - 节点管理
- **RAFT.CONFIG GET** - 配置查询

## 配置

在 `pika.conf` 中启用 Raft：

```ini
raft-enabled : yes
raft-group-id : pika_raft_group
raft-peers : 127.0.0.1:12221,127.0.0.1:12222,127.0.0.1:12223
raft-election-timeout-ms : 1000
raft-snapshot-interval-s : 3600
```

## 使用示例

```bash
# 初始化集群
redis-cli RAFT.CLUSTER INIT 127.0.0.1:12221,127.0.0.1:12222 db0

# 获取集群信息
redis-cli RAFT.CLUSTER INFO db0

# 添加节点
redis-cli RAFT.NODE ADD 127.0.0.1:12223 db0
```

## 依赖

- braft (百度 Raft 库)
- brpc (百度 RPC 库)
- protobuf (Protocol Buffers)
- glog (Google 日志库)
- leveldb (用于 Raft 元数据存储)

## 文档

详细文档请参考：
- [Raft 集成指南](../../docs/design/raft_integration.md)
- [实现总结](../../BRAFT_INTEGRATION_SUMMARY.md)

## 测试

运行测试脚本：
```bash
./tests/raft_cluster_test.sh
```

## 许可证

Copyright (c) 2015-present, Qihoo, Inc.
采用 BSD 风格许可证授权。
