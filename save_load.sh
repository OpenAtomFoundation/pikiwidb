#!/bin/bash
killall -9 pikiwidb
mkdir leader follower1 follower2

cd leader && ulimit -n 99999  && rm -fr *  && ../bin/pikiwidb ../pikiwidb.conf --port 7777 &

cd follower1 && ulimit -n 99999 && rm -fr * && ../bin/pikiwidb ../pikiwidb.conf --port 8888 &
sleep 10
redis-cli -p 7777 raft.cluster init 
redis-benchmark -p 7777 -c 5 -n 10000 -r 10000000 -d 1024 -t set


redis-cli -p 7777 raft.node DSS 
redis-cli -p 7777 raft.node DSS

redis-cli -p 8888 raft.cluster join 127.0.0.1:7777



