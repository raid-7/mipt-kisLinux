#!/usr/bin/env bash

set -x

build/client/sys_dircli add Zuker Zusak 24142 zusak@my.com 46 find Zuker
build/client/ch_dircli -d /dev/directory0 add Zuker Mark 0000 mark@my.com 25 find Zuker
build/client/sys_dircli add Grosp Dovin 565656 dovin@my.com 12
build/client/sys_dircli find Zuker del Zuker
build/client/sys_dircli find Zuker
build/client/ch_dircli -d /dev/directory0 find Grosp del Grosp
