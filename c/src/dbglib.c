/*
   dbglib.c

   Knihovna s funkcemi ladicich a chybovych vypisu.
*/   

#include <stdarg.h>
#include <stdio.h>
#include <time.h>


static int verbose = 0; //verbose flag (podrobne vypisy o cinnosti programu na stdout)


void dbglib_set_verbose(int v) {
	if (v > 1 || v < 0) {
		return;
	}
	verbose = v;
}


static void print_now(FILE* descriptor) {
	struct tm *actual;
	time_t tnow;
	time(&tnow);
	actual = localtime(&tnow);
	fprintf(descriptor, "%02d:%02d:%02d ", actual->tm_hour, actual->tm_min, actual->tm_sec);
}

/** debug hlasky na std. vystup. Bezi pri zapnutem flagu verbose. */
void printfdbg(const char *format, ...) {
	va_list ap; 
	if (verbose) {
		print_now(stdout);
	}
	va_start(ap, format);
	if (verbose) {
		vfprintf(stdout, format, ap);
	}
	va_end(ap); 
	if (verbose) {
		printf("\n");
	}
}


/** chybovy vypis (stderr) */
void printferr(const char *format, ...) {
	va_list ap; 
	print_now(stderr);
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap); 
	printf("\n");
}
