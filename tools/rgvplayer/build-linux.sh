#!/bin/bash
# Build RGVPlayer for Linux

mkdir -p build-linux
cd build-linux
cmake ..
make
