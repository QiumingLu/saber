#!/bin/sh

for i in $(seq 1 10); do
  ../build/release/bin/bench 127.0.0.1:6666,127.0.0.1:6667,127.0.0.1:6668 10 10000 100000
done
