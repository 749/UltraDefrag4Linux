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
     -I../../include -I.
COPT=-DLXGC=1 -O2
GCCOPT=-DLXGC=1 -O2
NTFS=/usr/include/ntfs-3g

H= ../../include/compiler.h ../../include/linux.h  ../../include/ultradfgver.h \
   ../udefrag/udefrag.h \
   zenwinx.h ntndk.h ntfs.h

O= dbg.o env.o event.o file.o ftw.o ftw_ntfs.o ldr.o list.o lock.o \
	mem.o misc.o path.o prb.o string.o thread.o time.o volume.o zenwinx.o

.c.s :
	$(GCC) $(COPT) $(INCL) -S $*.c

.cpp.o :
	$(GCC) $(GCCOPT) $(INCL) -c $*.cpp

.c.o :
	$(GCC) $(COPT) $(INCL) -c $*.c

.cpp.s :
	$(GCC) $(GCCOPT) $(INCL) -S $*.cpp

all : zenwinx.a

dbg.o : $(H) dbg.c

env.o : $(H) env.c

event.o : $(H) event.c

file.o : $(H) file.c

ftw.o : $(H) ftw.c

ftw_ntfs.o : $(H) ftw_ntfs.c

int64.o : $(H) int64.c

keyboard.o : $(H) keyboard.c

keytrans.o : $(H) keytrans.c

ldr.o : $(H) ldr.c

list.o : $(H) list.c

lock.o : $(H) lock.c

mem.o : $(H) mem.c

misc.o : $(H) misc.c

path.o : $(H) path.c

prb.o : $(H) prb.c

privilege.o : $(H) privilege.c

reg.o : $(H) reg.c

stdio.o : $(H) stdio.c

string.o : $(H) string.c

thread.o : $(H) thread.c

time.o : $(H) time.c

volume.o : $(H) volume.c

zenwinx.o : $(H) zenwinx.c

clean :
	rm -f $(O)
	rm -f *.s *.asm *.l *.map

zenwinx.a : $(O)
	rm -f zenwinx.a
	$(AR) -rv zenwinx.a $(O)
