#!/bin/bash

# Kill all pika instances
echo "Stopping all Pika instances..."
pkill -f pika

# Wait for services to completely stop
sleep 2

# Clean up configuration files
echo "Cleaning up configuration files..."
rm -f pika_single.conf
rm -f pika_master.conf
rm -f pika_slave.conf
rm -f pika_rename.conf
rm -f pika_acl_both_password.conf
rm -f pika_acl_only_admin_password.conf
rm -f pika_has_other_acl_user.conf
rm -f pacifica_master.conf
rm -f pacifica_slave1.conf
rm -f pacifica_slave2.conf

# Clean up backup files
echo "Cleaning up backup files..."
rm -f *.conf.bak

# Clean up data directories
echo "Cleaning up data directories..."
rm -rf master_data
rm -rf slave_data
rm -rf rename_data
rm -rf acl1_data
rm -rf acl2_data
rm -rf acl3_data
rm -rf pacifica_test

# Clean up PacificA test directories
echo "Cleaning up PacificA test directories..."
rm -rf pacifica_test/master
rm -rf pacifica_test/slave1
rm -rf pacifica_test/slave2

# Clean up log directories
echo "Cleaning up log files..."
rm -rf */log/*
rm -rf log/*
rm -rf *.log

# Clean up dump files
echo "Cleaning up dump files..."
rm -rf */dump/*
rm -rf dump/*

# Clean up db files
echo "Cleaning up database files..."
rm -rf */db/*
rm -rf db/*

# Clean up pid files
echo "Cleaning up pid files..."
rm -rf *.pid

# Clean up dbsync files
echo "Cleaning up dbsync files..."
rm -rf */dbsync/*
rm -rf dbsync/*

# Verify cleanup
echo "Verifying cleanup..."
if pgrep -f pika > /dev/null; then
    echo "Warning: Some Pika instances are still running"
    pgrep -f pika
else
    echo "All Pika instances have been stopped"
fi

if [ -d "pacifica_test" ] || [ -d "master_data" ] || [ -d "slave_data" ]; then
    echo "Warning: Some data directories still exist"
else
    echo "All data directories have been removed"
fi

echo "Cleanup completed" 