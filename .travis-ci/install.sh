#!/bin/bash -e
export PS4="$ "
set -x

if [ "$TRAVIS_OS_NAME" != "windows" ]; then
	sudo apt-get install libopenal-dev libogg-dev libvorbis-dev \
		libflac-dev libwavpack-dev libmpcdec-dev libfaac-dev libfaad-dev \
		libmad0-dev libid3tag0-dev
else
fi
