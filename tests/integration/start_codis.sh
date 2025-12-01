#!/bin/bash

# Clean up processes and temporary files
pkill -9 pika || true
pkill -9 codis || true
# Wait for processes to fully terminate and release resources
sleep 5
rm -rf /tmp/codis || true
rm -rf codis_data_1 || true
rm -rf codis_data_2 || true

# Clean up temporary directories in codis to free space
rm -rf ../codis/bin || true
rm -rf ../codis/log || true
mkdir -p ../codis/bin
mkdir -p ../codis/log

# Display memory usage for monitoring
free -h || true

# Display available disk space
df -h

CODIS_DASHBOARD_ADDR=127.0.0.1:18080

CODIS_GROUP_1_MASTER=127.0.0.1:8000
CODIS_GROUP_2_MASTER=127.0.0.1:8001

# startup pika server
cp -f ../conf/pika.conf ./pika_8000.conf
cp -f ../conf/pika.conf ./pika_8001.conf
cp -f ../conf/pika.conf ./pika_8002.conf
cp -f ../conf/pika.conf ./pika_8003.conf
# Create folders for storing data on the primary and secondary nodes
mkdir codis_data_1
mkdir codis_data_2

# Example Change the location for storing data on primary and secondary nodes in the configuration file
sed -i.bak -e 's|databases : 1|databases : 2|' -e 's|port : 9221|port : 8000|' -e 's|log-path : ./log/|log-path : ./codis_data_1/log/|' -e 's|db-path : ./db/|db-path : ./codis_data_1/db/|' -e 's|dump-path : ./dump/|dump-path : ./codis_data_1/dump/|' -e 's|pidfile : ./pika.pid|pidfile : ./codis_data_1/pika.pid|' -e 's|db-sync-path : ./dbsync/|db-sync-path : ./codis_data_1/dbsync/|' -e 's|#daemonize : yes|daemonize : yes|' ./pika_8000.conf
sed -i.bak -e 's|databases : 1|databases : 2|' -e 's|port : 9221|port : 8001|' -e 's|log-path : ./log/|log-path : ./codis_data_2/log/|' -e 's|db-path : ./db/|db-path : ./codis_data_2/db/|' -e 's|dump-path : ./dump/|dump-path : ./codis_data_2/dump/|' -e 's|pidfile : ./pika.pid|pidfile : ./codis_data_2/pika.pid|' -e 's|db-sync-path : ./dbsync/|db-sync-path : ./codis_data_2/dbsync/|' -e 's|#daemonize : yes|daemonize : yes|' ./pika_8001.conf
# Start pika instances with reduced memory footprint
# Add memory_limit parameter to reduce memory usage (256MB per instance)
sed -i.bak -e 's|#max-memory : 0|max-memory : 256|' ./pika_8000.conf
sed -i.bak -e 's|#max-memory : 0|max-memory : 256|' ./pika_8001.conf

# Start instances one by one with time to initialize
./pika -c ./pika_8000.conf
sleep 5
./pika -c ./pika_8001.conf
#ensure both master and slave are ready
sleep 10

# Display memory usage after starting pika instances
free -h || true

cd ../codis
echo "Building codis..."

# Free up memory before building
echo "Clearing caches to free up memory..."
sync
if [ $(id -u) -eq 0 ]; then
    echo 3 > /proc/sys/vm/drop_caches || true
fi

# Limit parallel jobs during make to reduce memory usage
make -j2

echo 'startup codis dashboard and codis proxy'
./admin/codis-dashboard-admin.sh start
./admin/codis-proxy-admin.sh start
./admin/codis-fe-admin.sh start

sleep 20

echo 'assign codis slots to groups and resync groups'
./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --create-group --gid=1
./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --create-group --gid=2

./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --group-add --gid=1 --addr=$CODIS_GROUP_1_MASTER

./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --group-add --gid=2 --addr=$CODIS_GROUP_2_MASTER

./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --slot-action --create-range --beg=0 --end=511 --gid=1
./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --slot-action --create-range --beg=512 --end=1023 --gid=2

echo 'resync all groups'
./bin/codis-admin --dashboard=$CODIS_DASHBOARD_ADDR --resync-group --all

#ensure codis are ready
sleep 10
