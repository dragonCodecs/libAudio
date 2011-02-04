@echo off
cd libAudioDepends
cp libcuefile/build-win32/{debug,release}/cuefile-static*.lib .
cp libreplaygain/build-win32/{debug,release}/replaygain-static*.lib .
cp musepack*/build-win32/{debug,release}/libmpcdec*.lib .
cp faac*/libfaac/{Debug,Release}/libfaac*.lib .
cp faad2*/libfaad/{Debug,Release}/libfaad*.lib .
cp libogg*/win32/VS2005/Win32/{Debug,Release}/libogg*.{lib,dll} .
cp libvorbis*/win32/VS2005/Win32/{debug,release}/libvorbis*.{lib,dll} .
cp mp4v2*/vstudio8.0/{debug,release}/libmp4v2*.{lib,dll} .
cp OptimFROG_SDK/OptimFROG/OptimFROG.{lib,dll} .
(cd faac2*/common/mp4v2 && cp ../../config.h mpeg4ip_config.h)
@echo on
