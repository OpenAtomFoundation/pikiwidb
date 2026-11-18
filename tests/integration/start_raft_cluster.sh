#!/bin/bash
# This script is used to start a 3-node Pika Raft cluster for consistency testing
# It is used by .github/workflows/pika.yml

set -e

echo "=== Starting Pika Raft Cluster for Consistency Testing ==="

# Get script directory
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
cd "$SCRIPT_DIR"

# Determine sed command based on OS (macOS sed requires '' after -i)
if [[ "$OSTYPE" == "darwin"* ]]; then
    SED_INPLACE="sed -i ''"
else
    SED_INPLACE="sed -i"
fi

# Clean up any previous raft data
rm -rf raft_node1_data raft_node2_data raft_node3_data
rm -f pika_raft_node*.conf pika_raft_node*.conf.bak pika_raft_node*.conf.tmp

# Create data directories for each node
mkdir -p raft_node1_data
mkdir -p raft_node2_data
mkdir -p raft_node3_data

# Function to configure a node using simple text replacement
configure_node() {
    local node_num=$1
    local port=$2
    local conf_file="./pika_raft_node${node_num}.conf"
    local data_dir="./raft_node${node_num}_data"
    
    echo "Configuring node $node_num (port $port)..."
    
    # Copy fresh from original - use project root conf, not tests/conf
    cp "$SCRIPT_DIR/../../conf/pika.conf" "$conf_file"
    
    # Apply substitutions one by one using awk for reliability
    awk -v port="$port" -v data_dir="$data_dir" '
    {
        # Port configuration
        gsub(/port : 9221/, "port : " port)
        # Path configurations
        gsub(/log-path : \.\/log\//, "log-path : " data_dir "/log/")
        gsub(/db-path : \.\/db\//, "db-path : " data_dir "/db/")
        gsub(/dump-path : \.\/dump\//, "dump-path : " data_dir "/dump/")
        gsub(/pidfile : \.\/pika\.pid/, "pidfile : " data_dir "/pika.pid")
        gsub(/db-sync-path : \.\/dbsync\//, "db-sync-path : " data_dir "/dbsync/")
        # Daemon and timeout
        gsub(/#daemonize : yes/, "daemonize : yes")
        gsub(/daemonize : no/, "daemonize : yes")
        gsub(/timeout : 60/, "timeout : 500")
        # Raft configurations
        gsub(/raft-enabled : no/, "raft-enabled : yes")
        gsub(/raft-group-id : pika_raft_group/, "raft-group-id : pika_test_cluster")
        print
    }
    ' "$conf_file" > "${conf_file}.tmp" && mv "${conf_file}.tmp" "$conf_file"
    
    # Verify configurations
    echo "  port: $(grep '^port :' "$conf_file" | head -1)"
    if grep -q "raft-enabled : yes" "$conf_file"; then
        echo "  raft-enabled: YES"
    else
        echo "  raft-enabled: NO (WARNING: Raft may not be supported)"
        grep "raft-enabled" "$conf_file" || echo "  (raft-enabled line not found)"
    fi
}

# Configure all nodes
configure_node 1 9321
configure_node 2 9322
configure_node 3 9323

echo "Starting Raft Node 1 on port 9321..."
./pika -c ./pika_raft_node1.conf

echo "Starting Raft Node 2 on port 9322..."
./pika -c ./pika_raft_node2.conf

echo "Starting Raft Node 3 on port 9323..."
./pika -c ./pika_raft_node3.conf

# Wait for nodes to start
echo "Waiting for nodes to start..."
sleep 10

# Check if all nodes are running
echo "Checking node status..."

check_node() {
    local port=$1
    local name=$2
    if redis-cli -p $port PING > /dev/null 2>&1; then
        echo "  $name (port $port): OK"
        return 0
    else
        echo "  $name (port $port): FAILED"
        return 1
    fi
}

all_ok=true
check_node 9321 "Node1" || all_ok=false
check_node 9322 "Node2" || all_ok=false
check_node 9323 "Node3" || all_ok=false

if [ "$all_ok" = false ]; then
    echo "ERROR: Not all nodes started successfully"
    exit 1
fi

# Initialize Raft cluster
echo ""
echo "Initializing Raft cluster..."

# The Raft ports are Redis port + 3000
# Node1: 9321 -> 12321
# Node2: 9322 -> 12322
# Node3: 9323 -> 12323

RAFT_PEERS="127.0.0.1:12321,127.0.0.1:12322,127.0.0.1:12323"

echo "Initializing cluster with peers: $RAFT_PEERS"

# Initialize Raft on all nodes - each node needs to be initialized
echo "Initializing Node 1..."
redis-cli -p 9321 RAFT.CLUSTER INIT "$RAFT_PEERS"
echo "Initializing Node 2..."
redis-cli -p 9322 RAFT.CLUSTER INIT "$RAFT_PEERS"
echo "Initializing Node 3..."
redis-cli -p 9323 RAFT.CLUSTER INIT "$RAFT_PEERS"

# Wait for leader election
echo "Waiting for leader election..."
sleep 5

# Check cluster status
echo ""
echo "=== Cluster Status ==="
echo "Node 1:"
redis-cli -p 9321 RAFT.CLUSTER INFO 2>/dev/null || echo "  (unable to get status)"
echo ""
echo "Node 2:"
redis-cli -p 9322 RAFT.CLUSTER INFO 2>/dev/null || echo "  (unable to get status)"
echo ""
echo "Node 3:"
redis-cli -p 9323 RAFT.CLUSTER INFO 2>/dev/null || echo "  (unable to get status)"

echo ""
echo "=== Raft cluster setup complete ==="
echo "Nodes are available at:"
echo "  - Node 1: 127.0.0.1:9321"
echo "  - Node 2: 127.0.0.1:9322"
echo "  - Node 3: 127.0.0.1:9323"