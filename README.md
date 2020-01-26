
libAudio
========

![C++ CI](https://github.com/DX-MON/libAudio/workflows/C++%20CI/badge.svg)

## Getting started

After cloning the repository, you will need to run `git submodule init --update` to pull all dependencies for a build.

libAudio is built using Meson + Ninja and is designed and tested for Meson 0.48+.
For general use, this means running
```
meson build
cd build
ninja
```

At this point, you will have a few binaries in the build tree if you have all the relevant dependencies installed:
* In `libAudio`, you will have the library shared object and link files
* In `player`, you will have a simple CLI audio player
* In `transcoder`, you will have an audio transcoder built on the back of the library

Installing the library is as simple as `ninja install` and providing an appropriate password at the privilege elevation prompt if installing to a system location.

## Using the player

The player is designed to take a list of files to play in sequence, and work its way through them. For example, if you have a directory of MP3 files, `libAudio *.mp3` will play each file in turn.

## Using the transcoder

The transcoder is designed to operate over input-output file pairs, with each pair being transcoded to the same output file format. The library will copy metadata over if the output format supports it, and try its best to preseve audio quality by default.

An example invocation is as follows: `libAudioTranscode ogg input.flac output.ogg`

## Documentation

This project is documented in-source using Doxygen, and configurations for Doxygen can be found in the `docs` tree.
To build the documentation, simply run `doxygen libAudio.conf` in the docs tree.
