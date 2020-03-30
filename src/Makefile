#
#         Plain general makefile, for ultradefrag compilations 
#         Host Linux, target Linux on X86-32 or X86-64
#

# restrict suffixes list to the ones we define
.SUFFIXES :
# this list controls the ordering of rule evaluation
.SUFFIXES : .c .cpp .java .s .o .l .map

# cancel implicit rule for building .c from .l (lex file) :
%.c : %.l

# cancel implicit rule for building . from .c
% : %.c

# cancel implicit rule for building . from .s
% : %.s

GCC=gcc
GCC_VERSION=$(shell expr `$(GCC) -dumpversion`)
LD=ld
AR=ar
INCL=-I/usr/include -Iinclude
COPT=-DLXGC=1 -O2
GCCOPT=-DLXGC=1 -O2
LIB1=/usr/lib64
LIB2=/usr/lib/gcc/x86_64-pc-linux-gnu/$(GCC_VERSION)
NTFSLIB=/lib64/libntfs-3g.so.*.0.0

A=console.a udefrag.a zenwinx.a wincalls.a

.c.s :
	$(GCC) $(COPT) $(INCL) -S $*.c

.cpp.o :
	$(GCC) $(GCCOPT) $(INCL) -c -o$*.o $*.cpp

.c.o :
	$(GCC) $(GCCOPT) $(INCL) -c -o$*.o $*.c

.cpp.s :
	$(GCC) $(GCCOPT) $(INCL) -S -o$*.s $*.cpp

all : udefrag

console.a :
	cd console; make console.a

udefrag.a :
	cd dll/udefrag; make udefrag.a

zenwinx.a :
	cd dll/zenwinx; make zenwinx.a

wincalls.a :
	cd wincalls; make wincalls.a

clean :
	cd console; make clean
	cd dll/udefrag; make clean
	cd dll/zenwinx; make clean
	cd wincalls; make clean

udefrag : $(A)
	$(LD) -dynamic-linker /lib64/ld-linux-x86-64.so.2 -o udefrag -M \
		$(LIB1)/crt1.o $(LIB1)/crti.o $(LIB2)/crtbegin.o \
		console/console.a dll/udefrag/udefrag.a dll/zenwinx/zenwinx.a \
		wincalls/wincalls.a $(NTFSLIB) \
		-lgcc -L$(LIB2) -L$(LIB1) -lc -lm -lpthread -lncurses \
		$(LIB2)/crtend.o $(LIB1)/crtn.o > udefrag.map
