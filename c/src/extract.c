/*
   main.c

   Jonas Fiala

*/


#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <libintl.h>
#include <locale.h>
#include "../include/dbglib.h"
#include "../include/common.h" // spolecna makra a datove struktury pro klienta i server


//static int client_ids[MAX_CLIENTS];


void print_usage(char **args) {
    printf(_("Usage:\n%s -h     prints this.\n"), args[0]);
    printf(_("%s -v     enables verbose mode(i.e. printing debug messages).\n\n"), args[0]);
	printf("\n");
}



void handle_sig(int sig) {
	if (sig == SIGINT) {
		printfdbg("Catched SIGINT...");
	} else if (sig == SIGPIPE) {
		printfdbg("Catched SIGPIPE...");
	} else {
		printfdbg("Catched signal [%d]", sig);
	}
	printfdbg("done. Bye!");
	exit(0);
}


int main(int argCount, char **args) {
	signal(SIGINT, handle_sig);
	signal(SIGPIPE, handle_sig);
    // i18n
    setlocale (LC_ALL, "");
    bindtextdomain(I18N_PACKAGE, I18N_LOCALEDIR);
    textdomain(I18N_PACKAGE);

	if (argCount > 1) {
		if (strcmp(args[1], "-v") == 0 || strcmp(args[1], "--verbose") == 0) {
			dbglib_set_verbose(1);
			printfdbg(_("Verbose mode enabled.\n"));
		} else if (strcmp(args[1], "-h") != 0 || strcmp(args[1], "--help") != 0) {
			//print usage
            print_usage(args);
			return 0;
		} else {
			print_usage(args);
			return 0;
		}
	}

	printfdbg(_("main_loop() started"));
	return 0;
}


// EOF
