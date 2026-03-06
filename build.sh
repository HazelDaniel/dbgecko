#!/bin/sh

rm -rf build
mkdir -p build
cmake ./cmake/  -B ./build -DCMAKE_C_FLAGS="-DDEBUG_MODE"
