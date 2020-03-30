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
INCL=-I$(NTFS) -I$(NTFS)/include/ntfs-3g -I$(NTFS)/replace \
     -I/shared/sparc/root/usr/sparc-include -I/shared/c-src/include/linux \
     -I../include -I../dll/zenwinx
COPT=-DSPGC=1 -O2
GCCOPT=-DSPGC=1 -O2
LIB1=/shared/sparc/root/usr/lib
LIB2=/shared/sparc/gcc/lib/gcc/sparc-sun-linux/4.0.0
NTFS=/shared/ntfs/ntfslowprof

H=../include/ultradfgver.h udefrag.h \
         ../include/compiler.h ../include/linux.h

O=defrag.o map.o options.o

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

all : console.a

defrag.o : $(H) defrag.c
                
map.o : $(H) map.c
                
options.o : $(H) options.c

clean :
	rm -f $(O)
	rm -f *.s *.asm *.l *.map

console.a : $(O)
	rm -f console.a
	$(AR) -rv console.a $(O)
