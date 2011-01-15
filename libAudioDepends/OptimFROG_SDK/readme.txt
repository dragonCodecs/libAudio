OptimFROG Lossless/DualStream/IEEE Float Audio Compressor,
   version 4.520b [2006.03.02] (beta)
Copyright (C) 1996-2006 Florin Ghido, all rights reserved.
Visit http://www.LosslessAudio.org for updates

This is a beta version and most of the
documentation is not yet updated or not completely
updated, but just included here for your reference.


What is new in this version (4.520b):
  - successfully ported to Linux/amd64
  - successfully ported to Darwin/ppc (PowerPC G3)
  - many internal source code improvements
  - all the newly created compressed files are
    completely backwards compatible (can be
    decoded with previous 4.50x versions)
  - added ID3v2 tag support (all decoders can
    search for main header, skipping up to 1MB)

  - added --selfextract option, the Win32/x86 sfx
    stub (statically linked) is only 55 kB in size
  - complete self extracting support for Win32/x86,
    Linux/x86, and Linux/amd64 (all sfx stubs are
    statically linked)
  - C source code for sfx stub available in SDK

  - slightly faster encoding and decoding with
    exactly the same compression
  - compression improved very slightly when using
    --optimize best option
  - improved compression (0.1-0.3%) when using
    command --maximumcompression (this option
    is mainly intended for benchmark)

  - added --correct_audition option to IEEE Float
    to correct Adobe Audition / CoolEdit conversion
    bug: when converting from int to float, Audition
    converts 0 values to random noise with maximum
    relative amplitude < 5*10^-16; this option
    carefully corrects this bug setting them back
    to 0 and significantly improves compression
    ratio in these files

  - the OptimFROG.dll/.so version 1.210 is binary
    compatible with the previous versions, now also
    available for Linux/amd64 and Darwin/ppc
  - fixed a missing check for errors after calling
    OptimFROG_readTail(...), which could return -1,
    in Test_C and Test_CPP SDK samples


What to expect next:
  - development of OptimFROG will greatly accelerate
  - next major version will most probably include:
    * significantly improved compression (soon)
    * recovery information
    * gapless joining of multiple OptimFROG files
    * recovery of data from severely broken files
    * using .ofc correction file when decoding in
      plug-ins and an additional function for
      specifying the .ofc correction file in SDK
    * multichannel support
    * cue sheet support
    * Adobe Audition plug-in
    * integrated GUI interface and automated
      installer with plug-ins
    * something completely different and new ;-)


What is needed (what you can contribute):
  - it would be great to have extended support (more
    plug-ins for other players, sound editing tools)


If you have any questions, comments, suggestions or problems
regarding OptimFROG please don't hesitate to contact me at:

  FlorinGhido@yahoo.com

You can always find the newest version of OptimFROG at:

  http://www.LosslessAudio.org

Copyright (C) 1996-2006 Florin Ghido, all rights reserved.
