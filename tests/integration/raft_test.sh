#!/bin/bash
# Copyright (c) 2023-present, Qihoo, Inc.  All rights reserved.
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

# Script to run Raft consistency tests

set -e

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

echo "=== Running Pika Raft Consistency Tests ==="
echo "Make sure Raft cluster is started with start_raft_cluster.sh first"
echo ""

# Change to raft test directory
cd "$SCRIPT_DIR/raft"

# Update dependencies
go mod tidy

# Run Raft consistency tests
# Note: These tests require a running Raft cluster
# Use start_raft_cluster.sh to set up the cluster first

echo "Running Raft consistency tests..."
go test -v -timeout 30m

echo "=== Raft Consistency Tests Complete ==="