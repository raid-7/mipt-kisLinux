#!/bin/bash

K="./rfs /tmp/fs"

set -v
$K init 200000000

$K mkfile "/file 1"

$K mkdir "/dir 1"

$K ls /

$K ls "/dir 1"

$K ls "/file 1"

