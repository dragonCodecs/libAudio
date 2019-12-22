threading = dependency('threads')
libOpenAL = dependency('openal')
libOGG = dependency('ogg')
libVorbis = [dependency('vorbis'), dependency('vorbisfile'), dependency('vorbisenc')]
libOpus = [dependency('opus'), dependency('opusfile')]
libFLAC = dependency('flac')
libWavPack = dependency('wavpack', version: '>=5.0.0')
libMPC = cxx.find_library('mpcdec')
libAAC = [cxx.find_library('faac'), cxx.find_library('faad')]
libMP4v2 = cxx.find_library('mp4v2')
libMAD = cxx.find_library('mad')
libID3Tag = cxx.find_library('id3tag')
maths = cxx.find_library('m', required: false)

cxx.has_function('MP4CreateProvider', dependencies: libMP4v2)
cxx.has_function('WavpackOpenFileInputEx64', dependencies: libWavPack)

extraFormats = get_option('extra_formats')
extraSrcs = []
extraArgs = []
extraDeps = []

if extraFormats.contains('wma')
	extraSrcs += 'loadWMA.cpp'
	extraArgs += '-DENABLE_WMA'
endif
if extraFormats.contains('optimFROG')
	extraSrcs += 'loadOptimFROG.cpp'
	extraArgs += '-DENABLE_OptimFROG'
	extraDeps += cxx.find_library('OptimFROG')
endif
if extraFormats.contains('sndh')
	extraSrcs += 'loadSNDH.cpp'
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

genericModuleSrcs = [
	'genericModule/ModuleFile.cpp', 'genericModule/ModuleHeader.cpp', 'genericModule/ModuleInstrument.cpp',
	'genericModule/ModuleSample.cpp', 'genericModule/ModulePattern.cpp', 'genericModule/ModuleEffects.cpp',
	'moduleMixer/moduleMixer.cpp', 'moduleMixer/channel.cxx',
	'loadMOD.cpp', 'loadS3M.cpp', 'loadSTM.cpp', 'loadAON.cpp', 'loadIT.cpp'
]

libAudioSrcs = [
	'loadAudio.cpp', 'loadOggVorbis.cpp', 'loadWAV.cpp', 'loadAAC.cpp', 'loadM4A.cpp',
	'loadMP3.cpp', 'loadMPC.cpp', 'loadFLAC.cpp', 'loadWavPack.cpp', 'fixedPoint/fixedPoint.cpp', genericModuleSrcs,
	'loadRealAudio.cpp', 'loadOggOpus.cxx', 'oggCommon.cxx',
	'saveAudio.cpp', 'saveOggVorbis.cpp', 'saveFLAC.cpp', 'saveM4A.cpp',
	'openAL.cxx', 'openALPlayback.cxx', 'playback.cxx', extraSrcs
]

libAudioLibrary = shared_library(
	'libAudio',
	libAudioSrcs,
	dependencies: [
		threading, libOpenAL, libOGG, libVorbis, libFLAC, libWavPack, libMPC,
		libAAC, libMP4v2, libMAD, libID3Tag, libOpus, extraDeps, maths
	],
	cpp_args: ['-DHAVE_STDINT_H', '-DlibAUDIO', extraArgs],
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
