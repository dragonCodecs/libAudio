# SPDX-License-Identifier: BSD-3-Clause
include $(call my-dir)/*/Android.mk
#SRC_WMA = loadWMA.cpp
SRC_WMA =
#SRC_OPTIMFROG = loadOptimFROG.cpp
SRC_OPTIMFROG =
#SRC_FC1x = loadFC1x.cpp
SRC_FC1x =
SRC_MOD = loadMOD.cpp loadS3M.cpp loadSTM.cpp loadAON.cpp $(SRC_FC1x) loadIT.cpp
#SRC_SHDN = loadSHDN.cpp
SRC_SHDN =
SRC_SID = loadSID.cpp
LOCAL_SRC_FILES += loadAudio.cpp libAudio_Common.cpp loadOggVorbis.cpp loadWAV.cpp loadAAC.cpp loadM4A.cpp loadMP3.cpp loadMPC.cpp \
	loadFLAC.cpp $(SRC_MOD) $(SRC_SHDN) $(SRC_SID) loadWavPack.cpp $(SRC_OPTIMFROG) loadRealAudio.cpp $(SRC_WMA) \
	saveAudio.cpp saveOggVorbis.cpp saveFLAC.cpp saveM4A.cpp openAL.cxx openALPlayback.cxx playback.cxx

LOCAL_SRC_FILES := $(addprefix libAudio/,$(LOCAL_SRC_FILES))
