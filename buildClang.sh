#!/bin/bash
rm -rf build-clang
CC=clang-6.0 CXX=clang++-6.0 meson build-clang --prefix=/usr --buildtype=release -D b_lto=true -D b_pch=false -D b_lundef=false
cd build-clang
ninja
