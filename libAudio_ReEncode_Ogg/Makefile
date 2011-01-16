GCC ?= gcc
CC = $(GCC)
EXTRA_CFLAGS = $(shell pkg-config --cflags libAudio)
CFLAGS = -c $(EXTRA_CFLAGS) -o $*.o
LIBS = $(shell pkg-config --libs libAudio)
LFLAGS = $(O) $(LIBS) -o $(EXE)

O = main.o
EXE = libAudio_ReEncodeApp.out

default: all

all: $(O) $(EXE)

libAudio_ReEncodeApp.out:
	rm -f $(EXE)
	$(CC) $(LFLAGS)
	chmod +x $(EXE)

install: all
	install -m755 -D $(EXE) ../bin/libAudio_ReEncodeApp

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $*.cpp

main.o: main.cpp
