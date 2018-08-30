include $(call my-dir)/*/Android.mk
#SRC_WMA = loadWMA.cpp
SRC_WMA =
#SRC_SHORTEN = loadShorten.cpp
SRC_SHORTEN =
#SRC_OPTIMFROG = loadOptimFROG.cpp
SRC_OPTIMFROG =
SRC_MOD = loadMOD.cpp loadS3M.cpp loadSTM.cpp loadAON.cpp loadFC1x.cpp loadIT.cpp
#SRC_SHDN = loadSHDN.cpp
SRC_SHDN =
SRC_SID = loadSID.cpp
LOCAL_SRC_FILES += loadAudio.cpp libAudio_Common.cpp loadOggVorbis.cpp loadWAV.cpp loadAAC.cpp loadM4A.cpp loadMP3.cpp loadMPC.cpp loadFLAC.cpp $(SRC_MOD) $(SRC_SHDN) $(SRC_SID) \
	loadWavPack.cpp $(SRC_OPTIMFROG) $(SRC_SHORTEN) loadRealAudio.cpp $(SRC_WMA) saveAudio.cpp saveOggVorbis.cpp saveFLAC.cpp saveM4A.cpp

LOCAL_SRC_FILES := $(addprefix libAudio/,$(LOCAL_SRC_FILES))