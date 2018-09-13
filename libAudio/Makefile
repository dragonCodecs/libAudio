include ../Makefile.inc

PKG_CONFIG_PKGS = openal ogg vorbis vorbisfile vorbisenc flac wavpack
EXTRA_DEFINE = -DHAVE_STDINT_H -DlibAUDIO -D__NO_OptimFROG__
EXTRA_CFLAGS = $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
CFLAGS = $(OPTIM_FLAGS) -c -Wall -Wextra -pedantic $(EXTRA_CFLAGS) $(EXTRA_DEFINE) -o $@ $<
DEPFLAGS = $(OPTIM_FLAGS) -E -MM -Wall -Wextra -pedantic $(EXTRA_CFLAGS) $(EXTRA_DEFINE) -o .dep/$*.d $<
EXTRA_LIBS = $(shell pkg-config --libs $(PKG_CONFIG_PKGS))
LIBS = -lstdc++ $(EXTRA_LIBS) -lmpcdec -lfaac -lfaad -lmp4v2 -lmad -lid3tag -lm
LFLAGS = -shared $(O) $(LIBS) -Wl,-soname,$(SOMAJ) -z defs -o $(SO)

SED = sed -e 's:@LIBDIR@:$(LIBDIR):g' -e 's:@PREFIX@:$(PREFIX):g'

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
PKGDIR = $(LIBDIR)/pkgconfig/
INCDIR = $(PREFIX)/include/

#WMA = loadWMA.o
WMA =
#SHORTEN = loadShorten.o
SHORTEN =
#OPTIMFROG = loadOptimFROG.o
OPTIMFROG =
H = libAudio.h
O_MOD = genericModule/ModuleFile.o genericModule/ModuleHeader.o genericModule/ModuleInstrument.o genericModule/ModuleSample.o genericModule/ModulePattern.o genericModule/ModuleEffects.o moduleMixer/moduleMixer.o \
	loadMOD.o loadS3M.o loadSTM.o loadAON.o loadFC1x.o loadIT.o
#O_SNDH = loadSNDH.o
O_SNDH =
O_SID = loadSID.o
O = loadAudio.o libAudio_Common.o loadOggVorbis.o loadWAV.o loadAAC.o loadM4A.o loadMP3.o loadMPC.o loadFLAC.o fixedPoint/fixedPoint.o $(O_MOD) $(O_SNDH) $(O_SID) \
	loadWavPack.o $(OPTIMFROG) $(SHORTEN) loadRealAudio.o $(WMA) saveAudio.o saveOggVorbis.o saveFLAC.o saveM4A.o
VERMAJ = .0
VERMIN = .2
VERREV = .1
VER = $(VERMAJ)$(VERMIN)$(VERREV)
SOMAJ = libAudio.so
SOMIN = $(SOMAJ)$(VERMAJ)
SOREV = $(SOMIN)$(VERMIN)
SO = $(SOREV)$(VERREV)
A = libAudio.a
PC = libAudio.pc
IN = libAudio.pc.in

DEPS = .dep .dep/moduleMixer .dep/fixedPoint .dep/genericModule

default: all

all: $(DEPS) $(SO) $(PC)

.dep:
	$(call run-cmd,install_dir,.dep)

.dep/moduleMixer:
	$(call run-cmd,install_dir,.dep/moduleMixer)

.dep/fixedPoint:
	$(call run-cmd,install_dir,.dep/fixedPoint)

.dep/genericModule:
	$(call run-cmd,install_dir,.dep/genericModule)

install: all
	$(call run-cmd,install_dir,$(LIBDIR))
	$(call run-cmd,install_dir,$(PKGDIR))
	$(call run-cmd,install_dir,$(INCDIR))
	$(call run-cmd,install_file,$(SO),$(LIBDIR))
	$(call run-cmd,install_file,$(PC),$(PKGDIR))
	$(call run-cmd,install_file,$(H),$(INCDIR))
	$(call run-cmd,ln,$(LIBDIR)/$(SO),$(LIBDIR)/$(SOREV))
	$(call run-cmd,ln,$(LIBDIR)/$(SOREV),$(LIBDIR)/$(SOMIN))
	$(call run-cmd,ln,$(LIBDIR)/$(SOMIN),$(LIBDIR)/$(SOMAJ))
	$(call ldconfig)

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

clean: $(DEPS)
	$(call run-cmd,rm,moduleMixer,moduleMixer/*.o)
	$(call run-cmd,rm,fixedPoint,fixedPoint/*.o)
	$(call run-cmd,rm,genericModule,genericModule/*.o)
	$(call run-cmd,rm,libAudio,*.o $(SOMAJ)* $(A) $(PC))
	$(call run-cmd,rm,makedep,.dep/*.d .dep/moduleMixer/*.d .dep/fixedPoint/*.d .dep/genericModule/*.d)

.cpp.o:
	$(call makedep,$(CXX),$(DEPFLAGS) -std=c++11)
	$(call run-cmd,cxx,$(CFLAGS) -std=c++11)

.PHONY: clean libAudio.pc install all uninstall default .cpp.o

-include .dep/*.d
-include .dep/moduleMixer/*.d
-include .dep/fixedPoint/*.d
-include .dep/genericModule/*.d
