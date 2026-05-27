#!/bin/sh

set -e

cd submission
cmake -B build -DBUILD_WITH_CUDA=OFF -DCMAKE_PREFIX_PATH=./install
cmake --build build -j $(nproc)
