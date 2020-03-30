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

H=udefrag-internals.h udefrag.h \
   ../../include/compiler.h ../../include/linux.h ../../include/ultradfgver.h \
   ../zenwinx/zenwinx.h ../zenwinx/ntndk.h

O= analyze.o defrag.o map.o move.o optimize.o options.o \
	reports.o search.o tasks.o udefrag.o volume.o 

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

all : udefrag.lib

analyze.o : $(H) analyze.c

defrag.o : $(H) defrag.c

#int64.o : $(H) int64.c

map.o : $(H) map.c

move.o : $(H) move.c

optimize.o : $(H) optimize.c

options.o : $(H) options.c

reports.o : $(H) reports.c

search.o : $(H) search.c

tasks.o : $(H) tasks.c

udefrag.o : $(H) udefrag.c

volume.o : $(H) volume.c

clean :
	rm -f $(O)
	rm -f *.s *.asm *.l *.map

udefrag.lib : $(O)
	rm -f udefrag.lib
	$(WLIB) -r udefrag.lib $(O)
