# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2018-2023 Rachel Mant <git@dragonmux.network>

# Required dependencies
threading = dependency('threads')
if host_machine.system() == 'darwin'
	if cxx.get_id() == 'clang'
		libOpenAL = dependency('openal', method: 'extraframework', required: true)
	else
		# GCC cannot find frameworks properly
		# https://github.com/mesonbuild/meson/issues/12160
		libOpenALDep = dependency('openal', method: 'cmake', required: true, modules: ['OpenAL::OpenAL'])
		# OpenAL-soft is also broken on Homebrew GCC, as it never enables OBJCXX for the Core Audio backend
		# The easy fix is to reuse CMake's work and prod GCC a bit
		libOpenAL = declare_dependency(
			# not links or link_args as Meson thinks the framework folder /is/ the library
			dependencies: libOpenALDep.partial_dependency(includes: true,  compile_args: true),
			# this is a system framework so no -F flag necessary
			compile_args: ['-framework', 'OpenAL'],
			link_args: ['-framework', 'OpenAL'],
		)
		assert(cxx.has_header('OpenAL.h', dependencies: libOpenAL))
	endif
else
	libOpenAL = dependency('openal', required: host_machine.system() != 'windows')
endif

if not libOpenAL.found()
	cmake = import('cmake')

	# CMake and Meson don't have a common minimum OSX deployment version
	if host_machine.system() == 'darwin'
		minOSXVersion = cxx.get_define('__MAC_OS_X_VERSION_MIN_REQUIRED', prefix: '#include <Availability.h>')
		if minOSXVersion != ''
			minOSXVersion = run_command(python, '-c', 'from sys import argv; x = argv[1]; s = "0"; min = x[2:4].lstrip(s); min = min if min else s; print(f"{x[0:2]}.{min}")', minOSXVersion, check: true).stdout().strip()
		endif
	else
		minOSXVersion = ''
	endif

	openALOptions = cmake.subproject_options()
	# OpenAL can NOT be built with c++17 -- see almalloc.cpp
	openALOptions.set_override_option('cpp_std', 'c++14')
	openALOptions.set_override_option('strip', 'false')
	# Disable Git because Meson somehow gets confused with the
	# custom version generator
	openALOptions.add_cmake_defines({
		'CMAKE_BUILD_TYPE': 'RelWithDebInfo',
		'ALSOFT_UTILS': false,
		'ALSOFT_NO_CONFIG_UTIL': true,
		'ALSOFT_EXAMPLES': false,
		'ALSOFT_INSTALL_CONFIG': false,
		'ALSOFT_INSTALL_HRTF_DATA': false,
		'ALSOFT_INSTALL_AMBDEC_PRESETS': false,
		'ALSOFT_BACKEND_ALSA': host_machine.system() == 'linux',
		'ALSOFT_BACKEND_OSS': false,
		'ALSOFT_BACKEND_SOLARIS': host_machine.system() == 'sunos',
		'ALSOFT_BACKEND_SNDIO': false,
		'ALSOFT_BACKEND_PORTAUDIO': false,
		'ALSOFT_BACKEND_PULSEAUDIO': host_machine.system() == 'linux',
		'ALSOFT_BACKEND_COREAUDIO': host_machine.system() == 'darwin',
		'ALSOFT_BACKEND_JACK': false,
		'ALSOFT_BACKEND_OPENSL': false,
		'ALSOFT_BACKEND_WAVE': true,
		'ALSOFT_BACKEND_WINMM': false,
		'ALSOFT_BACKEND_DSOUND': ['windows', 'cygwin'].contains(host_machine.system()),
		'ALSOFT_BACKEND_WASAPI': false,
		'ALSOFT_CPUEXT_NEON': false,
		'CMAKE_DISABLE_FIND_PACKAGE_WindowsSDK': true,
		'CMAKE_DISABLE_FIND_PACKAGE_Git': true,
		'CMAKE_OSX_DEPLOYMENT_TARGET': minOSXVersion,
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
libOpusFile = dependency('opusfile', required: requiresOpus, allow_fallback: true)
libOpusEnc = dependency('libopusenc', required: requiresOpus, allow_fallback: true)

requiresFlac = extraFormats.contains('flac')
libFLAC = dependency('flac', required: requiresFlac, allow_fallback: true, disabler: true)

requiresMusepack = extraFormats.contains('musepack')
libMPC = cxx.find_library('mpcdec', required: false, disabler: true)
if not libMPC.found()
	libMPC = dependency('mpcdec', required: requiresMusepack, allow_fallback: true, disabler: true)
endif

requiresWavpack = extraFormats.contains('wavpack')
libWavPack = dependency('wavpack', version: '>=5.0.0', required: requiresWavpack, allow_fallback: true)

if libWavPack.found() and libWavPack.type_name() != 'internal'
	cxx.has_header_symbol('wavpack/wavpack.h', 'WavpackOpenFileInputEx64', dependencies: libWavPack, required: true)
endif

requiresMP3 = extraFormats.contains('mp3')
libID3Tag = dependency('id3tag', required: requiresMP3, allow_fallback: true, disabler: true)
libMAD = cxx.find_library('mad', required: false)
if not libMAD.found()
	libMAD = dependency('mad', version: '>=0.16.3', required: requiresMP3, allow_fallback: true)
endif
libLAME = cxx.find_library('mp3lame', required: false)
if not libLAME.found()
	libLAME = dependency('mp3lame', version: '>= 3.100', required: requiresMP3, allow_fallback: true)
endif

requiresAAC = extraFormats.contains('aac')
libMP4v2 = cxx.find_library('mp4v2', required: false)
if not libMP4v2.found()
	libMP4v2 = dependency('mp4v2', version: '>=2.1.2', required: requiresAAC, allow_fallback: true)
endif
libFAAC = cxx.find_library('faac', required: false)
if not libFAAC.found()
	libFAAC = dependency('faac', version: '>= 1.30', required: requiresAAC, allow_fallback: true)
endif
libFAAD = cxx.find_library('faad', required: false)
if not libFAAD.found()
	libFAAD = dependency('faad2', version: '>= 1.30', required: requiresAAC, allow_fallback: true)
endif

if libMP4v2.found() and libMP4v2.type_name() != 'internal'
	cxx.has_header_symbol('mp4v2/mp4v2.h', 'MP4CreateProvider', dependencies: libMP4v2, required: true)
endif

requiresOptimFROG = extraFormats.contains('optimfrog')
optimFROGFallback = []
if target_machine.system() == 'linux' and target_machine.cpu_family() == 'x86_64'
	optimFROGFallback += ['optimfrog-linux-x64']
elif target_machine.system() == 'darwin' and target_machine.cpu_family() == 'x86_64'
	optimFROGFallback += ['optimfrog-macos-x64']
elif target_machine.system() == 'windows' and target_machine.cpu_family() == 'x86_64' and cxx.get_argument_syntax() == 'msvc'
	optimFROGFallback += ['optimfrog-win-x64']
endif

libOptimFROG = dependency('optimfrog', required: requiresOptimFROG, fallback: optimFROGFallback)

extraSrcs = []
extraDeps = []
includes = [include_directories('.')]
formats = {}

formats += {'Opus': libOgg.found() and libOpusFile.found() and libOpusEnc.found()}
if formats['Opus']
	message('Enabling support for Opus')
	extraSrcs += ['loadOggOpus.cxx', 'saveOggOpus.cxx',]
	confData.set10('ENABLE_OPUS', true)
	extraDeps += [libOpusFile, libOpusEnc,]
endif

formats += {'Vorbis': libOgg.found() and libVorbis.found() and libVorbisFile.found() and libVorbisEnc.found()}
if formats['Vorbis']
	message('Enabling support for Vorbis')
	extraSrcs += ['loadOggVorbis.cpp', 'saveOggVorbis.cpp',]
	confData.set10('ENABLE_VORBIS', true)
	extraDeps += [libVorbis, libVorbisFile, libVorbisEnc,]
endif

formats += {'FLAC': libOgg.found() and libFLAC.found()}
if formats['FLAC']
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

formats += {'Musepack': libMPC.found()}
if formats['Musepack']
	message('Enabling support for Musepack')
	extraSrcs += ['loadMPC.cpp',]
	confData.set10('ENABLE_MUSEPACK', true)
	extraDeps += libMPC
endif

formats += {'WavPack': libWavPack.found()}
if formats['WavPack']
	message('Enabling support for WavPack')
	extraSrcs += ['loadWavPack.cpp',]
	confData.set10('ENABLE_WAVPACK', true)
	if libWavPack.type_name() == 'internal'
		confData.set10('USE_MESON_WAVPACK', true)
	endif
	extraDeps += libWavPack
endif

formats += {'MP3': libID3Tag.found() and libMAD.found() and libLAME.found()}
if formats['MP3']
	message('Enabling support for MP3')
	extraSrcs += ['loadMP3.cxx', 'saveMP3.cxx',]
	confData.set10('ENABLE_MP3', true)
	extraDeps += [libID3Tag, libMAD, libLAME,]
endif

# TODO: decouple M4A from the AAC encoder
formats += {
	'AAC': libFAAD.found(),
	'M4A': libMP4v2.found() and libFAAC.found() and libFAAD.found()
}
if formats['M4A']
	message('Enabling support for AAC')
	extraSrcs += ['loadM4A.cpp', 'loadAAC.cpp', 'saveM4A.cpp',]
	confData.set10('ENABLE_M4A', true)
	confData.set10('ENABLE_AAC', true)
	extraDeps += [libMP4v2, libFAAC, libFAAD,]
endif

formats += {'WMA': extraFormats.contains('wma')}
if formats['WMA']
	message('Enabling support for Windows Media Audio')
	extraSrcs += 'loadWMA.cpp'
	confData.set10('ENABLE_WMA', true)
endif
formats += {'OptimFROG': libOptimFROG.found()}
if formats['OptimFROG']
	message('Enabling support for OptimFROG')
	extraSrcs += 'loadOptimFROG.cpp'
	confData.set10('ENABLE_OptimFROG', true)
	extraDeps += [libOptimFROG]
endif
formats += {'SNDH': extraFormats.contains('sndh')}
if formats['SNDH']
	message('Enabling support for SNDH')
	extraSrcs += [
		'loadSNDH.cpp',
		'sndh/loader.cxx',
		'sndh/iceDecrunch.cxx',
		'emulator/atariSTe.cxx',
		'emulator/cpu/m68k.cxx',
	]
	confData.set10('ENABLE_SNDH', true)
endif
formats += {'SID': extraFormats.contains('sid')}
if formats['SID']
	message('Enabling support for SID')
	extraSrcs += 'loadSID.cpp'
	confData.set10('ENABLE_SID', true)
endif
formats += {'AON': extraFormats.contains('aon')}
if formats['AON']
	message('Enabling support for AON')
	extraSrcs += 'loadAON.cpp'
	confData.set10('ENABLE_AON', true)
endif
formats += {'FC1x': extraFormats.contains('fc1x')}
if formats['FC1x']
	message('Enabling support for FC1x')
	extraSrcs += 'loadFC1x.cpp'
	confData.set10('ENABLE_FC1x', true)
endif
formats += {'Real Audio': extraFormats.contains('ra')}
if formats['Real Audio']
	message('Enabling support for RealAudio')
	extraSrcs += 'loadRealAudio.cpp'
	confData.set10('ENABLE_RA', true)
endif

if cxx.has_header_symbol('stdio.h', 'fseeko64')
	confData.set10('HAVE_FSEEKO64', true)
endif

if libOpenAL.type_name() != 'internal'
	if not cxx.has_header('AL/al.h', dependencies: libOpenAL)
		confData.set10('USE_CMAKE_OPENAL', true)
	endif
endif

if cxx.get_id() == 'msvc'
	includes += include_directories('windows')
endif

formats += {
	'MOD': true,
	'S3M': true,
	'STM': true,
	'IT': true,
	'WAV': true,
}

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
	'loadIT.cpp',
]

libAudioSrcs = [
	'loadAudio.cpp',
	'saveAudio.cpp',
	'fileInfo.cxx',
	'loadWAV.cpp',
	'fixedPoint/fixedPoint.cpp',
	'openAL.cxx',
	'openALPlayback.cxx',
	'playback.cxx',
	'console.cxx',
]

libAudioConfigHeader = configure_file(
	output : 'libAudioConfig.h',
	configuration : confData,
)

if target_machine.system() == 'windows'
	windows = import('windows')

	rcCfg = configuration_data()
	rcCfg.set('VERSION', '@0@,0'.format(meson.project_version().replace('.', ',')))
	rcCfg.set_quoted('DESCRIPTION', meson.project_name())
	rcCfg.set_quoted('FILENAME', 'libAudio.dll')
	rcCfg.set_quoted('VERSION_STRING', meson.project_version())
	rcCfg.set_quoted('NAME', meson.project_name())
	rcCfg.set_quoted('COMPANY', 'https://github.com/dragonCodecs/libAudio')
	rcCfg.set_quoted('COPYRIGHT', 'Copyright (c) 2009-2023 Rachel Mant @0@'.format(meson.project_license()))

	libAudioResource = configure_file(
		input: 'libAudio.rc.in',
		output: 'libAudio.rc',
		configuration: rcCfg
	)

	extraSrcs += windows.compile_resources(libAudioResource)
endif

summary(
	formats,
	section: 'Formats'
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
libAudioIncludes = [meson.current_source_dir(), meson.current_build_dir()]

libAudio = declare_dependency(
	include_directories: include_directories('.'),
	link_with: libAudioLibrary,
	version: meson.project_version()
)
