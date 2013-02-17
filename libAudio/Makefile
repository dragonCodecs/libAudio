include ../Makefile.inc

PKG_CONFIG_PKGS = openal ogg vorbis vorbisfile vorbisenc flac wavpack
EXTRA_DEFINE = -DHAVE_STDINT_H -DlibAUDIO -D__NO_IT__ -D__NO_OptimFROG__
EXTRA_CFLAGS = $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
CFLAGS = -c $(OPTIM_FLAGS) -pedantic -Wall $(EXTRA_CFLAGS) $(EXTRA_DEFINE) -o $@ $<
EXTRA_LIBS = $(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LIBS = -lstdc++ $(EXTRA_LIBS) -lmpcdec -lfaac -lfaad -lmp4v2 -lmad -lid3tag -lm
LFLAGS = -shared $(O) $(LIBS) -Wl,-soname,$(SOMAJ) -o $(SO)

SED = sed -e 's:@LIBDIR@:$(LIBDIR):g'

LIBDIR ?= /usr/lib
PKGDIR = $(LIBDIR)/pkgconfig/
INCDIR = /usr/include/

#WMA = loadWMA.o
WMA = 
#IT = loadIT.o mixIT/mixIT.o
#SHORTEN = loadShorten.o
SHORTEN = 
#OPTIMFROG = loadOptimFROG.o
OPTIMFROG = 
H = libAudio.h
O_MOD = genericModule/ModuleFile.o genericModule/ModuleHeader.o genericModule/ModuleSample.o genericModule/ModulePattern.o genericModule/ModuleEffects.o moduleMixer/moduleMixer.o loadMOD.o loadS3M.o loadSTM.o loadFC1x.o
#O_SNDH = loadSNDH.o
O_SNDH = 
O_SID = loadSID.o
O = loadAudio.o libAudio_Common.o loadOggVorbis.o loadWAV.o loadAAC.o loadM4A.o loadMP3.o loadMPC.o loadFLAC.o fixedPoint/fixedPoint.o $(O_MOD) $(O_SNDH) $(O_SID) loadWavPack.o $(OPTIMFROG) $(SHORTEN) loadRealAudio.o $(WMA)  saveAudio.o saveOggVorbis.o saveFLAC.o saveM4A.o
VERMAJ = .0
VERMIN = .1
VERREV = .43
VER = $(VERMAJ)$(VERMIN)$(VERREV)
SOMAJ = libAudio.so
SOMIN = $(SOMAJ)$(VERMAJ)
SOREV = $(SOMIN)$(VERMIN)
SO = $(SOREV)$(VERREV)
A = libAudio.a
LA = libAudio.la
PC = libAudio.pc
IN = libAudio.pc.in

default: all

all: $(SO) $(PC)

install: all
	$(call run-cmd,install_file,$(SO),$(LIBDIR))
	$(call run-cmd,install_file,$(PC),$(PKGDIR))
	$(call run-cmd,install_file,$(H),$(PKGDIR))
	$(call run-cmd,ln,$(LIBDIR)/$(SO),$(LIBDIR)/$(SOREV))
	$(call run-cmd,ln,$(LIBDIR)/$(SOREV),$(LIBDIR)/$(SOMIN))
	$(call run-cmd,ln,$(LIBDIR)/$(SOMIN),$(LIBDIR)/$(SOMAJ))
	$(call run-cmd,ldconfig)

uninstall:
	rm $(LIBDIR)/$(SOMAJ)*
	rm $(LIBDIR)/$(A)
	rm $(LIBDIR)/$(LA)
	rm $(PKGDIR)/$(PC)

$(SO): $(O)
	$(call run-cmd,ar,$(A),$(O))
	$(call run-cmd,ranlib,$(A))
	$(call run-cmd,ccld,$(LFLAGS))
	$(call debug-strip,$(SO))

libAudio.pc: $(IN)
	$(call run-cmd,sed,$(IN),$(PC))

clean:
	$(call run-cmd,rm,mixIT,mixIT/*.o)
	$(call run-cmd,rm,moduleMixer,moduleMixer/*.o)
	$(call run-cmd,rm,fixedPoint,fixedPoint/*.o)
	$(call run-cmd,rm,genericModule,genericModule/*.o)
	$(call run-cmd,rm,libAudio,*.o $(SOMAJ)* $(A) $(PC))

.cpp.o:
	$(call run-cmd,cc,$(CFLAGS))

.PHONY: clean libAudio.pc install all uninstall default .cpp.o

loadAudio.o: loadAudio.cpp
libAudio_Common.o: libAudio_Common.cpp
loadOggVorbis.o: loadOggVorbis.cpp
loadWAV.o: loadWAV.cpp
loadAAC.o: loadAAC.cpp
loadM4A.o: loadM4A.cpp
loadMP3.o: loadMP3.cpp
loadMPC.o: loadMPC.cpp
loadFLAC.o: loadFLAC.cpp
fixedPoint/fixedPoint.o: CFLAGS += -std=c++11
fixedPoint/fixedPoint.o: fixedPoint/fixedPoint.cpp
genericModule/ModuleFile.o: genericModule/ModuleFile.cpp
genericModule/ModuleHeader.o: genericModule/ModuleHeader.cpp
genericModule/ModuleSample.o: genericModule/ModuleSample.cpp
genericModule/ModulePattern.o: genericModule/ModulePattern.cpp
genericModule/ModuleEffects.o: genericModule/ModuleEffects.cpp
loadMOD.o: loadMOD.cpp
moduleMixer/moduleMixer.o: moduleMixer/moduleMixer.cpp moduleMixer/mixFunctions.h moduleMixer/mixFunctionTables.h moduleMixer/waveTables.h \
	moduleMixer/frequencyTables.h moduleMixer/moduleMixer.h
loadSNDH.o: loadSNDH.cpp
loadS3M.o: loadS3M.cpp
loadSTM.o: loadSTM.cpp
loadFC1x.o: loadFC1x.cpp
#loadIT.o: loadIT.cpp
#mixIT/mixIT.o: mixIT/mixIT.cpp
loadSID.o: loadSID.cpp
loadWavPack.o: loadWavPack.cpp
loadOptimFROG.o: loadOptimFROG.cpp
loadShorten.o: loadShorten.cpp
loadRealAudio.o: loadRealAudio.cpp
loadWMA.o: loadWMA.cpp

saveAudio.o: saveAudio.cpp
saveOggVorbis.o: saveOggVorbis.cpp
saveFLAC.o: saveFLAC.cpp
saveM4A.o: saveM4A.cpp
