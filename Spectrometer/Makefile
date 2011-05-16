GCC ?= gcc
GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
else
GCC_FLAGS = 
endif
CC = $(GCC) $(GCC_FLAGS)
EXTRA_CFLAGS = $(shell pkg-config --cflags libAudio libGTK++)
CFLAGS = -O2 -g -c $(EXTRA_CFLAGS) -o $@
LIBS = $(shell pkg-config --libs libAudio libGTK++)
LFLAGS = $(O) $(LIBS) -o $(EXE)
CHMOD = chmod 0755
STRIP = strip -s

O = FFT.o AboutBox.o DrawingFunctions.o Playback.o VUMeter.o Spectrometer.o
EXE = Spectrometer.out

default: all

all: $(EXE)

Spectrometer.out: $(O)
	$(CC) $(LFLAGS)
	$(CHMOD) $(EXE)
	$(STRIP) $(EXE)

install: all
	install -m755 -D $(EXE) ../bin/Spectrometer

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $<

.c.o:
	$(GCC) $(CFLAGS) $<

.PHONY: .cpp.o clean install all default

FFT.o: FFT.c
AboutBox.o: AboutBox.cpp
DrawingFunctions.o: DrawingFunctions.cpp
Playback.o: Playback.cpp
VUMeter.o: VUMeter.cpp
Spectrometer.o: Spectrometer.cpp
