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

wstring.o: ../src/wstring.c
	$(CC_MODULE) ../src/wstring.c

extract.o:	../src/extract.c ../include/dbglib.h ../include/common.h
	$(CC_MODULE) ../src/extract.c


# --- clean ---
clean:
	rm $(OBJECTS) $(EXE_EPGEXTRACT)
