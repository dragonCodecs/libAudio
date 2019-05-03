#!/bin/bash -e
export PS4="$ "
set -x

cd build
#if [ "$TRAVIS_OS_NAME" != "windows" ]; then
#	ninja test
#else
#	meson test --no-rebuild --print-errorlogs -v
#fi
ninja install
