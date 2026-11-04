package redis

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestMasterInfoReplication(t *testing.T) {
	text := `
# Replication(MASTER)
role:master
ReplicationID:94e8feeaf9036a77c59ad2f091f1c0b0858047f06fa1e09afa
connected_slaves:1
slave0:ip=10.224.129.104,port=9971,conn_fd=104,lag=(db0:0)
db0:binlog_offset=2 384,safety_purge=none
`
	res, err := parseInfoReplication(text)
	if err != nil {
		fmt.Println(err)
		return
	}

	assert.Equal(t, res.DbBinlogFileNum, uint64(2), "db0 binlog file_num not right")
	assert.Equal(t, res.DbBinlogOffset, uint64(384), "db0 binlog offset not right")
	assert.Equal(t, len(res.Slaves), 1, "slaves numbers not right")
	assert.Equal(t, res.Slaves[0].IP, "10.224.129.104", "slave0 IP not right")
	assert.Equal(t, res.Slaves[0].Port, "9971", "slave0 Port not right")
	assert.Equal(t, res.Slaves[0].Lag, 0, "slave0 lag not right")
}

func TestParseSlaveLag(t *testing.T) {
	// pika per-db format
	assert.Equal(t, 0, parseSlaveLag("(db0:0)"), "caught up should be 0")
	assert.Equal(t, 128, parseSlaveLag("(db0:128)"), "single db lag")
	// multiple dbs: max lag wins (any db behind => behind)
	assert.Equal(t, 512, parseSlaveLag("(db0:128)(db1:512)"), "multi db should take max")
	assert.Equal(t, 0, parseSlaveLag("(db0:0)(db1:0)"), "multi db all caught up")
	// non-numeric states must never look caught up
	assert.Equal(t, slaveLagUnknown, parseSlaveLag("(db0:full syncing)"), "full syncing not caught up")
	assert.Equal(t, slaveLagUnknown, parseSlaveLag("(db0:not syncing)"), "not syncing not caught up")
	// backward compatible bare integer
	assert.Equal(t, 42, parseSlaveLag("42"), "bare integer")
	assert.Equal(t, 0, parseSlaveLag(""), "empty is zero")
	// garbage without parens is treated as unknown
	assert.Equal(t, slaveLagUnknown, parseSlaveLag("garbage"), "garbage not caught up")
}

func TestMasterInfoReplicationLag(t *testing.T) {
	text := `
# Replication(MASTER)
role:master
connected_slaves:2
slave0:ip=10.0.0.1,port=9971,conn_fd=104,lag=(db0:0)
slave1:ip=10.0.0.2,port=9971,conn_fd=105,lag=(db0:2048)
db0:binlog_offset=2 384,safety_purge=none
`
	res, err := parseInfoReplication(text)
	assert.NoError(t, err)
	assert.Equal(t, 2, len(res.Slaves), "slaves numbers not right")
	assert.Equal(t, 0, res.Slaves[0].Lag, "slave0 caught up")
	assert.Equal(t, 2048, res.Slaves[1].Lag, "slave1 lag 2048")
}

func TestSlaveInfoReplication(t *testing.T) {
	text := `
# Replication(SLAVE)
role:slave
ReplicationID:94e8feeaf9036a77c59ad2f091f1c0b0858047f06fa1e09afa
master_host:10.224.129.40
master_port:9971
master_link_status:up
slave_priority:100
slave_read_only:1
db0:binlog_offset=1 284,safety_purge=none
`
	res, err := parseInfoReplication(text)
	if err != nil {
		fmt.Println(err)
		return
	}

	assert.Equal(t, res.DbBinlogFileNum, uint64(1), "db0 binlog file_num not right")
	assert.Equal(t, res.DbBinlogOffset, uint64(284), "db0 binlog offset not right")
	assert.Equal(t, len(res.Slaves), 0)
}
