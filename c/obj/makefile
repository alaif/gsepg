#
# Makefile pro 2. semestralni praci do predmetu Operacni Systemy.
#

DEBUG = -g -ggdb #-D__TESTING__ #-DTRACEPRN_ENABLED
FORK = -D__FORK_MODE__ 
THRFLAGS =  -D__USE_REENTRANT
THRLIB = -lpthread 
CC = cc -Wall $(DEBUG)
CC_MODULE = $(CC) -c
# Executable names (output)
EXE_EPGEXTRACT=gsepg-extract

OBJECTS=dbglib.o \
		extract.o

project: $(OBJECTS) 
	$(CC) $(OBJECTS) -o $(EXE_EPGEXTRACT)

dbglib.o: ../src/dbglib.c
	$(CC_MODULE) ../src/dbglib.c

wstring.o: ../src/wstring.c
	$(CC_MODULE) ../src/wstring.c

extract.o:	../src/extract.c ../include/dbglib.h ../include/common.h
	$(CC_MODULE) ../src/extract.c


# --- clean ---
clean:
	rm $(OBJECTS) $(EXE_EPGEXTRACT)
