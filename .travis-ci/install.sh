#!/bin/bash -e
export PS4="$ "
set -x

if [ "$TRAVIS_OS_NAME" != "windows" ]; then
	sudo apt-get install libopenal-dev libogg-dev libvorbis-dev \
		libflac-dev libwavpack-dev libmpcdec-dev libfaac-dev libfaad-dev \
		libmad0-dev libid3tag0-dev

	pushd deps/mp4v2
	aclocal -I .
	libtoolize -icf
	automake -ac
	autoheader
	autoconf
	CC=$CC_ CXX=$CXX_ ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu --disable-gch
	make
	sudo make install
	popd
#else
fi
