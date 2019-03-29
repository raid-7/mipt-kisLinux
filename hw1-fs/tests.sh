#!/bin/bash

make clean
make

LONG_STR_LEN=70000
LONG_STR=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $LONG_STR_LEN | head -n 1)
K="./rfs /tmp/fs"

if [[ ! -e ./rfs ]]; then
	echo "Build failed"
	exit 1
fi
echo "Tests"
echo ""

set -v
$K init 200000000

$K mkfile "/file 1"

$K mkdir "/dir 1"

$K ls /

$K ls "/dir 1"

$K ls "/file 1"

echo -n $LONG_STR | $K write "/file 1"

$K stat "/file 1"

$K cat "/file 1" | wc

$K mkfile "/dir 1/file"

echo -n "$($K cat /file\ 1)" | $K write "/dir 1/file"

$K stat "/dir 1/file"

echo -n "$($K cat /file\ 1)" | $K write "/dir 1/file" $(( LONG_STR_LEN / 2 ))

[ "$($K cat "/dir 1/file")" == "${LONG_STR:0:$(( LONG_STR_LEN / 2 ))}$LONG_STR" ]
echo $?
