GCC ?= gcc
GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
else
GCC_FLAGS = 
endif
CC = $(GCC) $(GCC_FLAGS)
EXTRA_CFLAGS = -I../libAudio
CFLAGS = -c -O2 $(EXTRA_CFLAGS) -o $@
genericModule = ModuleEffects.o ModuleFile.o ModuleHeader.o ModulePattern.o ModuleSample.o
libAudio = $(addprefix genericModule/,$(genericModule)) moduleMixer/moduleMixer.o fixedPoint/fixedPoint.o loadS3M.o libAudio_Common.o
LIBS = $(addprefix ../libAudio/,$(libAudio)) -lstdc++ -lm $(shell pkg-config --libs openal)
LFLAGS = $(O) $(LIBS) -o $(EXE)
CHMOD = chmod 0755
STRIP = strip -s

O = main.o
EXE = libAudio_S3MHasCmd

default: all

all: $(EXE)

$(EXE): $(O)
	$(CC) $(LFLAGS)
	$(CHMOD) $(EXE)
	$(STRIP) $(EXE)

install: all
	install -m755 -D $(EXE) ../bin

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $<

.PHONY: .cpp.o clean install all default
