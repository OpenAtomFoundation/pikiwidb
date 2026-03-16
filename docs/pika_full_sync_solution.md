# Pika 全量同步方案详解（Scheme A）

## 文档说明

本文档详细描述 Pika 3.5.0+ 版本的全量同步机制（Scheme A），包括各场景下的完整流程、状态变化、数据流转以及已知问题。

**版本信息**
- 适用版本：Pika 3.5.0+
- 方案名称：Scheme A（独立 Dump + 延迟清理）
- 最后更新：2026-03-06

---

## 1. 架构概述

### 1.1 核心设计

**Scheme A** 采用以下设计原则：

1. **每个 Slave 独占一个 Dump 目录**：`dump-YYYYMMDD-NN/db_name` 格式
2. **传输完成后延迟清理**：孤儿文件（nlink=1）传输完成后加入延迟清理队列（10分钟后删除）
3. **最大并发限制**：默认最多 3 个并发 dump
4. **细粒度文件保护**：传输中的文件受保护，防止被误删
5. **统一清理入口**：所有孤儿文件清理通过 `RemoveTransferringFile` 统一处理

### 1.2 关键组件

| 组件 | 文件 | 职责 |
|------|------|------|
| RsyncServer | `rsync_server.cc` | 处理 Slave 文件同步请求 |
| RsyncServerConn | `rsync_server.cc` | 维护单个连接的状态 |
| PikaServer | `pika_server.cc` | 管理 Dump 占用、snapshot 注册 |
| DB | `pika_db.cc` | 管理 bgsave 和 dump 元数据 |

### 1.3 关键数据结构

```cpp
// Dump 占用信息
struct DumpOwnerInfo {
    std::string conn_id;      // 占用连接的 ID
    std::string dump_path;    // dump 目录路径
};
std::map<std::string, DumpOwnerInfo> dump_owners_;  // snapshot_uuid -> 占用信息

// 传输中文件保护
std::map<std::string, std::set<std::string>> rsync_transferring_files_;  // snapshot_uuid -> 文件集合

// 活跃 snapshot
std::set<std::string> active_rsync_snapshots_;  // 用于孤儿文件清理保护
```

---

## 2. 单个 Slave 单 DB 全量同步流程

以 `db0` 为例，详细描述 Master 和 Slave 的状态变化。

### 2.1 流程时序图

```
阶段1: 触发全量同步
┌─────────────┐                    ┌─────────────┐
│    Slave    │                    │    Master   │
└──────┬──────┘                    └──────┬──────┘
       │                                   │
       │  1. 判断需要全量同步               │
       │  (repl_state: kTryConnect)       │
       │                                   │
       │  2. 发送 DBSync 请求              │
       │ ───────────────────────────────>│
       │                                   │
       │                              3. 检查是否正在 bgsave
       │                              (IsBgSaving())
       │                                   │
       │                              4. 如果不在 bgsave，触发 bgsave
       │                              (BgSaveDB())
       │                                   │
       │  5. 返回 kErr（等待 bgsave）      │
       │ <───────────────────────────────│
       │                                   │
       │  6. 重试（循环）                   │
       │ ───────────────────────────────>│
       │                              7. 如果仍在 bgsave，返回 kErr
       │ <───────────────────────────────│
       │                                   │

阶段2: bgsave 执行
┌─────────────┐                    ┌─────────────┐
│  Background │                    │    Master   │
│    Thread   │                    │             │
└──────┬──────┘                    └──────┬──────┘
       │                                   │
       │  1. 创建 dump 目录                 │
       │  (InitBgsaveEnv)                  │
       │  dump-20260305-0/db0              │
       │                                   │
       │  2. 创建 RocksDB Checkpoint       │
       │  (创建硬链接)                      │
       │                                   │
       │  3. 生成 info 文件                 │
       │                                   │
       │  4. bgsave 完成                   │
       │  (IsBgSaving() -> false)          │
       │                                   │

阶段3: Meta 请求处理
┌─────────────┐                    ┌─────────────┐
│    Slave    │                    │    Master   │
└──────┬──────┘                    └──────┬──────┘
       │                                   │
       │  1. 再次发送 DBSync 请求          │
       │  (循环重试后 bgsave 已完成)        │
       │ ───────────────────────────────>│
       │                                   │
       │                              2. 获取文件列表
       │                              (GetDumpMeta)
       │                              扫描 dump-20260305-0/db0
       │                              生成 snapshot_uuid
       │                                   │
       │                              3. 检查 dump 完整性
       │                              4. 检查是否已被占用
       │                              5. 检查并发限制
       │                              6. 标记 dump 为占用
       │                              (MarkDumpInUse)
       │                              7. 注册 snapshot
       │                              (RegisterSnapshot)
       │                              8. 预注册所有文件
       │                              (AddTransferringFile)
       │                                   │
       │  9. 返回 Meta 响应                 │
       │  (snapshot_uuid + 文件列表)       │
       │ <───────────────────────────────│
       │                                   │

阶段4: 文件传输
┌─────────────┐                    ┌─────────────┐
│    Slave    │                    │    Master   │
└──────┬──────┘                    └──────┬──────┘
       │                                   │
       │  1. 多线程下载文件                 │
       │  ─────────────────────────────> │
       │                                   │
       │                              2. 检查文件是否存在
       │                              3. 注册文件为传输中
       │                              4. 读取文件内容
       │                              5. 注销文件
       │                              6. 如果是最后一块(is_eof)
       │                                 检查是否为孤儿文件(nlink=1)
       │                                 如果是孤儿，加入延迟清理队列(10分钟)
       │                                   │
       │  7. 返回文件数据                   │
       │ <───────────────────────────────│
       │                                   │
       │  (重复直到所有文件下载完成)         │

阶段5: 清理
┌─────────────┐                    ┌─────────────┐
│    Slave    │                    │    Master   │
└──────┬──────┘                    └──────┬──────┘
       │                                   │
       │  1. 下载完成，关闭连接             │
       │ ───────X────────────────────────>│
       │                                   │
       │                              2. 连接断开，析构 RsyncServerConn
       │                              3. 释放 dump 占用
       │                              (ReleaseDump)
       │                              4. 注销 snapshot
       │                              (UnregisterSnapshot)
       │                                   │
       │                              5. AutoDeleteExpiredDump 定时执行
       │                              处理延迟清理队列(ProcessPendingCleanupFiles)
       │                              删除过期 dump 目录
       │                              (注：CleanupOrphanSstFiles 已移除，延迟清理统一处理)
       │                                   │
```

### 2.2 Master 状态变化

| 阶段 | 状态 | 说明 |
|------|------|------|
| T0 | 无 dump | 初始状态 |
| T1 | bgsaving | 创建 dump-20260305-0/db0 |
| T2 | dump 可用 | bgsave 完成，等待 Meta 请求 |
| T3 | dump 占用 | 收到 Meta 请求，标记为占用 |
| T4 | 传输中 | 文件传输中，即时清理进行中 |
| T5 | dump 释放 | Slave 断开，释放占用 |
| T6 | dump 过期 | AutoDeleteExpiredDump 删除过期 dump |

### 2.3 Slave 状态变化

| 阶段 | 状态 | 说明 |
|------|------|------|
| T0 | kTryConnect | 尝试连接 Master |
| T1 | kWaitDBSync | 等待 Master bgsave 完成 |
| T2 | kWaitDBSync | 获取文件列表，开始下载 |
| T3 | kWaitDBSync | 文件下载中 |
| T4 | kConnected | 全量同步完成，开始增量同步 |

### 2.4 数据变化

**Master 磁盘占用变化**：

| 时间点 | 数据目录 | Dump 目录 | 总计 |
|--------|----------|-----------|------|
| 初始 | 100GB | 0 | 100GB |
| bgsave 中 | 100GB | 0 (硬链接不占用) | 100GB |
| compaction 后 | 100GB | 部分孤儿文件 | 100GB + 孤儿文件 |
| 传输中 | 100GB | 100GB (dump) | 200GB |
| 传输完成 | 100GB | 孤儿文件延迟10分钟清理 | 100GB ~ 200GB |

---

## 3. 多 Slave 同步流程

### 3.1 场景描述
- Master 有 100GB 数据
- Slave-1 先发起同步
- Slave-2 在 Slave-1 同步过程中发起同步

### 3.2 流程时序

```
时间线:
T0:
  Slave-1 ──DBSync──> Master
  Master: IsBgSaving? No
  Master: 触发 BgSaveDB()
  Master: 创建 dump-20260305-0/db0
  Slave-1 <──kErr─── Master (等待 bgsave)

T30s:
  Master: bgsave 完成
  Slave-1 ──DBSync──> Master
  Master: 获取文件列表 (dump-0)
  Master: MarkDumpInUse(dump-0, Slave-1)
  Slave-1 <──文件列表── Master
  Slave-1 开始下载...

T31s:
  Slave-2 ──DBSync──> Master
  Master: IsDumpInUse(dump-0)? Yes (被 Slave-1 占用)
  Master: 触发新的 BgSaveDB()
  Master: 创建 dump-20260305-1/db0
  Slave-2 <──kErr─── Master (等待新 bgsave)

T61s:
  Master: 新 bgsave 完成
  Slave-2 ──DBSync──> Master
  Master: MarkDumpInUse(dump-1, Slave-2)
  Slave-2 <──文件列表── Master
  Slave-2 开始下载...

T120s:
  Slave-1: 下载完成，断开连接
  Master: ReleaseDump(dump-0)
  Master: 删除 dump-0 (AutoDeleteExpiredDump)

T180s:
  Slave-2: 下载完成，断开连接
  Master: ReleaseDump(dump-1)
  Master: 删除 dump-1
```

### 3.3 关键限制

- **最大并发 dump 数**：3 个（`kMaxConcurrentDumps = 3`）
- **超过限制**：返回 kErr，Slave 重试

---

## 4. 单 Slave 多 DB 同步流程

### 4.1 场景描述
- Master 配置 3 个 DB：db0, db1, db2
- 每个 DB 有独立的 RocksDB 实例（db-instance-num=3）
- Slave 同时同步所有 DB

### 4.2 目录结构

```
dump/dump-20260305-0/
├── db0/
│   ├── 0/          # RocksDB 实例 0
│   │   ├── 000001.sst
│   │   └── 000002.sst
│   ├── 1/          # RocksDB 实例 1
│   │   └── 000003.sst
│   ├── 2/          # RocksDB 实例 2
│   │   └── 000004.sst
│   └── info        # dump 元信息
├── db1/
│   ├── 0/
│   ├── 1/
│   ├── 2/
│   └── info
└── db2/
    ├── 0/
    ├── 1/
    ├── 2/
    └── info
```

### 4.3 文件命名规则

- Slave 请求格式：`{rocksdb_instance}/{filename}`
- 示例：`0/000001.sst`, `1/000003.sst`
- **注意**：不包含 db0/db1/db2 前缀

### 4.4 同步流程

每个 DB 独立同步：

1. Slave 发送 db0 的 DBSync 请求
2. Master 返回 db0 的文件列表
3. Slave 下载 db0 的所有文件
4. 重复步骤 1-3 对 db1 和 db2

### 4.5 潜在问题

**问题：info 文件位置不一致**
- `AutoDeleteExpiredDump` 查找：`dump/dump-xxx/info`
- 实际位置：`dump/dump-xxx/db0/info`

**已修复**：先尝试 `db0/info`，再回退到 `info`

---

## 5. 多 Slave 多 DB 同步流程

这是 Scheme A 最复杂的场景，结合了多 Slave 和多 DB 的特点。

### 5.1 场景描述
- Master：3 个 DB (db0, db1, db2)
- Slave-1：同步 db0, db1, db2
- Slave-2：同步 db0, db1, db2

### 5.2 Dump 占用机制

**方案 A 设计**：每个 Slave 独占整个 dump 目录（包含所有 DB）

```
Slave-1 占用 dump-20260305-0:
├── db0 (传输中)
├── db1 (传输中)
└── db2 (传输中)

Slave-2 占用 dump-20260305-1:
├── db0 (传输中)
├── db1 (传输中)
└── db2 (传输中)
```

### 5.3 占用检查

- 检查粒度：**整个 dump 目录**
- 如果一个 Slave 正在使用 dump-0，其他 Slave 不能使用
- 触发新的 bgsave 创建 dump-1

### 5.4 潜在问题

**问题 1：DB 级别粒度 vs Dump 级别粒度**
- 当前设计：dump 级别占用
- 如果 Slave-1 只同步 db0，dump-0 仍不能被 Slave-2 使用
- 浪费磁盘空间

**问题 2：多 DB 的孤儿文件清理**
- `AutoDeleteExpiredDump` 只检查 `db0/info`
- 如果 db1 或 db2 还在传输，可能被误判为可清理

---

## 6. 孤儿文件清理机制（统一延迟清理）

### 6.1 触发条件

孤儿文件：nlink=1 的 SST 文件（只被 dump 引用，不被 RocksDB 引用）

**产生原因**：
- RocksDB compaction 删除旧 SST
- dump 中的硬链接变成孤儿

### 6.2 统一清理策略

**设计变更**：移除 `CleanupOrphanSstFiles` 函数，统一使用延迟清理队列

**新清理流程**：

```
1. 文件传输完成时（RemoveTransferringFile）
   - 检查 is_eof=true（最后一块传输完成）
   - stat 检查文件 nlink
   - 如果 nlink=1（孤儿文件）：
     * 加入延迟清理队列（ScheduleFileForCleanup，延迟600秒）
     * 记录日志 "Scheduled orphan file for cleanup"
   - 如果 nlink=2（非孤儿）：
     * 不做处理，RocksDB 会管理生命周期

2. AutoDeleteExpiredDump 定时执行（每60秒）
   - 调用 ProcessPendingCleanupFiles()
   - 检查队列中到期的文件
   - 删除到期文件，记录日志 "Deleted delayed cleanup file"
   - 同时检查并删除过期的 dump 目录
```

### 6.3 保护机制

| 保护级别 | 说明 | 实现位置 |
|----------|------|----------|
| 传输中保护 | 传输中的文件不会被清理 | `rsync_transferring_files_` |
| 延迟保护 | 孤儿文件延迟10分钟删除，给 Slave 重试时间 | `ScheduleFileForCleanup(filepath, 600)` |
| nlink 检查 | 只清理孤儿文件（nlink=1），避免误删 | `stat` 检查 |

### 6.4 时序说明

```
T0: 文件传输完成（is_eof=true）
  └─> RemoveTransferringFile 检查 nlink==1
      └─> ScheduleFileForCleanup(filepath, 600) 加入队列
          └─> 日志: "Scheduled orphan file for cleanup in 10min"

T0+10min: AutoDeleteExpiredDump 定时执行
  └─> ProcessPendingCleanupFiles()
      └─> 检查到期文件
          └─> 删除文件
              └─> 日志: "Deleted delayed cleanup file"
```

### 6.5 对比：旧方案 vs 新方案

| 方面 | 旧方案（CleanupOrphanSstFiles） | 新方案（统一延迟清理） |
|------|--------------------------------|----------------------|
| 触发时机 | 定时扫描所有 dump 目录 | 传输完成时即时检查 |
| 清理延迟 | 扫描周期不确定 | 固定延迟10分钟 |
| 竞争条件 | 与传输过程可能竞争 | 无竞争，统一入口 |
| 代码复杂度 | ~170行独立函数 | ~15行集成逻辑 |
| Slave 重试 | 可能失败（文件已被删） | 10分钟内可重试 |

---

## 7. Bug 列表

### 7.1 已修复的 Bug

| Bug | 影响 | 修复 | Commit |
|-----|------|------|--------|
| CreatePath 逻辑错误 | 无法创建 dump 目录 | 添加 EnsureDirExists 包装函数 | ee37a586 |
| 文件分片传输被提前删除 | Slave 重试失败 | 添加 is_eof 参数，只在传输完成时删除 | eb778848 |
| Snapshot 注册失败 | 文件被误删 | 修复 RegisterSnapshot 调用顺序 | 08ee1c36 |
| Info 文件路径错误 | 无法读取 snapshot_uuid | 先尝试 db0/info，再回退到 info | ad54f43a |
| 空 dump 目录返回空列表 | Slave 尝试下载不存在的文件 | 添加 filenames.empty() 检查 | 27c3a838 |
| 即时清理与延迟清理竞争 | Slave 重试时文件已被删 | 移除 CleanupOrphanSstFiles，统一使用延迟清理队列 | 当前版本 |
| CleanupOrphanSstFiles 误删传输中文件 | 同步失败 | 删除 CleanupOrphanSstFiles 函数 | 当前版本 |

### 7.2 已移除的 Bug（通过架构调整修复）

| Bug | 原影响 | 修复方式 |
|-----|--------|----------|
| CleanupOrphanSstFiles 竞争问题 | 与延迟清理队列竞争同一文件 | 移除 CleanupOrphanSstFiles 函数 |
| 即时清理导致重试失败 | Slave 30分钟后重试，文件已删 | 统一使用延迟10分钟清理 |

### 7.3 待修复的 Bug

| Bug | 影响 | 严重程度 | 修复方案 |
|-----|------|----------|----------|
| 多 DB 场景下孤儿文件清理粒度问题 | db1/db2 传输中可能被误判 | 中 | 检查所有 DB 的 info 文件 |
| 多 Slave 多 DB 的磁盘浪费 | 每个 Slave 独占整个 dump | 低 | 支持 DB 级别占用 |

---

## 8. 待办事项

### 8.1 高优先级

- [x] **统一孤儿文件清理机制（已完成）**
  - 移除 CleanupOrphanSstFiles 函数
  - 统一使用 RemoveTransferringFile + 延迟清理队列
  - 延迟10分钟，给 Slave 重试时间

- [ ] **修复多 DB 孤儿文件清理粒度问题**
  - 当前只检查 db0/info
  - 需要检查所有 DB 子目录
  - 如果任何 DB 在使用中，整个 dump 应被保护

### 8.2 中优先级

- [ ] **优化多 Slave 多 DB 的磁盘占用**
  - 当前：每个 Slave 独占整个 dump
  - 优化：支持 DB 级别占用
  - 影响：需要修改占用管理逻辑

- [ ] **完善监控指标**
  - Dump 占用数量
  - 孤儿文件清理统计
  - 传输失败率

### 8.3 低优先级

- [ ] **支持动态调整并发限制**
  - 当前：编译期常量 kMaxConcurrentDumps=3
  - 优化：支持配置热更新

- [ ] **Dump 压缩传输**
  - 减少网络带宽
  - 权衡 CPU 和网络

---

## 9. 配置建议

### 9.1 关键配置项

```ini
# pika.conf

# dump 目录前缀
dump-prefix : dump-

# dump 目录路径
dump-path : ./dump/

# dump 过期时间（天）
# 0 表示永不过期
dump-expire : 1

# RocksDB 实例数
db-instance-num : 3

# 最大并发 dump 数（编译期配置）
# kMaxConcurrentDumps = 3
```

### 9.2 部署建议

1. **磁盘空间**：预留 3 × 数据量 的空间
2. **监控**：监控 dump 目录数量和磁盘使用率
3. **日志**：关注 `[Rsync Meta]`、`[RsyncTransfer]`、`[Scheduled orphan file`、`[Deleted delayed cleanup` 日志

---

## 10. 附录

### 10.1 关键日志

```bash
# 查看 Meta 请求处理
grep "Rsync Meta" log/pika.INFO

# 查看文件传输
grep "RsyncTransfer" log/pika.INFO

# 查看孤儿文件延迟清理调度
grep "Scheduled orphan file" log/pika.INFO

# 查看延迟清理执行
grep "Deleted delayed cleanup file" log/pika.INFO

# 查看 dump 占用
grep "DumpOwnership" log/pika.INFO

# 查看错误
grep "File no longer exists" log/pika.WARNING
```

### 10.2 状态码说明

| 状态码 | 含义 | 处理 |
|--------|------|------|
| kOk | 成功 | 继续处理 |
| kErr | 错误 | Slave 重试 |

### 10.3 文件路径规范

| 类型 | 格式 | 示例 |
|------|------|------|
| Dump 目录 | dump-YYYYMMDD-NN/db_name | dump-20260305-0/db0 |
| RocksDB 实例 | {rocksdb_instance}/ | 0/, 1/, 2/ |
| SST 文件 | {instance}/{filename}.sst | 0/000001.sst |
| Info 文件 | db_name/info | db0/info |

---

## 文档历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| 1.0 | 2026-03-05 | 初始版本，整理 Scheme A 方案 |
| 1.1 | 2026-03-06 | 更新为统一延迟清理机制，移除 CleanupOrphanSstFiles |
