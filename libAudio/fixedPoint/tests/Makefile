GCC ?= gcc
#GCC_VER = $(shell gcc -dumpversion | cut -d . -f 1)
#ifeq ($(shell if [ $(GCC_VER) -ge 4 ]; then echo 1; else echo 0; fi), 1)
#GCC_FLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
#else
#GCC_FLAGS = 
#endif
GCC_FLAGS = -ggdb
CC = $(GCC) $(GCC_FLAGS)
CFLAGS = -c -o $@ $<
LIBS = ../fixedPoint.o -lstdc++ -lm
LFLAGS = $< $(LIBS) -o $@
INSTALL = install -m755

EXE = TestLinearSlideUp.out TestLinearSlideDown.out
O = TestLinearSlideUp.o TestLinearSlideDown.o

default: all

all: $(EXE)

clean:
	rm -f *.o *~

$(EXE): ../fixedPoint.o

.o.out:
	$(CC) $(LFLAGS)
	$(INSTALL) $@ $*

.cpp.o:
	$(CC) $(CFLAGS)

.PHONY: .cpp.o .o.out clean default all

TestLinearSlideUp.o: TestLinearSlideUp.cpp
TestLinearSlideDown.o: TestLinearSlideDown.cpp

TestLinearSlideUp.out: TestLinearSlideUp.o
TestLinearSlideDown.out: TestLinearSlideDown.o
