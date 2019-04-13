threading = dependency('threads')
libOpenAL = dependency('openal')
libOGG = dependency('ogg')
libVorbis = [dependency('vorbis'), dependency('vorbisfile'), dependency('vorbisenc')]
libFLAC = dependency('flac')
libWavPack = dependency('wavpack')
libMPC = cxx.find_library('mpcdec')
libAAC = [cxx.find_library('faac'), cxx.find_library('faad')]
libMP4v2 = cxx.find_library('mp4v2')
libMAD = cxx.find_library('mad')
libID3Tag = cxx.find_library('id3tag')
maths = cxx.find_library('m', required: false)

cxx.has_function('MP4CreateProvider', dependencies: libMP4v2)

#WMA = loadWMA.o
srcWMA = []
#SHORTEN = loadShorten.o
srcShorten = []
#OPTIMFROG = loadOptimFROG.o
srcOptimFROG = []
#O_SNDH = loadSNDH.o
srcSNDH = []
srcSID = 'loadSID.cpp'

genericModuleSrcs = [
	'genericModule/ModuleFile.cpp', 'genericModule/ModuleHeader.cpp', 'genericModule/ModuleInstrument.cpp', 'genericModule/ModuleSample.cpp',
	'genericModule/ModulePattern.cpp', 'genericModule/ModuleEffects.cpp', 'moduleMixer/moduleMixer.cpp',
	'loadMOD.cpp', 'loadS3M.cpp', 'loadSTM.cpp', 'loadAON.cpp', 'loadFC1x.cpp', 'loadIT.cpp'
]

libAudioSrcs = [
	'loadAudio.cpp', 'libAudio_Common.cpp', 'loadOggVorbis.cpp', 'loadWAV.cpp', 'loadAAC.cpp', 'loadM4A.cpp',
	'loadMP3.cpp', 'loadMPC.cpp', 'loadFLAC.cpp', 'fixedPoint/fixedPoint.cpp', genericModuleSrcs, srcSNDH, srcSID,
	'loadWavPack.cpp', srcOptimFROG, srcShorten, 'loadRealAudio.cpp', srcWMA,
	'saveAudio.cpp', 'saveOggVorbis.cpp', 'saveFLAC.cpp', 'saveM4A.cpp', 'openAL.cxx', 'openALPlayback.cxx', 'playback.cxx'
]

libAudio = declare_dependency(
	include_directories: include_directories('.'),
	link_with: shared_library(
		'libAudio',
		libAudioSrcs,
		dependencies: [threading, libOpenAL, libOGG, libVorbis, libFLAC, libWavPack, libMPC, libAAC, libMP4v2, libMAD, libID3Tag, maths],
		cpp_args: ['-DHAVE_STDINT_H', '-DlibAUDIO', '-D__NO_OptimFROG__'],
		name_prefix: '',
		install: true,
		version: meson.project_version()
	),
	version: meson.project_version()
)
