#!/bin/sh

for i in $(seq 1 100); do
  ../build/release/bin/bench 127.0.0.1:6666,127.0.0.1:6667,127.0.0.1:6668 10 1000 10000
done
