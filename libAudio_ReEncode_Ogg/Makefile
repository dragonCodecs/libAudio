GCC ?= gcc
GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
else
GCC_FLAGS = 
endif
CC = $(GCC) $(GCC_FLAGS)
EXTRA_CFLAGS = $(shell pkg-config --cflags libAudio)
CFLAGS = -c $(EXTRA_CFLAGS) -o $*.o
LIBS = $(shell pkg-config --libs libAudio)
LFLAGS = $(O) $(LIBS) -o $(EXE)
CHMOD = chmod 0755
STRIP = strip -s

O = main.o
EXE = libAudio_ReEncodeApp.out

default: all

all: $(EXE)

libAudio_ReEncodeApp.out: $(O)
	rm -f $(EXE)
	$(CC) $(LFLAGS)
	$(CHMOD) $(EXE)
	$(STRIP) $(EXE)

install: all
	install -m755 -D $(EXE) ../bin/libAudio_ReEncodeApp

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $*.cpp

.PHONY: .cpp.o clean install all default

main.o: main.cpp
