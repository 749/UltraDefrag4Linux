#
#         Plain general makefile, for ultradefrag compilations 
#         Host Windows, target Windows 32-bit
#

# restrict suffixes list to the ones we define
.SUFFIXES :
# this list controls the ordering of rule evaluation
.SUFFIXES : .c .cpp .asm .o .O .l .obj .exe . .dll .map

# cancel implicit rule for building .c from .l (lex file) :
%.c : %.l

# cancel implicit rule for building target from .c
% : %.c

# cancel implicit rule for building . from .s
% : %.s

HCC=hcc86
TOP=top86
ASM=asm86
LMX=lmx
WLIB=wlib

LIB=D:\c-src\lib

# use of Borland library
#INCL=-ID:\c-src\include
#DEFS=-D_RTLDLL;WNSC
#PDEFS=-D_RTLDLL;WNBC=1
#OLIB=D:\c-src\lib\c0x32.obj
#DLIB=D:\c-src\lib\c0d32.obj
#ILIB=D:\c-src\lib\cw32i.lib

# use of Microsoft library
INCL=-ID:\c-src\msvcrt -ID:\c-src\include -I..\..\include
DEFS="-D_RTLDLL;WNSC"
PDEFS="-D_RTLDLL;WNBC=1"
OLIB=D:\c-src\msvcrt\c0ms.obj
DLIB=D:\c-src\msvcrt\c0msd.obj
ILIB=D:\c-src\msvcrt\msvcrt.lib

H= ../../include/compiler.h ../../include/linux.h  ../../include/ultradfgver.h \
   ../udefrag/udefrag.h \
   zenwinx.h ntndk.h ntfs.h

O=dbg.o env.o event.o file.o ftw.o ftw_ntfs.o keyboard.o keytrans.o \
	ldr.o list.o lock.o mem.o misc.o path.o prb.o privilege.o reg.o \
	stdio.o string.o thread.o time.o volume.o zenwinx.o

#                  32-bit mode (Sozobon C)

.c.asm :
	$(HCC) -A16S $(DEFS) $(INCL) $*.c > $*.asm

.asm.o :
	$(ASM) -lk3 -o$*.o $*.asm

.c.o :
	$(HCC) -A16S $(DEFS) $(INCL) $*.c | $(TOP) - $*.o

# not optimized and no-pipe (so usable under ms-dos)
.c.O :
	$(HCC) -A16S -DWNSC $(INCL) $*.c > $*.asm
	$(ASM) -k3 -o$*.o $*.asm
	rm $*.asm

.c.l :
	$(HCC) -A16S $(DEFS) $(INCL) $*.c | $(TOP) -lv - $*.o > $*.l

# not optimized and no-pipe (so usable under ms-dos)
.c.L :
	$(HCC) -A16S $(DEFS) $(INCL) $*.c > $*.asm
	$(ASM) -lk3 -o$*.o $*.asm
	rm $*.asm

.o. .o :
	$(LMX) $(OLIB) $*.o, $*., $*, $(LIB)\lmlib.lib $(ILIB) $(LIB)\import32.lib,
	chmod 755 $*

.o.exe :
	$(LMX) $(OLIB) $*.o, $*.exe, $*, $(LIB)\lmlib.lib $(ILIB) $(LIB)\import32.lib,
	chmod 755 $*.exe

.o.map :
	$(LMX) $(OLIB) $*.o, $*., $*/map, $(LIB)\lmlib.lib $(ILIB) $(LIB)\import32.lib,
	chmod 755 $*

.o.dll :
	$(LMX) $(DLIB) $*.o, $*.dll, $*/dll/map, $(LIB)\lmlib.lib $(ILIB) $(LIB)\import32.lib,

all : zenwinx.lib

dbg.o dbg.asm dbg.l : $(H) dbg.c

env.o env.asm env.l : $(H) env.c

event.o event.asm event.l : $(H) event.c

file.o file.asm file.l : $(H) file.c

ftw.o ftw.asm ftw.l : $(H) ftw.c

ftw_ntfs.o ftw_ntfs.asm ftw_ntfs.l : $(H) ftw_ntfs.c

int64.o int64.asm int64.l : $(H) int64.c

keyboard.o keyboard.asm keyboard.l : $(H) keyboard.c

keytrans.o keytrans.asm keytrans.l : $(H) keytrans.c

ldr.o ldr.asm ldr.l : $(H) ldr.c

list.o list.asm list.l : $(H) list.c

lock.o lock.asm lock.l : $(H) lock.c

mem.o mem.asm mem.l : $(H) mem.c

misc.o misc.asm misc.l : $(H) misc.c

path.o path.asm path.l : $(H) path.c

prb.o prb.asm prb.l : $(H) prb.c

privilege.o privilege.asm privilege.l : $(H) privilege.c

reg.o reg.asm reg.l : $(H) reg.c

stdio.o stdio.asm stdio.l : $(H) stdio.c

string.o string.asm string.l : $(H) string.c

thread.o thread.asm thread.l : $(H) thread.c

time.o time.asm time.l : $(H) time.c

volume.o volume.asm volume.l : $(H) volume.c

zenwinx.o zenwinx.asm zenwinx.l : $(H) zenwinx.c

clean :
	rm -f $(O)
	rm -f *.s *.asm *.l *.map

zenwinx.lib : $(O)
	rm -f zenwinx.lib
	$(WLIB) -r zenwinx.lib $(O)
