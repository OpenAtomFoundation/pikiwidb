#!/bin/bash

cd /home/pika/caiyu/pikiwidb || exit

clean_ports() {
  echo "Checking and cleaning ports..."

  sudo killall -9 pika

  sleep 1
}

echo "Building project..."
sudo ./build.sh
if [ $? -ne 0 ]; then
  echo "Build failed!"
  exit 1
fi
echo "Build successful."

echo "Cleaning up test directory..."
sudo rm -rf ./output/pacifica_test/
echo "Cleanup completed."

clean_ports

cd output || exit

echo "Starting master and slave servers..."
sudo ../tests/integration/start_master_and_slave.sh
sleep 10

echo "Setting up strong consistency replication..."

redis-cli -p 9302 slaveof 127.0.0.1 9301 strong
sleep 1
echo "Replication setup successful."

echo "Running benchmark..."

# redis-cli -p 9301 set key "12313"
redis-benchmark -p 9301 -t set -n 100000 -c 10 --threads 1
echo "Benchmark finished." 

echo -e "\n==== 主节点 INFO 日志 ===="
tail -n 150 ./pacifica_test/master/log/pika.INFO

echo -e "\n==== 主节点 WARNING 日志 ===="
tail -n 150 ./pacifica_test/master/log/pika.WARNING 

echo -e "\n==== 从节点 INFO 日志 ===="
tail -n 150 ./pacifica_test/slave1/log/pika.INFO

echo -e "\n==== 从节点 WARNING 日志 ===="
tail -n 150 ./pacifica_test/slave1/log/pika.WARNING 