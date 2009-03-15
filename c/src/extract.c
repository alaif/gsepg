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
#include "../include/dbglib.h"
#include "../include/common.h" // spolecna makra a datove struktury pro klienta i server


//static int client_ids[MAX_CLIENTS];


void print_help() {
	printf("Pro vypis ladicich hlasek lze pouzit pri spusteni parametr -v \n\n");
	printf("-f | --fork    spusti server v rezimu podprocesu.\n");
	printf("               jinak server bezi ve vlaknech\n");
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
	if (argCount > 1) {
		if (strcmp(args[1], "-v") == 0 || strcmp(args[1], "--verbose") == 0) {
			dbglib_set_verbose(1);
			printfdbg("Verbose rezim zapnut.\n");
		} else if (strcmp(args[1], "-h") != 0 || strcmp(args[1], "--help") != 0) {
			//print usage
			printf("Pouziti programu:\n%s -h     vypise tuto napovedu.\n", args[0]);
			printf("%s -v     zapne podrobne hlasky o cinnosti programu.\n\n", args[0]);
			return 0;
		} else {
			print_help();
			return 0;
		}
	}

	printfdbg("main_loop() started");
	return 0;
}


// EOF
