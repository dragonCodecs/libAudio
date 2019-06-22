#!/bin/bash -e
export PS4="$ "
set -x

if [ "$TRAVIS_OS_NAME" != "windows" ]; then
	wget https://bootstrap.pypa.io/get-pip.py
	wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
	python3.6 get-pip.py --user
	pip3 install --user meson
	unzip ninja-linux.zip -d ~/.local/bin
	rm get-pip.py ninja-linux.zip
	sudo apt-get update
else
	wget --progress=dot:mega https://bootstrap.pypa.io/get-pip.py
	wget --progress=dot:mega https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-win.zip
	choco install -r --no-progress msys2 --params="'/NoUpdate'"
	choco install -r --no-progress make
	choco install -r --no-progress python --version 3.6.8
	python get-pip.py
	pip3 install meson
	mkdir /c/tools/ninja-build
	7z x -oC:\\tools\\ninja-build ninja-win.zip
	rm get-pip.py ninja-win.zip

	powershell -executionpolicy bypass "pacman -Sy --noconfirm"
	powershell -executionpolicy bypass "pacman -S --noconfirm m4 autoconf autoconf-archive automake-wrapper libtool make pkg-config"

	ln -sv /c/tools/msys64/usr/share/{autoconf,automake,aclocal,libtool,pkgconfig}* /usr/share
	ln -sv /c/tools/msys64/usr/bin/{autom4te,autoconf,automake,autoheader,aclocal,libtool{,ize},m4} /usr/bin
fi
