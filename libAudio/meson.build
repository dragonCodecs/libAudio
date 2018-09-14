libOpenAL = dependency('openal')
libOGG = dependency('ogg')
libVorbis = [dependency('vorbis'), dependency('vorbisfile'), dependency('vorbisenc')]
libFLAC = dependency('flac')
libWavPack = dependency('wavpack')
libMPC = compiler.find_library('mpcdec')
libAAC = [compiler.find_library('faac'), compiler.find_library('faad')]
libMP4v2 = compiler.find_library('mp4v2')
libMAD = compiler.find_library('mad')
libID3Tag = compiler.find_library('id3tag')
maths = compiler.find_library('m', required: false)

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
	'saveAudio.cpp', 'saveOggVorbis.cpp', 'saveFLAC.cpp', 'saveM4A.cpp'
]

libAudio = shared_library(
	'libAudio',
	libAudioSrcs,
	dependencies: [libOpenAL, libOGG, libVorbis, libFLAC, libWavPack, libMPC, libAAC, libMP4v2, libMAD, libID3Tag, maths],
	cpp_args: ['-DHAVE_STDINT_H', '-DlibAUDIO', '-D__NO_OptimFROG__'],
	name_prefix: '',
	install: true,
	version: meson.project_version()
)
