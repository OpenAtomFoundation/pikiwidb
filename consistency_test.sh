#!/usr/bin/env bash

PIKIWIDB_ROOT=.
PIKIWIDB_BIN=$PIKIWIDB_ROOT/bin/pikiwidb
PIKIWIDB_CONF=$PIKIWIDB_ROOT/pikiwidb.conf
PIKIWIDB_BUILD_DIR=$PIKIWIDB_ROOT/build

function assert() {
  local command="$1"
  local expected_result="$2"
  local result=$(eval "$command")

  if [[ $result != "$expected_result" ]]; then
    echo "Command '$command' failed. Expected result: '$expected_result', actual result: '$result'."
    exit 1
  else
    echo "Executed the command correctly: $command"
  fi
}

function start_node() {
  num=$1
  node_path=$PIKIWIDB_BUILD_DIR/node$num
	rm -rf $node_path
	mkdir -p $node_path
  cp $PIKIWIDB_CONF $node_path/node.conf
  sed -i "s|db-path ./db|db-path ./build/node${num}|" $node_path/node.conf
  sed -i "s/port 9221/port 12${num}00/" $node_path/node.conf
  ${PIKIWIDB_BIN} $node_path/node.conf 1>$node_path/node.log 2>&1 &

  echo "node${num} started"
}

start_node 1
start_node 2
start_node 3

sleep 3

assert "redis-cli -p 12100 raft.cluster init" "OK"
assert "redis-cli -p 12200 raft.cluster join 127.0.0.1:12100" "OK"
assert "redis-cli -p 12300 raft.cluster join 127.0.0.1:12100" "OK"

assert "redis-cli -p 12100 hset hash fa va fb vb fc vc" "3"
assert "redis-cli -p 12100 hgetall hash" "fa
va
fb
vb
fc
vc"
sleep 1
assert "redis-cli -p 12200 hgetall hash" "fa
va
fb
vb
fc
vc"
assert "redis-cli -p 12300 hgetall hash" "fa
va
fb
vb
fc
vc"

assert "redis-cli -p 12100 hdel hash fb" "1"
sleep 1
assert "redis-cli -p 12100 hgetall hash" "fa
va
fc
vc"
assert "redis-cli -p 12200 hgetall hash" "fa
va
fc
vc"
assert "redis-cli -p 12300 hgetall hash" "fa
va
fc
vc"

echo "Synchronized data in the cluster successfully "

assert "redis-cli -p 12200 hset hash fd vd" "ERR MOVED 127.0.0.1:12100"
