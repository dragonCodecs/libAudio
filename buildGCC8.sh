#!/bin/bash
rm -rf build-gcc8
CC=gcc-8 CXX=g++-8 meson build-gcc8 --prefix=/usr --buildtype=release -D b_lto=true -D b_pch=false -D b_lundef=false
cd build-gcc8
ninja
