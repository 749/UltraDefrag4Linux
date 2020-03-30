#
#         Plain general makefile, for ultradefrag compilations 
#         Host Linux, target Linux on Sparc 32-bit
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

GCC=/shared/sparc/gcc/bin/sparc-sun-linux-gcc
LD=/shared/sparc/gcc/sparc-sun-linux/bin/ld
AR=/shared/sparc/gcc/sparc-sun-linux/bin/ar
INCL=-I/shared/sparc/root/usr/sparc-include -I/shared/c-src/include/linux \
     -Iinclude
COPT=-DSPGC=1 -O2
GCCOPT=-DSPGC=1 -O2
LIB1=/shared/sparc/root/usr/lib
LIB2=/shared/sparc/gcc/lib/gcc/sparc-sun-linux/4.0.0
NTFS=/shared/ntfs/ntfslowprof

A=console.a udefrag.a zenwinx.a wincalls.a

.c.s :
	$(GCC) $(COPT) $(INCL) -S $*.c

.cpp.o :
	$(GCC) $(GCCOPT) $(INCL) -c -o$*.o $*.cpp

.c.o :
	$(GCC) $(GCCOPT) $(INCL) -c -o$*.o $*.c

.cpp.s :
	$(GCC) $(GCCOPT) $(INCL) -S -o$*.s $*.cpp

.o .o. :
	$(LD) -dynamic-linker /lib/ld-linux.so.2 -o $* \
		-s $(LIB1)/crt1.o $(LIB1)/crti.o $(LIB2)/crtbegin.o $*.o \
		-lgcc -L$(LIB2) -L$(LIB1) -lc -lm -lpthread \
		$(LIB2)/crtend.o $(LIB1)/crtn.o

.o.map :
	$(LD) -dynamic-linker /lib/ld-linux.so.2 -o $* -M \
		$(LIB1)/crt1.o $(LIB1)/crti.o $(LIB2)/crtbegin.o $*.o \
		-lgcc -L$(LIB2) -L$(LIB1) -lc -lm -lpthread \
		$(LIB2)/crtend.o $(LIB1)/crtn.o > $*.map


all : udefrag

console.a :
	cd console; make -f console.spc console.a

udefrag.a :
	cd dll/udefrag; make -f udefrag.spc udefrag.a

zenwinx.a :
	cd dll/zenwinx; make -f zenwinx.spc zenwinx.a

wincalls.a :
	cd wincalls; make -f wincalls.spc wincalls.a

clean :
	cd console; make -f console.spc clean
	cd dll/udefrag; make -f udefrag.spc clean
	cd dll/zenwinx; make -f zenwinx.spc clean
	cd wincalls; make -f wincalls.spc clean

udefrag : $(A)
	$(LD) -dynamic-linker /lib/ld-linux.so.2 -o udefrag -M \
		$(LIB1)/crt1.o $(LIB1)/crti.o $(LIB2)/crtbegin.o \
		console/console.a dll/udefrag/udefrag.a dll/zenwinx/zenwinx.a \
		wincalls/wincalls.a $(NTFS)/libntfs-3g/sparclibntfs.a \
		-lgcc -L$(LIB2) -L$(LIB1) -lc -lm -lpthread \
		$(LIB2)/crtend.o $(LIB1)/crtn.o > udefrag.map
