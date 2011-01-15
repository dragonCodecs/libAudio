OptimFROG Lossless/DualStream/IEEE Float Audio Compressor,
   version 4.510 [2005.07.17] (pre-release)
Copyright (C) 1996-2005 Florin Ghido, all rights reserved.
Visit http://www.LosslessAudio.org for updates

This is a pre-release version and most of the
documentation is not yet updated or not completely
updated, but just included here for your reference.


What is new in this version:
  - encoding is now 6% faster with exactly the same
    compression
  - all the source code was reorganized and rewritten
    to use uniform standards and increase portability
  - OFR, OFS, OFF and the DLL/SO library were merged
    into a single, common code base
  - significantly reduced system dependent code areas
  - all programs are now Valgrind clean
  - fixed small problems (like not setting the file
    date for the correction file)

  - the OptimFROG library is now available for Linux
    as a SO library
  - the OptimFROG.dll version 1.200 is binary
    compatible with the previous versions
  - added new function OptimFROG_freeTags to release
    the memory allocated for the tags structure
  - new C++ wrapper interface for the OptimFROG
    library requiring less work for writing plug-ins
  - two C usage examples and two C++ usage examples
    of the library for Windows/Linux

  - complete source code for dBpowerAMP, Winamp 5,
    foobar2000, and XMMS plug-ins

  - site address has changed to (the other are still
    usable) http://www.LosslessAudio.org

What to expect next:
  - development of OptimFROG will greatly accelerate
  - next major version will most probably include:
    improved compression, creation of self extracting
    archives, ID3v2 tags, Darwin/MacOS port, recovery
    information, gapless joining of multiple OptimFROG
    files, recovery of data from severely broken files

What is needed (what you can contribute):
  - it would be great to have extended support (more
    plug-ins for other players, sound editing tools)


If you have any questions, comments, suggestions or problems
regarding OptimFROG please don't hesitate to contact me at:

  FlorinGhido@yahoo.com

You can always find the newest version of OptimFROG at:

  http://www.LosslessAudio.org

Copyright (C) 1996-2005 Florin Ghido, all rights reserved.
