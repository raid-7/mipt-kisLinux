#!/usr/bin/env bash


MOD_NAME=keystats_kmodule
USR=`whoami`

run_remote() {
    cmd="$@"
    ssh -p 2222 $USR@localhost 'sh -c "'"$cmd"'"'
}

sudo_remote() {
    cmd="$@"
    ssh -p 2222 $USR@localhost 'sudo sh -c "'"$cmd"'"'
}

run_remote rm -rf /tmp/kernel
scp -P 2222 -r kernel/ $USR@localhost:/tmp
run_remote 'cd /tmp/kernel && make'
sudo_remote "rmmod $MOD_NAME &> /dev/null || true"
sudo_remote insmod /tmp/kernel/$MOD_NAME.ko
