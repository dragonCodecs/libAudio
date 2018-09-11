include ../Makefile.inc

EXTRA_CFLAGS = $(shell pkg-config --cflags libAudio)
CFLAGS = -c $(OPTIM_FLAGS) $(EXTRA_CFLAGS) -o $@ $<
LIBS = $(shell pkg-config --libs libAudio)
LFLAGS = $(O) $(LIBS) -o $(EXE)
INSTALL_FILE = $(INSTALL) -m755
STRIP = strip -s

O = main.o
EXE = libAudio

default: all

all: $(EXE)

$(EXE): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call run-cmd,chmod,$(EXE))
	$(call debug-strip,$(EXE))

install: all
	$(call run-cmd,install_file,$(EXE),../bin)

clean:
	$(call run-cmd,rm,player,$(O) $(EXE))

.cpp.o:
	$(call run-cmd,cxx,$(CFLAGS))

.PHONY: .cpp.o clean install all default
