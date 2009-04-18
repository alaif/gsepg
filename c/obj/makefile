#
# This is Getstream EPG decoder Makefile.
#

DEBUG = -g -ggdb #-D__TESTING__ #-DTRACEPRN_ENABLED
FORK = -D__FORK_MODE__ 
THRFLAGS =  -D__USE_REENTRANT
THRLIB = -lpthread 
#LIBS= -lm
LIBS=-ljson
CC = cc -fexec-charset=utf-8 -Wall $(DEBUG)
CC_MODULE = $(CC) -c

# Project (builds all executables)
project: epgextract httpfilter

# Executable and object files for epgextract utility.
EXE_FILTER=gsepg-filter
FILTER_OBJECTS=dbglib.o wstring.o mysocket.o filter.o
httpfilter: $(FILTER_OBJECTS) 
	$(CC) $(FILTER_OBJECTS) -o $(EXE_FILTER)

# Executable and object files for epgextract utility.
EXE_EPGEXTRACT=gsepg-extract
EPGEXTRACT_OBJECTS=dbglib.o tsdecoder.o eitdecoder.o dvbchar.o bitoper.o crc32.o extract.o
epgextract: $(EPGEXTRACT_OBJECTS) 
	$(CC) $(EPGEXTRACT_OBJECTS) -o $(EXE_EPGEXTRACT) $(LIBS)

# Run debugger CGDB
extdebug:
	cgdb $(EXE_EPGEXTRACT)

fildebug:
	cgdb $(EXE_FILTER)

# Internationalization

i18n_template:
	mkdir locale; xgettext -d gsepg -s -o locale/gsepg.pot --language=C --from-code=UTF-8 --add-comments=/// --keyword=_ ../src/extract.c

i18n_po:
	 msginit -i locale/gsepg.pot -o locale/gsepg_cs.po -l cs; echo -n "\n\nNow should be translation into czech done. Then run make i18n_install.\n\n"

i18n_install:
	mkdir -p ./locale/cs/LC_MESSAGES; cp locale/gsepg_cs.mo locale/cs/LC_MESSAGES/

# GSEPG suite modules, programs

dbglib.o: ../src/dbglib.c
	$(CC_MODULE) ../src/dbglib.c

bitoper.o: ../src/bitoper.c ../include/dbglib.h ../include/common.h
	$(CC_MODULE) ../src/bitoper.c

mysocket.o: ../src/mysocket.c ../include/wstring.h
	$(CC_MODULE) ../src/mysocket.c

crc32.o: ../src/crc32.c
	$(CC_MODULE) ../src/crc32.c

wstring.o: ../src/wstring.c
	$(CC_MODULE) ../src/wstring.c

tsdecoder.o: ../include/tsdecoder.h ../src/tsdecoder.c ../include/bitoper.h ../include/dbglib.h ../include/common.h
	$(CC_MODULE) ../src/tsdecoder.c

eitdecoder.o: ../include/eitdecoder.h ../src/eitdecoder.c ../include/bitoper.h ../include/dbglib.h ../include/common.h \
../include/crc32.h ../include/dvbchar.h
	$(CC_MODULE) ../src/eitdecoder.c

dvbchar.o: ../src/dvbchar.c ../include/dvbchar.h
	$(CC_MODULE) ../src/dvbchar.c

extract.o:	../src/extract.c ../include/dbglib.h ../include/common.h ../include/tsdecoder.h \
../include/eitdecoder.h 
	$(CC_MODULE) ../src/extract.c

filter.o:	../src/filter.c ../include/dbglib.h ../include/common.h 
	$(CC_MODULE) ../src/filter.c


# Clean all object files and executables. 
clean:
	rm $(FILTER_OBJECTS) $(EXE_FILTER) \
	$(EPGEXTRACT_OBJECTS) $(EXE_EPGEXTRACT) \
	>/dev/null 2>&1; echo "Cleaned."
