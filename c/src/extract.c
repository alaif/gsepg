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


//static int variable;


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


int main(int arg_count, char **args) {
    // signal handling
	signal(SIGINT, handle_sig);
	signal(SIGPIPE, handle_sig);
    // i18n
    setlocale (LC_ALL, "");
    bindtextdomain(I18N_PACKAGE, I18N_LOCALEDIR);
    textdomain(I18N_PACKAGE);

    if (arg_count == 0) {
        print_usage(args);
        return 0;
    }

    opterr = 0;
    int c;
    while ( (c = getopt(arg_count, args, "vh")) != -1 ) {
        switch (c) {
            case 'v':
                dbglib_set_verbose(1);
                printfdbg(_("Verbose mode enabled.\n"));
                break;
            case 'h':
                print_usage(args);
                return 0;
            case '?':
                if (optopt == 'X')
                   printferr(_("Option -%c requires an argument.\n"), optopt);
                else
                   printferr(_("Unknown option character `\\x%x'.\n"), optopt);
                return EXIT_ARGS;
            default:
                return EXIT_ARGS;
        }
    }
    if (optind >= arg_count) { //there are unprocessed args i.e. filename
        printferr(_("Filename missing."));
        return EXIT_ARGS_FILE;
    }
	printfdbg(_("Processing file %s"), args[optind]);
	return EXIT_SUCCESS;
}


// EOF
