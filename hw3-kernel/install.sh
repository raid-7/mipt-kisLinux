#!/usr/bin/env bash

MOD_NAME=directory_kmodule
DEV_NAME=directory0

rmmod $MOD_NAME &> /dev/null || true
insmod build/kernel/$MOD_NAME.ko

major=$(awk '$2=="'$MOD_NAME'" { print $1 }' /proc/devices)

rm -f /dev/$DEV_NAME
mknod /dev/$DEV_NAME c $major 0
