# Pika 主从同步方案变更分析

## 最近14次提交记录

1. **27c3a838** - fix: 修复 dump 目录为空时返回空文件列表导致同步失败
2. **08ee1c36** - fix: 修复 snapshot 注册失败导致文件被误删
3. **ad54f43a** - fix: 修复 AutoDeleteExpiredDump 中 info 文件路径问题
4. **eb778848** - fix: 修复文件分片传输过程中被提前删除的问题
5. **ee37a586** - fix: 添加 EnsureDirExists 包装函数修复 dump 目录创建失败
6. **c3b83b5b** - feat: 实现方案A - 独立dump + 即时清理机制
7. **e166b072** - fix: 修复RsyncServerConn析构函数死锁问题
8. **6abdfd5b** - fix: 方案D完善 降低传输时的清理频率
9. **403c6eb6** - fix: 方案D完善 - 处理传输中途文件丢失场景
10. **733cd7ba** - fix: 修复全量同步过程中孤儿文件清理导致的文件丢失问题
11. **4ed0ea47** - GetChildren添加IsDir保护防止传入文件路径崩溃
12. **dc16f255** - 修复GetBgSaveMetaData目录遍历崩溃问题
13. **2f3b8337** - 修复目录扫描层级问题
14. **05248b00** - 修复Pika全量同步孤儿文件问题

---

## 当前最终方案

**方案A + 延迟清理 + 完整性检查**：
1. GetBgSaveMetaData 不过滤孤儿文件
2. HandleMetaRsyncRequest 增加完整性检查（重新扫描对比）
3. RemoveTransferringFile 延迟10分钟清理
4. AutoDeleteExpiredDump 调用 ProcessPendingCleanupFiles

---

## 与当前方案不一致的代码分析

### 1. 即时清理代码（c3b83b5b 引入）- 需要修改

**位置**: src/rsync_server.cc:RemoveTransferringFile
**问题**: c3b83b5b 提交实现了即时清理，但当前方案改为延迟清理
**状态**: 已在当前工作区修改为延迟清理

```cpp
// c3b83b5b 中的代码（即时清理）:
if (stat(filepath.c_str(), &st) == 0 && st.st_nlink == 1) {
    pstd::DeleteFile(filepath);  // 立即删除
}

// 当前方案（延迟清理）:
if (stat(filepath.c_str(), &st) == 0 && st.st_nlink == 1) {
    g_pika_server->ScheduleFileForCleanup(filepath, 600);  // 10分钟后删除
}
```

---

### 2. CleanupOrphanSstFiles 函数 - 需要评估

**位置**: src/pika_server.cc:1523
**问题**: 这个函数在 AutoDeleteExpiredDump 中被调用，用于清理孤儿文件
**与延迟清理的关系**:
- CleanupOrphanSstFiles: 定时清理所有孤儿文件（非延迟）
- ProcessPendingCleanupFiles: 清理延迟队列中的文件

**建议**: 保留 CleanupOrphanSstFiles，但降低调用频率或仅在 dump 不再使用时调用

**原因**:
1. 延迟清理只处理传输完成的文件
2. CleanupOrphanSstFiles 处理所有未被延迟清理覆盖的孤儿文件
3. 两者可以共存，作为双保险

---

### 3. GetBgSaveMetaData 的孤儿文件过滤 - 已修改

**位置**: src/pika_db.cc:421
**问题**: 之前提交过滤孤儿文件，当前方案不过滤
**状态**: 已在当前工作区修改为不跳过

```cpp
// 之前（跳过）:
if (st.st_nlink == 1) {
    continue;  // 跳过孤儿文件
}

// 当前（不跳过）:
if (st.st_nlink == 1) {
    // 记录但不跳过
    LOG(INFO) << "[GetBgSaveMetaData] Including orphan SST file: " << fullPath;
}
```

---

### 4. AutoDeleteExpiredDump 的 rate limiting - 需要关注

**位置**: src/pika_server.cc:1766
**问题**: 当前工作区将 120 秒改为 600 秒
**分析**:
```cpp
// HEAD~14: 120秒
if (!active_rsync_snapshots_.empty() && (now - last_cleanup_time < 120))

// 当前工作区: 600秒
if (!active_rsync_snapshots_.empty() && (now - last_cleanup_time < 600))
```
**建议**: 600秒（10分钟）与延迟清理时间一致，合理

---

### 5. rsync_transferring_files_ 保护机制 - 保留

**位置**: include/pika_server.h 和 src/pika_server.cc
**功能**: 跟踪正在传输的文件，防止被 CleanupOrphanSstFiles 误删
**状态**: 应该保留，与延迟清理不冲突

---

### 6. 旧方案D的代码 - 可以删除

**相关提交**:
- 6abdfd5b: 方案D完善 降低传输时的清理频率
- 403c6eb6: 方案D完善 - 处理传输中途文件丢失场景
- 733cd7ba: 修复全量同步过程中孤儿文件清理导致的文件丢失问题

**分析**: 这些提交是在方案A之前的尝试，方案A已经替代了方案D
**但是**: 方案A的实现（c3b83b5b）是基于方案D的改进，不是完全替换
**结论**: 不需要删除，方案A已经整合了这些修复

---

### 7. 早期bug修复 - 保留

**保留的提交**:
- e166b072: 修复RsyncServerConn析构函数死锁问题
- ee37a586: 添加 EnsureDirExists 包装函数
- ad54f43a: 修复 AutoDeleteExpiredDump 中 info 文件路径问题
- 08ee1c36: 修复 snapshot 注册失败导致文件被误删
- 27c3a838: 修复 dump 目录为空时返回空文件列表

**这些与当前方案不冲突，应该保留**

---

## 需要删除或修改的代码清单

### 需要修改的代码

1. **无** - 当前工作区的修改已经覆盖了需要改的地方

### 可能需要删除/禁用的代码

1. **CleanupOrphanSstFiles 中的即时删除逻辑**
   - 可选：改为仅在 dump 不再使用时调用
   - 或者降低调用频率

2. **检查是否有过时的配置项或调试代码**
   - 检查 conf/pika.conf 是否有不需要的配置

### 建议保留但可能未使用的代码

1. **rsync_transferring_files_ 保护机制**
   - 虽然延迟清理减少了即时删除的需求，但作为保护机制仍有用

2. **IsDumpInUse 和 dump_owners_**
   - Scheme A 的核心机制，应该保留

---

## 总结

**当前工作区的修改已经覆盖了主要的不一致**：
1. GetBgSaveMetaData 不过滤孤儿文件
2. RemoveTransferringFile 改为延迟清理
3. HandleMetaRsyncRequest 增加完整性检查
4. AutoDeleteExpiredDump 调用 ProcessPendingCleanupFiles

**唯一需要评估的是**: CleanupOrphanSstFiles 是否还需要在同步期间调用
建议：
- 选项1: 完全禁用 CleanupOrphanSstFiles（依赖延迟清理）
- 选项2: 仅在 dump 不再使用时调用 CleanupOrphanSstFiles（当前行为，可保留）

---

文档生成时间: 2026-03-05
