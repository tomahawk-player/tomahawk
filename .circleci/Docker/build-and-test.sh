#!/bin/bash

mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Debug} ..
ninja
ninja install
env CTEST_OUTPUT_ON_FAILURE=1 ninja test
