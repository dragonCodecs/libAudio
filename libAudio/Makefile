GCC ?= gcc
GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
else
GCC_FLAGS = 
endif
CC = $(GCC) $(GCC_FLAGS)
EXTRA_CFLAGS = $(shell pkg-config --cflags openal ogg vorbis vorbisfile vorbisenc flac wavpack)
CFLAGS = -c $(EXTRA_CFLAGS) -DHAVE_STDINT_H -DlibAUDIO -D__NO_IT__ -D__NO_OptimFROG__ -o $*.o
EXTRA_LIBS = $(shell pkg-config --libs openal ogg vorbis vorbisfile vorbisenc flac wavpack)
LIBS = -lstdc++ $(EXTRA_LIBS) -lmpcdec -lfaac -lfaad -lmp4v2 -lmad -lid3tag
LFLAGS = -shared $(O) $(LIBS) -Wl,-soname,$(SOMAJ) -o $(SO)
AR = ar cr
RANLIB = ranlib
LN = ln -sfT
LIBDIR ?= /usr/lib
PKGDIR = $(LIBDIR)/pkgconfig/
INCDIR = /usr/include/
INSTALL = install -m644
STRIP = strip -x

#WMA = loadWMA.o
WMA = 
#IT = loadIT.o mixIT/mixIT.o
IT = 
#SHORTEN = loadShorten.o
SHORTEN = 
#OPTIMFROG = loadOptimFROG.o
OPTIMFROG = 
H = libAudio.h
O = loadAudio.o libAudio_Common.o loadOggVorbis.o loadWAV.o loadAAC.o loadM4A.o loadMP3.o loadMPC.o loadFLAC.o loadMOD.o moduleMixer/moduleMixer.o $(IT) loadWavPack.o $(OPTIMFROG) $(SHORTEN) loadRealAudio.o $(WMA)  saveAudio.o saveOggVorbis.o saveFLAC.o saveM4A.o
VERMAJ = .0
VERMIN = .1
VERREV = .43
VER = $(VERMAJ)$(VERMIN)$(VERREV)
SO = libAudio.so$(VER)
SOREV = libAudio.so$(VERMAJ)$(VERMIN)
SOMIN = libAudio.so$(VERMAJ)
SOMAJ = libAudio.so
A = libAudio.a
LA = libAudio.la
PC = libAudio.pc
IN = libAudio.pc.in

default: all

all: $(O) $(SO) $(PC)

install: all
	$(INSTALL) $(SO) $(LIBDIR)
	$(INSTALL) $(PC) $(PKGDIR)
	$(INSTALL) $(H) $(INCDIR)
	$(LN) $(LIBDIR)/$(SO) $(LIBDIR)/$(SOREV)
	$(LN) $(LIBDIR)/$(SOREV) $(LIBDIR)/$(SOMIN)
	$(LN) $(LIBDIR)/$(SOMIN) $(LIBDIR)/$(SOMAJ)

uninstall:
	rm $(LIBDIR)/$(SOMAJ)*
	rm $(LIBDIR)/$(A)
	rm $(LIBDIR)/$(LA)
	rm $(PKGDIR)/$(PC)

libAudio.so$(VER):
	rm -f $(SO) $(A)
	$(AR) $(A) $(O)
	$(RANLIB) $(A)
	$(CC) $(LFLAGS)
	$(STRIP) $(SO)

libAudio.pc:
	$(SHELL) -c "sed -e 's:@LIBDIR@:$(LIBDIR):g' $(IN) > $(PC)"

clean:
	rm -f *.o *.so* *.a *~ *.pc
	@cd mixIT && rm -f *.o *~
	@cd moduleMixer && rm -f *.o *~

.cpp.o:
	$(CC) $(CFLAGS) $*.cpp

loadAudio.o: loadAudio.cpp
libAudio_Common.o: libAudio_Common.cpp
loadOggVorbis.o: loadOggVorbis.cpp
loadWAV.o: loadWAV.cpp
loadAAC.o: loadAAC.cpp
loadM4A.o: loadM4A.cpp
loadMP3.o: loadMP3.cpp
loadMPC.o: loadMPC.cpp
loadFLAC.o: loadFLAC.cpp
loadMOD.o: loadMOD.cpp
moduleMixer/moduleMixer.o: moduleMixer/moduleMixer.cpp
#loadIT.o: loadIT.cpp
#mixIT/mixIT.o: mixIT/mixIT.cpp
loadWavPack.o: loadWavPack.cpp
loadOptimFROG.o: loadOptimFROG.cpp
loadShorten.o: loadShorten.cpp
loadRealAudio.o: loadRealAudio.cpp
loadWMA.o: loadWMA.cpp

saveAudio.o: saveAudio.cpp
saveOggVorbis.o: saveOggVorbis.cpp
saveFLAC.o: saveFLAC.cpp
saveM4A.o: saveM4A.cpp
