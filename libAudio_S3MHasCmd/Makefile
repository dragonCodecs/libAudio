GCC ?= gcc
GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
else
GCC_FLAGS = 
endif
CC = $(GCC) $(GCC_FLAGS)
EXTRA_CFLAGS = -I../libAudio -DlibAUDIO -Wall -Wextra -pedantic -Wno-attributes
CFLAGS = -c -O2 -std=c++11 $(EXTRA_CFLAGS) -o $@
genericModule = ModuleEffects.cpp.o ModuleFile.cpp.o ModuleHeader.cpp.o ModulePattern.cpp.o ModuleSample.cpp.o ModuleInstrument.cpp.o
moduleMixer = moduleMixer.cpp.o channel.cxx.o
libAudio = $(addprefix genericModule_,$(genericModule)) $(addprefix moduleMixer_,$(moduleMixer)) fixedPoint_fixedPoint.cpp.o loadS3M.cpp.o \
	console.cxx.o
LIBS = $(addprefix ../build/libAudio/1ac7a65@@libAudio@sha/,$(libAudio)) -lstdc++ -lm $(shell pkg-config --libs openal)
LFLAGS = -O2 $(O) $(LIBS) -o $(EXE)
CHMOD = chmod 0755
STRIP = strip -s

O = main.o
EXE = libAudio_S3MHasCmd

default: all

all: $(EXE)

$(EXE): $(O)
	$(CC) $(LFLAGS)
	$(CHMOD) $(EXE)
	#$(STRIP) $(EXE)

install: all
	install -m755 -D $(EXE) ../bin

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $<

.PHONY: .cpp.o clean install all default
