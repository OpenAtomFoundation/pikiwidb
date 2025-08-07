#!/bin/bash

# 切换到项目根目录
cd /home/pika/caiyu/pikiwidb || exit

# 定义清理端口函数
clean_ports() {
  echo "Checking and cleaning ports..."
  
  sudo killall -9 pika
  
  # 等待端口完全释放
  sleep 1
}

# 编译项目
echo "Building project..."
sudo ./build.sh
if [ $? -ne 0 ]; then
  echo "Build failed!"
  exit 1
fi
echo "Build successful."

# 清理测试目录
echo "Cleaning up test directory..."
sudo rm -rf ./output/pacifica_test/
echo "Cleanup completed."

# 清理占用端口的进程
clean_ports

# 切换到输出目录
cd output || exit

# 启动主从服务器
echo "Starting master and slave servers..."
sudo ../tests/integration/start_master_and_slave.sh
echo "Waiting for servers to fully initialize..."
sleep 10

# 设置主从强一致性关系
echo "Setting up strong consistency replication..."
redis-cli -p 9302 slaveof 127.0.0.1 9301 strong
if [ $? -ne 0 ]; then
  echo "Failed to set slaveof."
  exit 1
fi
echo "Replication setup successful."

# 等待主从复制完成
echo "Waiting for replication to be fully established..."
sleep 1

# 执行 benchmark
echo "Running benchmark..."
redis-benchmark -p 9301 -t set -n 100000 -c 20 --threads 20
echo "Benchmark finished." 

# 打印日志信息
echo -e "\n==== 主节点 INFO 日志 ===="
tail -n 150 ./pacifica_test/master/log/pika.INFO

echo -e "\n==== 主节点 WARNING 日志 ===="
tail -n 150 ./pacifica_test/master/log/pika.WARNING 

echo -e "\n==== 从节点 INFO 日志 ===="
tail -n 150 ./pacifica_test/slave1/log/pika.INFO

echo -e "\n==== 从节点 WARNING 日志 ===="
tail -n 150 ./pacifica_test/slave1/log/pika.WARNING 