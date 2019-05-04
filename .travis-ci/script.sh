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

if [ "$TRAVIS_OS_NAME" != "windows" ]; then
	CC="$CC_" CXX="$CXX_" meson build --prefix=$HOME/.local -D b_coverage=`codecov`
	cd build
	ninja
else
	unset CC CXX CC_FOR_BUILD CXX_FOR_BUILD
	.travis-ci/build.bat $ARCH `codecov`
	cd build
fi
#if [ "$TRAVIS_OS_NAME" != "windows" ]; then
#	ninja test
#else
#	meson test --no-rebuild --print-errorlogs -v
#fi
ninja install

unset -f codecov
