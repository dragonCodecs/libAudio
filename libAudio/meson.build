# SPDX-License-Identifier: BSD-3-Clause
threading = dependency('threads')
libOpenAL = dependency('openal')
libOGG = dependency('ogg')
libVorbis = [dependency('vorbis'), dependency('vorbisfile'), dependency('vorbisenc')]
libOpus = [dependency('opus'), dependency('opusfile'), dependency('libopusenc')]
libFLAC = dependency('flac')
libWavPack = dependency('wavpack', version: '>=5.0.0')
libMPC = cxx.find_library('mpcdec')
libAAC = [cxx.find_library('faac'), cxx.find_library('faad')]
libMP4v2 = cxx.find_library('mp4v2')
libMAD = cxx.find_library('mad')
libID3Tag = cxx.find_library('id3tag')
libLAME = cxx.find_library('mp3lame')
maths = cxx.find_library('m', required: false)

if not cxx.has_function('MP4CreateProvider', dependencies: libMP4v2)
	error('Could not find the required function MP4CreateProvider() from libmp4v2')
elif not cxx.has_function('WavpackOpenFileInputEx64', dependencies: libWavPack)
	error('Could not find the required function WavpackOpenFileInputEx64() from libwavpack')
endif

extraFormats = get_option('extra_formats')
extraSrcs = []
extraArgs = []
extraDeps = []
includes = []

if extraFormats.contains('wma')
	extraSrcs += 'loadWMA.cpp'
	extraArgs += '-DENABLE_WMA'
endif
if extraFormats.contains('optimFROG')
	extraSrcs += 'loadOptimFROG.cpp'
	extraArgs += '-DENABLE_OptimFROG'
	extraDeps += dependency('optimfrog')
endif
if extraFormats.contains('sndh')
	extraSrcs += ['loadSNDH.cpp', 'sndh/loader.cxx']
	extraArgs += '-DENABLE_SNDH'
endif
if extraFormats.contains('sid')
	extraSrcs += 'loadSID.cpp'
	extraArgs += '-DENABLE_SID'
endif
if extraFormats.contains('aon')
	extraSrcs += 'loadAON.cpp'
	extraArgs += '-DENABLE_AON'
endif
if extraFormats.contains('fc1x')
	extraSrcs += 'loadFC1x.cpp'
	extraArgs += '-DENABLE_FC1x'
endif
if extraFormats.contains('ra')
	extraSrcs += 'loadRealAudio.cpp'
	extraArgs += '-DENABLE_RA'
endif
if target_machine.system() == 'windows'
	includes += include_directories('windows')
endif

genericModuleSrcs = [
	'genericModule/ModuleFile.cpp', 'genericModule/ModuleHeader.cpp', 'genericModule/ModuleInstrument.cpp',
	'genericModule/ModuleSample.cpp', 'genericModule/ModulePattern.cpp', 'genericModule/ModuleEffects.cpp',
	'moduleMixer/moduleMixer.cpp', 'moduleMixer/channel.cxx',
	'loadMOD.cpp', 'loadS3M.cpp', 'loadSTM.cpp', 'loadAON.cpp', 'loadIT.cpp'
]

libAudioSrcs = [
	'loadAudio.cpp', 'loadOggVorbis.cpp', 'loadWAV.cpp', 'loadAAC.cpp', 'loadM4A.cpp',
	'loadMP3.cxx', 'loadMPC.cpp', 'loadFLAC.cpp', 'loadWavPack.cpp', 'fixedPoint/fixedPoint.cpp',
	genericModuleSrcs, 'loadOggOpus.cxx', 'oggCommon.cxx',
	'saveAudio.cpp', 'saveOggVorbis.cpp', 'saveOggOpus.cxx', 'saveFLAC.cpp',
	'saveM4A.cpp', 'saveMP3.cxx', 'openAL.cxx', 'openALPlayback.cxx', 'playback.cxx',
	'console.cxx', extraSrcs
]

libAudioLibrary = shared_library(
	'libAudio',
	libAudioSrcs,
	dependencies: [
		threading, libOpenAL, libOGG, libVorbis, libFLAC, libWavPack, libMPC,
		libAAC, libMP4v2, libMAD, libID3Tag, libLAME, libOpus, extraDeps, maths,
		substrate
	],
	cpp_args: ['-DHAVE_STDINT_H', '-DlibAUDIO', extraArgs],
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
