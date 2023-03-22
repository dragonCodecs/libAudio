# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2018-2023 Rachel Mant <git@dragonmux.network>

# Required dependencies
threading = dependency('threads')
libOpenAL = dependency('openal', required: target_machine.system() != 'windows')

if not libOpenAL.found()
	cmake = import('cmake')

	openALOptions = cmake.subproject_options()
	# OpenAL can NOT be built with c++17 -- see almalloc.cpp
	openALOptions.set_override_option('cpp_std', 'c++14')
	# Disable Git because Meson somehow gets confused with the
	# custom version generator
	openALOptions.add_cmake_defines({
		'CMAKE_BUILD_TYPE': 'RelWithDebInfo',
		'ALSOFT_UTILS': false,
		'ALSOFT_NO_CONFIG_UTIL': true,
		'ALSOFT_EXAMPLES': false,
		'ALSOFT_CONFIG': false,
		'ALSOFT_HRTF_DEFS': false,
		'ALSOFT_AMBDEC_PRESETS': false,
		'ALSOFT_BACKEND_ALSA': false,
		'ALSOFT_BACKEND_OSS': false,
		'ALSOFT_BACKEND_SOLARIS': false,
		'ALSOFT_BACKEND_SNDIO': false,
		'ALSOFT_BACKEND_PORTAUDIO': false,
		'ALSOFT_BACKEND_PULSEAUDIO': false,
		'ALSOFT_BACKEND_COREAUDIO': false,
		'ALSOFT_BACKEND_JACK': false,
		'ALSOFT_BACKEND_OPENSL': false,
		'ALSOFT_BACKEND_WAVE': true,
		'ALSOFT_BACKEND_WINMM': false,
		'ALSOFT_BACKEND_DSOUND': false,
		'ALSOFT_REQUIRE_WASAPI': true,
		'ALSOFT_CPUEXT_NEON': false,
		'CMAKE_DISABLE_FIND_PACKAGE_WindowsSDK': true,
		'CMAKE_DISABLE_FIND_PACKAGE_Git': true,
	})

	libOpenAL = cmake.subproject('openal-soft', options: openALOptions, required: true).dependency('OpenAL')
endif

maths = cxx.find_library('m', required: false)

requiredDependencies = [threading, libOpenAL, maths, substrate]
confData = configuration_data()

# Optional dependencies, governed by extra_formats
extraFormats = get_option('extra_formats')

requiresVorbis = extraFormats.contains('vorbis')
libOgg = dependency('ogg', required: requiresVorbis, allow_fallback: true, disabler: true)
libVorbis = dependency('vorbis', required: requiresVorbis, allow_fallback: true, disabler: true)
libVorbisFile = dependency('vorbisfile', required: requiresVorbis, allow_fallback: true, disabler: true)
libVorbisEnc = dependency('vorbisenc', required: requiresVorbis, allow_fallback: true, disabler: true)

requiresOpus = extraFormats.contains('opus')
libOpus = dependency('opus', required: requiresOpus, allow_fallback: true, disabler: true)
libOpusFile = dependency('opusfile', required: requiresOpus, allow_fallback: true, disabler: true)
libOpusEnc = dependency('libopusenc', required: requiresOpus, allow_fallback: true, disabler: true)

requiresFlac = extraFormats.contains('flac')
libFLAC = dependency('flac', required: requiresFlac, allow_fallback: true, disabler: true)

requiresMusepack = extraFormats.contains('musepack')
libMPC = cxx.find_library('mpcdec', required: false, disabler: true)
if not libMPC.found()
	libMPC = dependency('mpcdec', required: requiresMusepack, allow_fallback: true, disabler: true)
endif

requiresWavpack = extraFormats.contains('wavpack')
libWavPack = dependency('wavpack', version: '>=5.0.0', required: requiresWavpack, allow_fallback: true, disabler: true)
if not cxx.has_function('WavpackOpenFileInputEx64', dependencies: libWavPack)
	error('Could not find the required function WavpackOpenFileInputEx64() from libwavpack')
endif

requiresMP3 = extraFormats.contains('mp3')
libID3Tag = dependency('id3tag', required: requiresMP3, allow_fallback: true, disabler: true)
libMAD = cxx.find_library('mad', required: requiresMP3, disabler: true)
libLAME = dependency('mp3lame', required: requiresMP3, allow_fallback: true)
if not libLAME.found()
	libLAME = cxx.find_library('mp3lame', required: false)
endif

requiresAAC = extraFormats.contains('aac')
libMP4v2 = cxx.find_library('mp4v2', required: requiresAAC, disabler: true)
libFAAC = cxx.find_library('faac', required: requiresAAC, disabler: true)
libFAAD = cxx.find_library('faad', required: requiresAAC, disabler: true)
if not cxx.has_function('MP4CreateProvider', dependencies: libMP4v2)
	error('Could not find the required function MP4CreateProvider() from libmp4v2')
endif

extraSrcs = []
extraDeps = []
includes = [include_directories('.')]

if libOgg.found() and libOpus.found() and libOpusFile.found() and libOpusEnc.found()
	message('Enabling support for Opus')
	extraSrcs += ['loadOggOpus.cxx', 'saveOggOpus.cxx',]
	confData.set10('ENABLE_OPUS', true)
	extraDeps += [libOpus, libOpusFile, libOpusEnc,]
endif

if libOgg.found() and libVorbis.found() and libVorbisFile.found() and libVorbisEnc.found()
	message('Enabling support for Vorbis')
	extraSrcs += ['loadOggVorbis.cpp', 'saveOggVorbis.cpp',]
	confData.set10('ENABLE_VORBIS', true)
	extraDeps += [libVorbis, libVorbisFile, libVorbisEnc,]
endif

if libOgg.found() and libFLAC.found()
	message('Enabling support for FLAC')
	extraSrcs += ['loadFLAC.cpp', 'saveFLAC.cpp',]
	confData.set10('ENABLE_FLAC', true)
	extraDeps += libFLAC
endif

if confData.has('ENABLE_VORBIS') or confData.has('ENABLE_OPUS') or confData.has('ENABLE_FLAC')
	message('Enabling support for OGG container')
	extraSrcs += ['oggCommon.cxx']
	extraDeps += libOgg
endif

if libMPC.found()
	message('Enabling support for Musepack')
	extraSrcs += ['loadMPC.cpp',]
	confData.set10('ENABLE_MUSEPACK', true)
	extraDeps += libMPC
endif

if libWavPack.found()
	message('Enabling support for WavPack')
	extraSrcs += ['loadWavPack.cpp',]
	confData.set10('ENABLE_WAVPACK', true)
	extraDeps += libWavPack
endif

if libID3Tag.found() and libMAD.found() and libLAME.found()
	message('Enabling support for MP3')
	extraSrcs += ['loadMP3.cxx', 'saveMP3.cxx',]
	confData.set10('ENABLE_MP3', true)
	extraDeps += [libID3Tag, libMAD, libLAME,]
endif

# TODO: decouple M4A from the AAC encoder
if libMP4v2.found() and libFAAC.found() and libFAAD.found()
	message('Enabling support for AAC')
	extraSrcs += ['loadM4A.cpp', 'loadAAC.cpp', 'saveM4A.cpp',]
	confData.set10('ENABLE_M4A', true)
	confData.set10('ENABLE_AAC', true)
	extraDeps += [libMP4v2, libFAAC, libFAAD,]
endif

if extraFormats.contains('wma')
	message('Enabling support for Windows Media Audio')
	extraSrcs += 'loadWMA.cpp'
	confData.set10('ENABLE_WMA', true)
endif
if extraFormats.contains('optimFROG')
	message('Enabling support for OptimFROG')
	extraSrcs += 'loadOptimFROG.cpp'
	confData.set10('ENABLE_OptimFROG', true)
	extraDeps += dependency('optimfrog')
endif
if extraFormats.contains('sndh')
	message('Enabling support for SNDH')
	extraSrcs += ['loadSNDH.cpp', 'sndh/loader.cxx', 'sndh/iceDecrunch.cxx']
	confData.set10('ENABLE_SNDH', true)
endif
if extraFormats.contains('sid')
	message('Enabling support for SID')
	extraSrcs += 'loadSID.cpp'
	confData.set10('ENABLE_SID', true)
endif
if extraFormats.contains('aon')
	message('Enabling support for AON')
	extraSrcs += 'loadAON.cpp'
	confData.set10('ENABLE_AON', true)
endif
if extraFormats.contains('fc1x')
	message('Enabling support for FC1x')
	extraSrcs += 'loadFC1x.cpp'
	confData.set10('ENABLE_FC1x', true)
endif
if extraFormats.contains('ra')
	message('Enabling support for RealAudio')
	extraSrcs += 'loadRealAudio.cpp'
	confData.set10('ENABLE_RA', true)
endif

if target_machine.system() == 'windows'
	includes += include_directories('windows')
endif

genericModuleSrcs = [
	'genericModule/ModuleFile.cpp',
	'genericModule/ModuleHeader.cpp',
	'genericModule/ModuleInstrument.cpp',
	'genericModule/ModuleSample.cpp',
	'genericModule/ModulePattern.cpp',
	'genericModule/ModuleEffects.cpp',
	'moduleMixer/moduleMixer.cpp',
	'moduleMixer/channel.cxx',
	'loadMOD.cpp',
	'loadS3M.cpp',
	'loadSTM.cpp',
	'loadIT.cpp'
]

libAudioSrcs = [
	'loadAudio.cpp', 
	'loadWAV.cpp',
	'fixedPoint/fixedPoint.cpp',
	'saveAudio.cpp',
	'openAL.cxx',
	'openALPlayback.cxx',
	'playback.cxx',
	'console.cxx',
]

libAudioConfigHeader = configure_file(
	output : 'libAudioConfig.h',
	configuration : confData,
)

libAudioLibrary = shared_library(
	'libAudio',
	genericModuleSrcs + libAudioSrcs + extraSrcs,
	dependencies: requiredDependencies + extraDeps,
	cpp_args: ['-DHAVE_STDINT_H', '-DlibAUDIO'],
	include_directories: includes,
	gnu_symbol_visibility: 'inlineshidden',
	name_prefix: '',
	install: true,
	version: meson.project_version()
)
libAudioInclude = meson.current_source_dir()

libAudio = declare_dependency(
	include_directories: include_directories('.'),
	link_with: libAudioLibrary,
	version: meson.project_version()
)
