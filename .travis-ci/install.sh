#!/bin/bash -e
export PS4="$ "
set -x

if [ "$TRAVIS_OS_NAME" != "windows" ]; then
	sudo apt-get install libopenal-dev libogg-dev libvorbis-dev \
		libflac-dev libmpcdec-dev libfaac-dev libfaad-dev \
		libmad0-dev libid3tag0-dev

	CPU_COUNT=`grep -c '^processor' /proc/cpuinfo`

	pushd deps/mp4v2
	aclocal -I . -I project
	libtoolize -icf
	automake -ac
	autoheader
	autoconf
	CC=$CC_ CXX=$CXX_ ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu --disable-gch
	make -j $CPU_COUNT
	sudo make install
	popd

	pushd deps/WavPack
	aclocal -I .
	libtoolize -icf
	automake -ac
	autoconf
	CC=$CC_ CXX=$CXX_ ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
	make -j $CPU_COUNT
	sudo make install
	popd
else
	echo "chocolatey"
	ls /c/ProgramData/chocolatey
	echo "lib"
	ls /c/ProgramData/chocolatey/lib
	echo "mingw"
	ls /c/ProgramData/chocolatey/lib/mingw
	echo "tools"
	ls /c/ProgramData/chocolatey/lib/mingw/tools
	echo "install"
	ls /c/ProgramData/chocolatey/lib/mingw/tools/install
	echo "mingw64"
	ls /c/ProgramData/chocolatey/lib/mingw/tools/install/mingw64

	pushd deps/mp4v2
	aclocal -I . -I project
	libtoolize -icf
	automake -ac
	autoheader
	autoconf
	popd

	pushd deps/WavPack
	aclocal -I .
	libtoolize -icf
	automake -ac
	autoconf
	popd
fi
