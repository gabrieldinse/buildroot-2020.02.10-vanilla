#!/bin/bash

make -C $BASE_DIR/../custom-scripts/hello_world
cp $BASE_DIR/../custom-scripts/hello_world/hello_world $BASE_DIR/target/usr/bin

make -C $BASE_DIR/../custom-scripts/jitter_and_latency
cp $BASE_DIR/../custom-scripts/jitter_and_latency/jitter_and_latency $BASE_DIR/target/usr/bin

sed -i "s/#PermitRootLogin prohibit-password/PermitRootLogin yes/g" $BASE_DIR/target/etc/ssh/sshd_config
