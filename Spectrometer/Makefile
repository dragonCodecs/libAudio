include Makefile.inc
STRIP = strip -s

PKG_CONFIG_PKGS = libAudio libGTK++ openal gl gobject-2.0
EXTRA_CFLAGS = $(shell pkg-config --cflags $(PKG_CONFIG_PKGS))
CFLAGS = $(OPTIM_FLAGS) -c -Wall -Wextra -pedantic $(EXTRA_CFLAGS) -o $@ $<
DEPFLAGS = $(OPTIM_FLAGS) -E -MM -Wall -Wextra -pedantic $(EXTRA_CFLAGS) -o .dep/$*.d $<
LIBS = $(shell pkg-config --libs $(PKG_CONFIG_PKGS)) -lm -ldl -lz -luuid -lresolv
LFLAGS = $(OPTIM_FLAGS) $(O) $(LIBS) -o $(EXE)

EXEDIR = ../bin

O = FFT.o AboutBox.o DrawingFunctions.o Playback.o VUMeter.o Icon.o Spectrometer.o
EXE = Spectrometer

default: all

all: .dep $(EXE)

.dep:
	$(call run-cmd,install_dir,.dep)

$(EXE): $(O)
	$(call run-cmd,ccld,$(LFLAGS))
	$(call run-cmd,chmod,$(EXE))
	$(call debug-strip,$(EXE))

$(EXEDIR):
	$(call run-cmd,install_dir,$(EXEDIR))

install: all $(EXEDIR)
	$(call run-cmd,install_exe,$(EXE),$(EXEDIR))

clean: .dep
	$(call run-cmd,rm,Spectrometer,*.o $(EXE) .dep/*.d)

.cpp.o:
	$(call makedep,$(DEPFLAGS) -std=c++11)
	$(call run-cmd,cxx,$(CFLAGS) -std=c++11)

.c.o:
	$(call makedep,$(DEPFLAGS))
	$(call run-cmd,cc,$(CFLAGS))

.PHONY: .cpp.o .c.o clean install all default

-include .dep/*.d
