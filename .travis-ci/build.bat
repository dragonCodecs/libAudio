call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" %1
meson build --prefix=%HOME%/local -D b_coverage=%2
cd build
ninja
