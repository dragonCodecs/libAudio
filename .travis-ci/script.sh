#!/bin/bash -e
export PS4="$ "
set -x

codecov()
{
	if [ $COVERAGE -ne 0 ]; then
		echo true
	else
		echo false
	fi
}

CC="$CC_" CXX="$CXX_" meson build --prefix=$HOME/.local -D b_coverage=`codecov`
cd build
ninja
#if [ "$TRAVIS_OS_NAME" != "windows" ]; then
#	ninja test
#else
#	meson test --no-rebuild --print-errorlogs -v
#fi
ninja install

unset -f codecov
