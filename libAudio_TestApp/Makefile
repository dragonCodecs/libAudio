GCC ?= gcc
CC = $(GCC)
EXTRA_CFLAGS = $(shell pkg-config --cflags libAudio)
CFLAGS = -c $(EXTRA_CFLAGS) -o $*.o
LIBS = $(shell pkg-config --libs libAudio) -lm
LFLAGS = $(O) $(LIBS) -o $(EXE)

O = main.o
EXE = libAudio_TestApp.out

default: all

all: $(O) $(EXE)

libAudio_TestApp.out:
	rm -f $(EXE)
	$(CC) $(LFLAGS)
	chmod +x $(EXE)

install: all
	install -m755 -D $(EXE) ../bin/libAudio_TestApp

clean:
	rm -f *.o *.out *~

.cpp.o:
	$(CC) $(CFLAGS) $*.cpp

main.o: main.cpp
