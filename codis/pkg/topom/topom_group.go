// Copyright 2016 CodisLabs. All Rights Reserved.
// Licensed under the MIT (MIT-LICENSE.txt) license.

package topom

import (
	"encoding/json"
	"net"
	"time"

	"pika/codis/v2/pkg/models"
	"pika/codis/v2/pkg/utils/errors"
	"pika/codis/v2/pkg/utils/log"
	"pika/codis/v2/pkg/utils/redis"
)

func (s *Topom) CreateGroup(gid int) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	if gid <= 0 || gid > models.MaxGroupId {
		return errors.Errorf("invalid group id = %d, out of range", gid)
	}
	if ctx.group[gid] != nil {
		return errors.Errorf("group-[%d] already exists", gid)
	}
	defer s.dirtyGroupCache(gid)

	g := &models.Group{
		Id:      gid,
		Servers: []*models.GroupServer{},
	}
	return s.storeCreateGroup(g)
}

func (s *Topom) RemoveGroup(gid int) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}
	if len(g.Servers) != 0 {
		return errors.Errorf("group-[%d] isn't empty", gid)
	}
	defer s.dirtyGroupCache(g.Id)

	return s.storeRemoveGroup(g)
}

func (s *Topom) ResyncGroup(gid int) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}

	if err := s.resyncSlotMappingsByGroupId(ctx, gid); err != nil {
		log.Warnf("group-[%d] resync-group failed", g.Id)
		return err
	}
	defer s.dirtyGroupCache(gid)

	g.OutOfSync = false
	return s.storeUpdateGroup(g)
}

func (s *Topom) ResyncGroupAll() error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	for _, g := range ctx.group {
		if err := s.resyncSlotMappingsByGroupId(ctx, g.Id); err != nil {
			log.Warnf("group-[%d] resync-group failed", g.Id)
			return err
		}
		defer s.dirtyGroupCache(g.Id)

		g.OutOfSync = false
		if err := s.storeUpdateGroup(g); err != nil {
			return err
		}
	}
	return nil
}

func (s *Topom) GroupAddServer(gid int, dc, addr string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	if addr == "" {
		return errors.Errorf("invalid server address")
	}

	for _, g := range ctx.group {
		for _, x := range g.Servers {
			if x.Addr == addr {
				return errors.Errorf("server-[%s] already exists", addr)
			}
		}
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}
	if g.Promoting.State != models.ActionNothing {
		return errors.Errorf("group-[%d] is promoting", g.Id)
	}

	if p := ctx.sentinel; len(p.Servers) != 0 {
		defer s.dirtySentinelCache()
		p.OutOfSync = true
		if err := s.storeUpdateSentinel(p); err != nil {
			return err
		}
	}
	defer s.dirtyGroupCache(g.Id)

	g.Servers = append(g.Servers, &models.GroupServer{Addr: addr, DataCenter: dc})
	return s.storeUpdateGroup(g)
}

func (s *Topom) GroupDelServer(gid int, addr string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}
	index, err := ctx.getGroupIndex(g, addr)
	if err != nil {
		return err
	}

	if g.Promoting.State != models.ActionNothing {
		return errors.Errorf("group-[%d] is promoting", g.Id)
	}

	if index == 0 {
		if len(g.Servers) != 1 || ctx.isGroupInUse(g.Id) {
			return errors.Errorf("group-[%d] can't remove master, still in use", g.Id)
		}
	}

	if p := ctx.sentinel; len(p.Servers) != 0 {
		defer s.dirtySentinelCache()
		p.OutOfSync = true
		if err := s.storeUpdateSentinel(p); err != nil {
			return err
		}
	}
	defer s.dirtyGroupCache(g.Id)

	if index != 0 && g.Servers[index].ReplicaGroup {
		g.OutOfSync = true
	}

	var slice = make([]*models.GroupServer, 0, len(g.Servers))
	for i, x := range g.Servers {
		if i != index {
			slice = append(slice, x)
		}
	}
	if len(slice) == 0 {
		g.OutOfSync = false
	}

	g.Servers = slice

	return s.storeUpdateGroup(g)
}

func (s *Topom) GroupPromoteServer(gid int, addr string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}
	index, err := ctx.getGroupIndex(g, addr)
	if err != nil {
		return err
	}

	if g.Promoting.State != models.ActionNothing {
		if index != g.Promoting.Index {
			return errors.Errorf("group-[%d] is promoting index = %d", g.Id, g.Promoting.Index)
		}
	} else {
		if index == 0 {
			return errors.Errorf("group-[%d] can't promote master", g.Id)
		}
	}
	if n := s.action.executor.Int64(); n != 0 {
		return errors.Errorf("slots-migration is running = %d", n)
	}

	switch g.Promoting.State {

	case models.ActionNothing:

		defer s.dirtyGroupCache(g.Id)

		log.Warnf("group-[%d] will promote index = %d", g.Id, index)

		g.Promoting.Index = index
		g.Promoting.State = models.ActionPreparing
		if err := s.storeUpdateGroup(g); err != nil {
			return err
		}

		fallthrough

	case models.ActionPreparing:

		defer s.dirtyGroupCache(g.Id)

		log.Warnf("group-[%d] resync to prepared", g.Id)

		slots := ctx.getSlotMappingsByGroupId(g.Id)

		g.Promoting.State = models.ActionPrepared
		if err := s.resyncSlotMappings(ctx, slots...); err != nil {
			log.Warnf("group-[%d] resync-rollback to preparing", g.Id)
			g.Promoting.State = models.ActionPreparing
			s.resyncSlotMappings(ctx, slots...)
			log.Warnf("group-[%d] resync-rollback to preparing, done", g.Id)
			return err
		}
		if err := s.storeUpdateGroup(g); err != nil {
			return err
		}

		fallthrough

	case models.ActionPrepared:

		if p := ctx.sentinel; len(p.Servers) != 0 {
			defer s.dirtySentinelCache()
			p.OutOfSync = true
			if err := s.storeUpdateSentinel(p); err != nil {
				return err
			}
			if s.ha.masters != nil {
				delete(s.ha.masters, gid)
			}
		}

		defer s.dirtyGroupCache(g.Id)

		var index = g.Promoting.Index

		// Before rearranging the servers, capture the current master (old
		// master) and the server being promoted (new master). If configured,
		// pause writes on the old master and wait for the new master to catch
		// up its binlog offset, so the old master can later become a slave of
		// the new master without a stale-offset slaveof failure.
		oldMasterAddr := g.Servers[0].Addr
		newMasterAddr := g.Servers[index].Addr
		// forceOldMasterFullSync records whether the old master must do a full
		// resync (slaveof ... force) because the new master could not catch up
		// within the timeout and the configured policy is "force".
		var forceOldMasterFullSync bool
		if s.config.PromotePauseWrite {
			aligned, werr := s.waitCatchUpBeforePromote(oldMasterAddr, newMasterAddr)
			if !aligned {
				switch s.config.PromoteAlignOnTimeout {
				case "force":
					// Proceed with the switch; the old master will full-resync.
					forceOldMasterFullSync = true
					log.Warnf("group-[%d] promote: new master[%s] not caught up, proceeding with force full sync of old master[%s]",
						g.Id, newMasterAddr, oldMasterAddr)
				default: // "abort"
					// Restore the old master and roll back the promotion.
					_ = setPauseWrite(oldMasterAddr, s.config.ProductAuth, false)
					if werr != nil {
						log.Warnf("group-[%d] promote aborted, err:%v", g.Id, werr)
					}
					g.Promoting.Index = 0
					g.Promoting.State = models.ActionNothing
					if serr := s.storeUpdateGroup(g); serr != nil {
						return serr
					}
					if werr != nil {
						return werr
					}
					return errors.Errorf("group-[%d] promote aborted: new master[%s] failed to catch up old master[%s] within %s",
						g.Id, newMasterAddr, oldMasterAddr, s.config.PromoteAlignTimeout)
				}
			}
		}

		var slice = make([]*models.GroupServer, 0, len(g.Servers))
		slice = append(slice, g.Servers[index])
		for i, x := range g.Servers {
			if i != index && i != 0 {
				slice = append(slice, x)
			}
		}
		slice = append(slice, g.Servers[0])

		for _, x := range slice {
			x.Action.Index = 0
			x.Action.State = models.ActionNothing
		}

		g.Servers = slice
		g.Promoting.Index = 0
		g.Promoting.State = models.ActionFinished
		if err := s.storeUpdateGroup(g); err != nil {
			return err
		}
		// Promote the new master (slaveof no one).
		_ = promoteServerToNewMaster(newMasterAddr, s.config.ProductAuth)

		// Demote the old master to a slave of the new master in the same flow.
		// This must not be left to the heartbeat fixer: the old master is still
		// in master role (and write-paused), and a plain (non-force) slaveof
		// from a master is rejected by pika ("already master of others").
		if s.config.PromotePauseWrite {
			s.demoteOldMasterToSlave(oldMasterAddr, newMasterAddr, forceOldMasterFullSync)
		}
		fallthrough

	case models.ActionFinished:

		log.Warnf("group-[%d] resync to finished", g.Id)

		slots := ctx.getSlotMappingsByGroupId(g.Id)

		if err := s.resyncSlotMappings(ctx, slots...); err != nil {
			log.Warnf("group-[%d] resync to finished failed", g.Id)
			return err
		}
		defer s.dirtyGroupCache(g.Id)

		g = &models.Group{
			Id:      g.Id,
			Servers: g.Servers,
		}
		return s.storeUpdateGroup(g)

	default:

		return errors.Errorf("group-[%d] action state is invalid", gid)

	}
}

func (s *Topom) tryFixReplicationRelationships(ctx *context, recoveredGroupServers []*redis.ReplicationState) {
	for _, state := range recoveredGroupServers {
		log.Infof("group-[%d] try to fix server[%v-%v] replication relationship", state.GroupID, state.Index, state.Addr)
		group, err := ctx.getGroup(state.GroupID)
		if err != nil {
			log.Error(err)
			continue
		}

		group.OutOfSync = true
		err = s.storeUpdateGroup(group)
		if err != nil {
			s.dirtyGroupCache(group.Id)
			continue
		}

		err = s.tryFixReplicationRelationship(group, state.Server, state)
		if err != nil {
			log.Warnf("group-[%d] fix server[%v] replication relationship failed, err: %v", group.Id, state.Addr, err)
			continue
		}

		// Notify all servers to update slot information
		slots := ctx.getSlotMappingsByGroupId(group.Id)
		if err = s.resyncSlotMappings(ctx, slots...); err != nil {
			log.Warnf("group-[%d] notify all proxy failed, %v", group.Id, err)
			continue
		} else {
			group.OutOfSync = false
			_ = s.storeUpdateGroup(group)
			s.dirtyGroupCache(group.Id)
		}
	}
}

// tryFixReplicationRelationship
//
// master or slave have already recovered service, fix its master-slave replication relationship.
// only fix which the old state of GroupServer is GroupServerStateOffline.
// It will only update the state of GroupServer to GroupServerStateNormal, If the GroupServer have right
// master-slave replication relationship.
func (s *Topom) tryFixReplicationRelationship(group *models.Group, groupServer *models.GroupServer, state *redis.ReplicationState) (err error) {
	curMasterAddr := group.Servers[0].Addr
	if isGroupMaster(state, group) {
		// execute the command `slaveof no one`
		if models.GroupServerRole(state.Replication.Role) != models.RoleMaster {
			if err = promoteServerToNewMaster(state.Addr, s.config.ProductAuth); err != nil {
				return err
			}
		}
	} else {
		if state.Replication.GetMasterAddr() != curMasterAddr {
			// current server is slave, execute the command `slaveof [new master ip] [new master port]`
			if err = updateMasterToNewOne(groupServer.Addr, curMasterAddr, s.config.ProductAuth); err != nil {
				return err
			}
		}
	}

	groupServer.State = models.GroupServerStateNormal
	groupServer.ReCallTimes = 0
	groupServer.ReplicaGroup = true
	groupServer.Role = models.GroupServerRole(state.Replication.Role)
	groupServer.DbBinlogFileNum = state.Replication.DbBinlogFileNum
	groupServer.DbBinlogOffset = state.Replication.DbBinlogOffset
	groupServer.IsEligibleForMasterElection = state.Replication.IsEligibleForMasterElection
	groupServer.Action.State = models.ActionSynced
	err = s.storeUpdateGroup(group)
	// clean cache whether err is nil or not
	s.dirtyGroupCache(group.Id)
	return err
}

func isGroupMaster(state *redis.ReplicationState, g *models.Group) bool {
	return state.Index == 0 && g.Servers[0].Addr == state.Addr
}

func (s *Topom) updateSlaveOfflineGroups(ctx *context, offlineGroups []*models.Group) {
	for _, group := range offlineGroups {
		log.Infof("group-[%d] update slave offline state", group.Id)
		group.OutOfSync = true
		err := s.storeUpdateGroup(group)
		if err != nil {
			s.dirtyGroupCache(group.Id)
			continue
		}

		// Notify all servers to update slot information
		slots := ctx.getSlotMappingsByGroupId(group.Id)
		if err := s.resyncSlotMappings(ctx, slots...); err != nil {
			log.Warnf("group-[%d] notify all proxy failed, %v", group.Id, err)
			continue
		}
	}
}

// trySwitchGroupsToNewMaster
//
// the master have already been offline, and it will select and switch to a new master from the Group
func (s *Topom) trySwitchGroupsToNewMaster(ctx *context, masterOfflineGroups []*models.Group) {
	for _, group := range masterOfflineGroups {
		log.Infof("group-[%d] try to switch new master", group.Id)
		group.OutOfSync = true
		err := s.storeUpdateGroup(group)
		if err != nil {
			s.dirtyGroupCache(group.Id)
			continue
		}

		// try to switch to new master
		if err := s.trySwitchGroupMaster(group); err != nil {
			log.Errorf("group-[%d] switch master failed, %v", group.Id, err)
			continue
		}

		// Notify all servers to update slot information
		slots := ctx.getSlotMappingsByGroupId(group.Id)
		if err := s.resyncSlotMappings(ctx, slots...); err != nil {
			log.Warnf("group-[%d] notify all proxy failed, %v", group.Id, err)
			continue
		} else {
			group.OutOfSync = false
			_ = s.storeUpdateGroup(group)
			s.dirtyGroupCache(group.Id)
		}
	}
}

func (s *Topom) trySwitchGroupMaster(group *models.Group) error {
	newMasterAddr, newMasterIndex := group.SelectNewMaster()
	if newMasterAddr == "" {
		servers, _ := json.Marshal(group)
		log.Errorf("group %d don't has any slaves to switch master, %s", group.Id, servers)
		return errors.Errorf("can't switch slave to master")
	}

	// TODO liuchengyu check new master is available
	//available := isAvailableAsNewMaster(masterServer, s.Config())
	//if !available {
	//	return ""
	//}

	return s.doSwitchGroupMaster(group, newMasterAddr, newMasterIndex)
}

func isAvailableAsNewMaster(groupServer *models.GroupServer, conf *Config) bool {
	rc, err := redis.NewClient(groupServer.Addr, conf.ProductAuth, 500*time.Millisecond)
	if err != nil {
		log.Warnf("connect GroupServer[%v] failed!, error:%v", groupServer.Addr, err)
		return false
	}
	defer rc.Close()

	info, err := rc.InfoReplication()
	if err != nil {
		log.Warnf("get InfoReplication from GroupServer[%v] failed!, error:%v", groupServer.Addr, err)
		return false
	}

	if info.MasterLinkStatus == "down" {
		// down state means the slave does not finished full sync from master
		log.Warnf("the master_link_status of GroupServer[%v] is down state. it cannot be selected as master", groupServer.Addr)
		return false
	}

	return true
}

func (s *Topom) doSwitchGroupMaster(g *models.Group, newMasterAddr string, newMasterIndex int) (err error) {
	if newMasterIndex <= 0 || newMasterAddr == "" {
		return nil
	}

	log.Warnf("group-[%d] will switch master to server[%d] = %s", g.Id, newMasterIndex, newMasterAddr)

	// If the old master is still reachable (e.g. an intentional switch rather
	// than a crash), pause writes on it and wait for the new master to catch
	// up its binlog offset before promoting, so the old master can rejoin as a
	// slave without a stale-offset slaveof failure. When the old master is
	// offline (the common HA failover case), there is nothing to pause and no
	// live write stream, so we skip alignment and rely on SelectNewMaster
	// having already picked the slave with the latest offset.
	oldMaster := g.Servers[0]
	forceOldMasterFullSync := false
	pausedOldMaster := false
	if s.config.PromotePauseWrite && oldMaster.State == models.GroupServerStateNormal && oldMaster.Addr != newMasterAddr {
		aligned, werr := s.waitCatchUpBeforePromote(oldMaster.Addr, newMasterAddr)
		pausedOldMaster = true
		if !aligned {
			if s.config.PromoteAlignOnTimeout == "force" {
				forceOldMasterFullSync = true
				log.Warnf("group-[%d] switch: new master[%s] not caught up, proceeding with force full sync of old master[%s]",
					g.Id, newMasterAddr, oldMaster.Addr)
			} else {
				// abort: restore the old master and cancel the switch
				_ = setPauseWrite(oldMaster.Addr, s.config.ProductAuth, false)
				if werr != nil {
					log.Warnf("group-[%d] switch aborted, err:%v", g.Id, werr)
				}
				return errors.Errorf("group-[%d] switch aborted: new master[%s] failed to catch up old master[%s] within %s",
					g.Id, newMasterAddr, oldMaster.Addr, s.config.PromoteAlignTimeout)
			}
		}
	}

	// Set the slave node as the new master node
	if err = promoteServerToNewMaster(newMasterAddr, s.config.ProductAuth); err != nil {
		if pausedOldMaster {
			_ = setPauseWrite(oldMaster.Addr, s.config.ProductAuth, false)
		}
		return errors.Errorf("promote server[%v] to new master failed, err:%v", newMasterAddr, err)
	}

	g.Servers[newMasterIndex].Role = models.RoleMaster
	g.Servers[newMasterIndex].Action.State = models.ActionSynced
	g.Servers[0], g.Servers[newMasterIndex] = g.Servers[newMasterIndex], g.Servers[0]
	defer func() {
		err = s.storeUpdateGroup(g)
		// clean cache whether err is nil or not
		s.dirtyGroupCache(g.Id)
	}()

	// Set other nodes in the group as slave nodes of the new master node
	for _, server := range g.Servers {
		if server.State != models.GroupServerStateNormal || server.Addr == newMasterAddr {
			continue
		}

		// The paused old master is still in master role: clear it first, then
		// attach it (force full sync if the new master could not catch up).
		if pausedOldMaster && server.Addr == oldMaster.Addr {
			s.demoteOldMasterToSlave(oldMaster.Addr, newMasterAddr, forceOldMasterFullSync)
			server.Action.State = models.ActionSynced
			server.Role = models.RoleSlave
			continue
		}

		if server.IsEligibleForMasterElection {
			err = updateMasterToNewOne(server.Addr, newMasterAddr, s.config.ProductAuth)
		} else {
			err = updateMasterToNewOneForcefully(server.Addr, newMasterAddr, s.config.ProductAuth)
		}

		if err != nil {
			// skip err, and retry to update master-slave replication relationship through next heartbeat check
			err = nil
			server.Action.State = models.ActionSyncedFailed
			server.State = models.GroupServerStateOffline
			log.Warnf("group-[%d] update server[%d] replication relationship failed, new master: %s", g.Id, newMasterIndex, newMasterAddr)
		} else {
			server.Action.State = models.ActionSynced
			server.Role = models.RoleSlave
		}
	}

	return err
}

func updateMasterToNewOne(serverAddr, masterAddr string, auth string) (err error) {
	log.Infof("[%s] switch master to server [%s]", serverAddr, masterAddr)
	return setNewRedisMaster(serverAddr, masterAddr, auth, false)
}

func promoteServerToNewMaster(serverAddr, auth string) (err error) {
	log.Infof("[%s] switch master to NO:ONE", serverAddr)
	return setNewRedisMaster(serverAddr, "NO:ONE", auth, false)
}

func updateMasterToNewOneForcefully(serverAddr, masterAddr string, auth string) (err error) {
	log.Infof("[%s] switch master to server [%s] forcefully", serverAddr, masterAddr)
	return setNewRedisMaster(serverAddr, masterAddr, auth, true)
}

func setNewRedisMaster(serverAddr, masterAddr string, auth string, force bool) (err error) {
	var rc *redis.Client
	if rc, err = redis.NewClient(serverAddr, auth, 500*time.Millisecond); err != nil {
		return errors.Errorf("create redis client to %s failed, err:%v", serverAddr, err)
	}
	defer rc.Close()
	if err = rc.SetMaster(masterAddr, force); err != nil {
		return errors.Errorf("server[%s] set master to %s failed, force:%v err:%v", serverAddr, masterAddr, force, err)
	}
	return err
}

// setPauseWrite toggles the role-independent write-pause switch on a pika
// instance. It is used to stop / resume writes on the old master during a
// master switch.
func setPauseWrite(serverAddr, auth string, pause bool) (err error) {
	var rc *redis.Client
	if rc, err = redis.NewClient(serverAddr, auth, 500*time.Millisecond); err != nil {
		return errors.Errorf("create redis client to %s failed, err:%v", serverAddr, err)
	}
	defer rc.Close()
	if err = rc.SetPauseWrite(pause); err != nil {
		return errors.Errorf("server[%s] set pause-write to %v failed, err:%v", serverAddr, pause, err)
	}
	return nil
}

// getBinlogOffset reads the current binlog (file_num, offset) of a pika
// instance via `info replication`.
func getBinlogOffset(serverAddr, auth string) (fileNum uint64, offset uint64, err error) {
	var rc *redis.Client
	if rc, err = redis.NewClient(serverAddr, auth, 500*time.Millisecond); err != nil {
		return 0, 0, errors.Errorf("create redis client to %s failed, err:%v", serverAddr, err)
	}
	defer rc.Close()
	info, err := rc.InfoReplication()
	if err != nil {
		return 0, 0, errors.Errorf("get info replication from %s failed, err:%v", serverAddr, err)
	}
	return info.DbBinlogFileNum, info.DbBinlogOffset, nil
}

// getSlaveLagFromMaster reads the old master's `info replication` and returns
// the replication lag (in bytes) that the master reports for the slave at
// slaveAddr, i.e. how far behind that slave is from the master's current binlog
// producer offset.
//
// This is the correct "has the slave caught up" signal: the master computes it
// from a single, consistent binlog coordinate (see pika GetSlaveListString),
// unlike comparing each node's own independent binlog producer offset. `found`
// is false when the slave is not present in the master's slave list yet (e.g.
// the replication link is not established); callers MUST treat that as "not
// caught up", never as lag 0.
func getSlaveLagFromMaster(masterAddr, slaveAddr, auth string) (lag uint64, found bool, err error) {
	var rc *redis.Client
	if rc, err = redis.NewClient(masterAddr, auth, 500*time.Millisecond); err != nil {
		return 0, false, errors.Errorf("create redis client to %s failed, err:%v", masterAddr, err)
	}
	defer rc.Close()
	info, err := rc.InfoReplication()
	if err != nil {
		return 0, false, errors.Errorf("get info replication from %s failed, err:%v", masterAddr, err)
	}

	slaveHost, slavePort, serr := net.SplitHostPort(slaveAddr)
	if serr != nil {
		return 0, false, errors.Errorf("invalid slave addr %s, err:%v", slaveAddr, serr)
	}
	for _, sl := range info.Slaves {
		if sl.IP == slaveHost && sl.Port == slavePort {
			if sl.Lag < 0 {
				return 0, true, nil
			}
			return uint64(sl.Lag), true, nil
		}
	}
	return 0, false, nil
}

// waitCatchUpBeforePromote pauses writes on the old master and then polls the
// old master's own view of the target slave's replication lag until it reaches
// the configured threshold, or until the configured timeout elapses.
//
// Why the old master's view: pika's `db0:binlog_offset` is each node's OWN
// binlog producer offset, so comparing the new master's value against the old
// master's is meaningless (independent binlog coordinates). The old master, on
// the other hand, reports each connected slave's lag against a single binlog
// coordinate (`slaveN:...,lag=(db0:N)`), which is the correct catch-up signal.
//
// On success it returns aligned=true with writes still paused on the old master
// (the caller performs the subsequent slaveof / resume). If pausing writes
// fails it returns err with pause best-effort cleared. If the timeout elapses
// without catching up it returns aligned=false (writes remain paused) and lets
// the caller decide whether to abort (default) or force per config.
//
// NOTE: the caller (GroupPromoteServer) holds s.mu while this runs, so the
// timeout is kept short (default 15s) to bound how long the dashboard lock is
// held.
func (s *Topom) waitCatchUpBeforePromote(oldMasterAddr, newMasterAddr string) (aligned bool, err error) {
	auth := s.config.ProductAuth

	// 1. Stop writes on the old master so its binlog offset stops advancing.
	if err = setPauseWrite(oldMasterAddr, auth, true); err != nil {
		// best effort resume, then bail out
		_ = setPauseWrite(oldMasterAddr, auth, false)
		return false, errors.Errorf("pause write on old master[%s] failed, err:%v", oldMasterAddr, err)
	}
	log.Warnf("promote: paused write on old master[%s], waiting new master[%s] to catch up", oldMasterAddr, newMasterAddr)

	// Log the old master's frozen producer offset for operability (not used for
	// the catch-up decision, which is lag-based).
	if oldFileNum, oldOffset, gerr := getBinlogOffset(oldMasterAddr, auth); gerr == nil {
		log.Warnf("promote: old master[%s] frozen at (file:%d offset:%d)", oldMasterAddr, oldFileNum, oldOffset)
	}

	// 2. Poll the old master's view of the new master's replication lag until it
	// falls within the threshold or the timeout elapses.
	threshold := s.config.PromoteAlignOffsetThreshold
	deadline := time.Now().Add(s.config.PromoteAlignTimeout.Duration())
	for {
		lag, found, gerr := getSlaveLagFromMaster(oldMasterAddr, newMasterAddr, auth)
		if gerr != nil {
			// transient read error; keep retrying until the deadline
			log.Warnf("promote: read slave[%s] lag from old master[%s] failed, err:%v", newMasterAddr, oldMasterAddr, gerr)
		} else if !found {
			// new master not yet in old master's slave list: not caught up
			log.Warnf("promote: new master[%s] not present in old master[%s] slave list yet", newMasterAddr, oldMasterAddr)
		} else if lag <= threshold {
			log.Warnf("promote: new master[%s] caught up old master[%s] (lag:%d <= threshold:%d)",
				newMasterAddr, oldMasterAddr, lag, threshold)
			return true, nil
		} else {
			log.Warnf("promote: new master[%s] lag behind old master[%s]: lag:%d > threshold:%d",
				newMasterAddr, oldMasterAddr, lag, threshold)
		}

		if time.Now().After(deadline) {
			log.Warnf("promote: new master[%s] failed to catch up old master[%s] within %s",
				newMasterAddr, oldMasterAddr, s.config.PromoteAlignTimeout)
			return false, nil
		}
		time.Sleep(200 * time.Millisecond)
	}
}

// demoteOldMasterToSlave turns the paused old master into a slave of the new
// master, then resumes writes on it. The old master is still in master role
// here, and pika rejects a plain (non-force) slaveof from a master
// ("already master of others"); so we first clear its master role with
// `slaveof no one`, then attach it to the new master.
//
// When forceFullSync is true (the new master could not catch up within the
// timeout), we attach with `slaveof ... force` so the old master does a full
// resync. Otherwise the new master has already covered the old master's binlog
// position, so a plain incremental slaveof is enough.
//
// It always makes a best effort to resume writes so the old master can never
// be left permanently read-only.
func (s *Topom) demoteOldMasterToSlave(oldMasterAddr, newMasterAddr string, forceFullSync bool) {
	auth := s.config.ProductAuth
	defer func() {
		// The old master is now a slave (writes are blocked by
		// slave-read-only), but clear the runtime pause switch anyway so it is
		// never left dangling.
		if rerr := setPauseWrite(oldMasterAddr, auth, false); rerr != nil {
			log.Warnf("promote: resume write on old master[%s] failed, err:%v", oldMasterAddr, rerr)
		}
	}()

	// Clear the master role first so a non-force slaveof is accepted.
	if err := promoteServerToNewMaster(oldMasterAddr, auth); err != nil {
		log.Warnf("promote: clear old master[%s] role before demote failed, err:%v", oldMasterAddr, err)
	}

	var derr error
	if forceFullSync {
		derr = updateMasterToNewOneForcefully(oldMasterAddr, newMasterAddr, auth)
	} else {
		derr = updateMasterToNewOne(oldMasterAddr, newMasterAddr, auth)
	}
	if derr != nil {
		// Leave the remaining fix to the heartbeat fixer; writes are resumed by
		// the deferred call above.
		log.Warnf("promote: demote old master[%s] to slave of[%s] failed, err:%v", oldMasterAddr, newMasterAddr, derr)
	}
}

func (s *Topom) EnableReplicaGroups(gid int, addr string, value bool) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, err := ctx.getGroup(gid)
	if err != nil {
		return err
	}
	index, err := ctx.getGroupIndex(g, addr)
	if err != nil {
		return err
	}

	if g.Promoting.State != models.ActionNothing {
		return errors.Errorf("group-[%d] is promoting", g.Id)
	}
	defer s.dirtyGroupCache(g.Id)

	if len(g.Servers) != 1 && ctx.isGroupInUse(g.Id) {
		g.OutOfSync = true
	}
	g.Servers[index].ReplicaGroup = value

	return s.storeUpdateGroup(g)
}

func (s *Topom) EnableReplicaGroupsAll(value bool) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	for _, g := range ctx.group {
		if g.Promoting.State != models.ActionNothing {
			return errors.Errorf("group-[%d] is promoting", g.Id)
		}
		defer s.dirtyGroupCache(g.Id)

		var dirty bool
		for _, x := range g.Servers {
			if x.ReplicaGroup != value {
				x.ReplicaGroup = value
				dirty = true
			}
		}
		if !dirty {
			continue
		}
		if len(g.Servers) != 1 && ctx.isGroupInUse(g.Id) {
			g.OutOfSync = true
		}
		if err := s.storeUpdateGroup(g); err != nil {
			return err
		}
	}
	return nil
}

func (s *Topom) SyncCreateAction(addr string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, index, err := ctx.getGroupByServer(addr)
	if err != nil {
		return err
	}
	if g.Promoting.State != models.ActionNothing {
		return errors.Errorf("group-[%d] is promoting", g.Id)
	}

	if g.Servers[index].Action.State == models.ActionPending {
		return errors.Errorf("server-[%s] action already exist", addr)
	}
	defer s.dirtyGroupCache(g.Id)

	g.Servers[index].Action.Index = ctx.maxSyncActionIndex() + 1
	g.Servers[index].Action.State = models.ActionPending
	return s.storeUpdateGroup(g)
}

func (s *Topom) SyncRemoveAction(addr string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, index, err := ctx.getGroupByServer(addr)
	if err != nil {
		return err
	}
	if g.Promoting.State != models.ActionNothing {
		return errors.Errorf("group-[%d] is promoting", g.Id)
	}

	if g.Servers[index].Action.State == models.ActionNothing {
		return errors.Errorf("server-[%s] action doesn't exist", addr)
	}
	defer s.dirtyGroupCache(g.Id)

	g.Servers[index].Action.Index = 0
	g.Servers[index].Action.State = models.ActionNothing
	return s.storeUpdateGroup(g)
}

func (s *Topom) SyncActionPrepare() (string, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return "", err
	}

	addr := ctx.minSyncActionIndex()
	if addr == "" {
		return "", nil
	}

	g, index, err := ctx.getGroupByServer(addr)
	if err != nil {
		return "", err
	}
	if g.Promoting.State != models.ActionNothing {
		return "", nil
	}

	if g.Servers[index].Action.State != models.ActionPending {
		return "", errors.Errorf("server-[%s] action state is invalid", addr)
	}
	defer s.dirtyGroupCache(g.Id)

	log.Warnf("server-[%s] action prepare", addr)

	g.Servers[index].Action.Index = 0
	g.Servers[index].Action.State = models.ActionSyncing
	return addr, s.storeUpdateGroup(g)
}

func (s *Topom) SyncActionComplete(addr string, failed bool) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return err
	}

	g, index, err := ctx.getGroupByServer(addr)
	if err != nil {
		return nil
	}
	if g.Promoting.State != models.ActionNothing {
		return nil
	}

	if g.Servers[index].Action.State != models.ActionSyncing {
		return nil
	}
	defer s.dirtyGroupCache(g.Id)

	log.Warnf("server-[%s] action failed = %t", addr, failed)

	var state string
	if !failed {
		state = models.ActionSynced
	} else {
		state = models.ActionSyncedFailed
	}
	g.Servers[index].Action.State = state
	// check whether the master is offline through heartbeat, if so, select a new master
	g.Servers[index].State = models.GroupServerStateOffline

	return s.storeUpdateGroup(g)
}

func (s *Topom) newSyncActionExecutor(addr string) (func() error, error) {
	s.mu.Lock()
	defer s.mu.Unlock()
	ctx, err := s.newContext()
	if err != nil {
		return nil, err
	}

	g, index, err := ctx.getGroupByServer(addr)
	if err != nil {
		return nil, nil
	}

	if g.Servers[index].Action.State != models.ActionSyncing {
		return nil, nil
	}

	var masterAddr string
	if index != 0 {
		masterAddr = g.Servers[0].Addr
	}

	return func() error {
		if index != 0 {
			return updateMasterToNewOne(addr, masterAddr, s.config.ProductAuth)
		} else {
			return promoteServerToNewMaster(addr, s.config.ProductAuth)
		}
	}, nil
}
