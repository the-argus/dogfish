#!/bin/sh

set -e

cmake -S . -B build
cd build
make
cd ..
./build/dogfish
