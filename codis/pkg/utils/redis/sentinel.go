// Copyright 2016 CodisLabs. All Rights Reserved.
// Licensed under the MIT (MIT-LICENSE.txt) license.

package redis

import (
	"encoding/json"
	"net"
	"strconv"
	"strings"
	"time"

	"pika/codis/v2/pkg/models"
)

type SentinelMaster struct {
	Addr  string
	Info  map[string]string
	Epoch int64
}

type MonitorConfig struct {
	Quorum          int
	ParallelSyncs   int
	DownAfter       time.Duration
	FailoverTimeout time.Duration

	NotificationScript   string
	ClientReconfigScript string
}

type SentinelGroup struct {
	Master map[string]string   `json:"master"`
	Slaves []map[string]string `json:"slaves,omitempty"`
}

type InfoSlave struct {
	IP     string `json:"ip"`
	Port   string `json:"port"`
	State  string `json:"state"`
	Offset int    `json:"offset"`
	Lag    int    `json:"lag"`
}

func (i *InfoSlave) UnmarshalJSON(b []byte) error {
	var kvmap map[string]string
	if err := json.Unmarshal(b, &kvmap); err != nil {
		return err
	}

	i.IP = kvmap["ip"]
	i.Port = kvmap["port"]
	i.State = kvmap["state"]

	if val, ok := kvmap["offset"]; ok {
		if intval, err := strconv.Atoi(val); err == nil {
			i.Offset = intval
		}
	}
	if val, ok := kvmap["lag"]; ok {
		i.Lag = parseSlaveLag(val)
	}
	return nil
}

// parseSlaveLag extracts the replication lag (in bytes) of a slave from the
// `lag=` field of a pika master's `info replication` output.
//
// pika emits the lag per-db in the form `(db0:N)` or `(db0:N)(db1:M)...`, where
// N is the byte distance between the master's binlog producer offset and the
// offset already sent to that slave. A plain integer (redis-style) is also
// accepted for backward compatibility. When several dbs are present the max lag
// is returned, so "caught up" requires every db to be caught up. Non-numeric
// states such as `(db0:full syncing)` / `(db0:not syncing)` yield a large
// sentinel value so the slave is never treated as caught up.
func parseSlaveLag(val string) int {
	val = strings.TrimSpace(val)
	if val == "" {
		return 0
	}
	// Backward compatible: a bare integer.
	if intval, err := strconv.Atoi(val); err == nil {
		return intval
	}
	// pika format: one or more (dbX:Y) groups.
	if !strings.Contains(val, "(") {
		return slaveLagUnknown
	}
	maxLag := 0
	sawNumeric := false
	for {
		open := strings.IndexByte(val, '(')
		if open < 0 {
			break
		}
		close := strings.IndexByte(val[open:], ')')
		if close < 0 {
			break
		}
		group := val[open+1 : open+close]
		val = val[open+close+1:]

		colon := strings.LastIndexByte(group, ':')
		if colon < 0 {
			continue
		}
		lagStr := strings.TrimSpace(group[colon+1:])
		intval, err := strconv.Atoi(lagStr)
		if err != nil {
			// e.g. "full syncing" / "not syncing": definitely not caught up.
			return slaveLagUnknown
		}
		sawNumeric = true
		if intval > maxLag {
			maxLag = intval
		}
	}
	if !sawNumeric {
		return slaveLagUnknown
	}
	return maxLag
}

// slaveLagUnknown is a large sentinel lag used when a slave's lag cannot be
// interpreted as "fully caught up" (parse failure, full syncing, etc.). It must
// stay far above any realistic byte threshold so such a slave is never selected
// as an aligned promote target.
const slaveLagUnknown = 1 << 62

type InfoReplication struct {
	Role                        string      `json:"role"`
	ConnectedSlaves             int         `json:"connected_slaves"`
	MasterHost                  string      `json:"master_host"`
	MasterPort                  string      `json:"master_port"`
	MasterLinkStatus            string      `json:"master_link_status"` // down; up
	DbBinlogFileNum             uint64      `json:"binlog_file_num"`    // db0
	DbBinlogOffset              uint64      `json:"binlog_offset"`      // db0
	IsEligibleForMasterElection bool        `json:"is_eligible_for_master_election"`
	Slaves                      []InfoSlave `json:"-"`
}

type ReplicationState struct {
	GroupID     int
	Index       int
	Addr        string
	Server      *models.GroupServer
	Replication *InfoReplication
	Err         error
}

func (i *InfoReplication) GetMasterAddr() string {
	if len(i.MasterHost) == 0 {
		return ""
	}

	return net.JoinHostPort(i.MasterHost, i.MasterPort)
}

func (i *InfoReplication) UnmarshalJSON(b []byte) error {
	var kvmap map[string]string
	if err := json.Unmarshal(b, &kvmap); err != nil {
		return err
	}

	if val, ok := kvmap["connected_slaves"]; ok {
		if intval, err := strconv.Atoi(val); err == nil {
			i.ConnectedSlaves = intval
		}
	}

	i.Role = kvmap["role"]
	i.MasterPort = kvmap["master_port"]
	i.MasterHost = kvmap["master_host"]
	i.MasterLinkStatus = kvmap["master_link_status"]
	i.IsEligibleForMasterElection = kvmap["is_eligible_for_master_election"] == "true"

	if val, ok := kvmap["binlog_file_num"]; ok {
		if intval, err := strconv.ParseUint(val, 10, 64); err == nil {
			i.DbBinlogFileNum = intval
		}
	}

	if val, ok := kvmap["binlog_offset"]; ok {
		if intval, err := strconv.ParseUint(val, 10, 64); err == nil {
			i.DbBinlogOffset = intval
		}
	}

	return nil
}
