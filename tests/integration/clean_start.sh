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
# 不使用sudo运行脚本以避免权限问题
sudo ../tests/integration/start_master_and_slave.sh
sleep 15

# 设置主从强一致性关系
echo "Setting up strong consistency replication..."

redis-cli -p 9302 slaveof 127.0.0.1 9301 strong
sleep 1
echo "Replication setup successful."

# 执行 benchmark
echo "Running benchmark..."

redis-benchmark -p 9301 -t set -r 100000 -n 100000 -c 500 --threads 4
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