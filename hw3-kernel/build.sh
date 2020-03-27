#!/usr/bin/env bash

rm -r build/kernel &> /dev/null || true
cp -r kernel build/kernel

cd build/kernel && make $@
cd ..

cmake .. && cmake --build .
