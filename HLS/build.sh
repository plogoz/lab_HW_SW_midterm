#!/bin/bash
echo "Building Conv2D testbench with clang++..."
clang++ -std=c++11 -O0 \
  conv2d.cpp \
  conv2DTestbench.cpp \
  -o conv2d_test
echo "Build successful! Binary: ./conv2d_test"
./conv2d_test
