#!/bin/sh

rm -rf build
mkdir -p build
cmake ./cmake/  -B ./build
