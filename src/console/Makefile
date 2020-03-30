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
LD=ld
AR=ar
INCL=-I$(NTFS) -I$(NTFS)/include/ntfs-3g -I/usr/include \
     -I../include -I../dll/zenwinx
COPT=-DLXGC=1 -O2
GCCOPT=-DLXGC=1 -O2
NTFS=/usr/include/ntfs-3g

H=../include/ultradfgver.h udefrag.h \
         ../include/compiler.h ../include/linux.h

O=defrag.o map.o options.o

.c.s :
	$(GCC) $(COPT) $(INCL) -S $*.c

.cpp.o :
	$(GCC) $(GCCOPT) $(INCL) -c $*.cpp

.c.o :
	$(GCC) $(COPT) $(INCL) -c $*.c

.cpp.s :
	$(GCC) $(GCCOPT) $(INCL) -S $*.cpp

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
