#!/bin/sh

cmake -S . -B build
cd build
make
cd ..
./build/dogfish
