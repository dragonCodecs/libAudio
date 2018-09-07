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
	name_prefix: '',
	install: true,
	version: meson.project_version()
)
