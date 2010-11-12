CC = gcc
CFLAGS = -c `pkg-config --cflags openal ogg vorbis vorbisfile vorbisenc flac wavpack` -DHAVE_STDINT_H -DlibAUDIO -D__NO_SAVE_M4A__ -o $*.o
LIBS = -lstdc++ `pkg-config --libs openal ogg vorbis vorbisfile vorbisenc flac wavpack` -lmpcdec -lfaac -lmp4v2 -lfaad -lmp4ff -lOptimFROG -lmad -lid3tag
LFLAGS = -shared $(O) $(LIBS) -Wl,-soname,$(SOMAJ) -o $(SO)
AR = ar cr
RANLIB = ranlib
LN = ln -sfT
LIBDIR = /usr/lib/
PKGDIR = /usr/lib/pkgconfig/
INCDIR = /usr/include/

#WMA = loadWMA.o
WMA = 
H = libAudio.h
O = loadAudio.o libAudio_Common.o loadOggVorbis.o loadWAV.o loadAAC.o loadM4A.o loadMP3.o loadMPC.o loadFLAC.o loadIT.o mixIT/mixIT.o loadWavPack.o loadOptimFROG.o loadShorten.o loadRealAudio.o $(WMA)  saveAudio.o saveOggVorbis.o saveFLAC.o saveM4A.o
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

default: all

all: $(O) $(SO)

install: all
	cp $(SO) $(LIBDIR)
	cp $(PC) $(PKGDIR)
	cp $(H) $(INCDIR)
	chmod +r $(INCDIR)$(H)
	$(LN) $(LIBDIR)$(SO) $(LIBDIR)$(SOREV)
	$(LN) $(LIBDIR)$(SOREV) $(LIBDIR)$(SOMIN)
	$(LN) $(LIBDIR)$(SOMIN) $(LIBDIR)$(SOMAJ)

uninstall:
	rm $(LIBDIR)$(SOMAJ)*
	rm $(LIBDIR)$(A)
	rm $(LIBDIR)$(LA)
	rm $(PKGDIR)$(PC)

libAudio.so$(VER):
	rm -f $(SO) $(A)
	$(AR) $(A) $(O)
	$(RANLIB) $(A)
	$(CC) $(LFLAGS)

clean:
	rm -f *.o *.so* *.a *~
	@cd mixIT && rm -f *.o *~

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
loadIT.o: loadIT.cpp
mixIT/mixIT.o: mixIT/mixIT.cpp
loadWavPack.o: loadWavPack.cpp
loadOptimFROG.o: loadOptimFROG.cpp
loadShorten.o: loadShorten.cpp
loadRealAudio.o: loadRealAudio.cpp
loadWMA.o: loadWMA.cpp

saveAudio.o: saveAudio.cpp
saveOggVorbis.o: saveOggVorbis.cpp
saveFLAC.o: saveFLAC.cpp
saveM4A.o: saveM4A.cpp
