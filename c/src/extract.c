/*
   extract.c

   Jonas Fiala, EPG information extractor.

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
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/dbglib.h"
#include "../include/common.h" // common macros and constants
#include "../include/tsdecoder.h"
#include "../include/eitdecoder.h"


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


void decode(char *filename) {
    transport_stream tab_ts;
    transport_stream* ts = &tab_ts;
    ts_packet packetts;
    ts_packet* pac = &packetts;
    bool result;
    bool eit_result;

    result = tsdecoder_init(ts, filename, EPG_GETSTREAM_PID);
    if (!result) {
        printferr("TS decoder init failed.");
        return;
    }

    // find all EIT headers
    while ( (result = tsdecoder_get_packet(ts, pac)) == TRUE ) {
        if ( eitdecoder_detect_eit(pac) ) {
            eitable tab_eit;
            eitable *eit = &tab_eit;
            eitdecoder_table(pac, eit);
            printfdbg(
                    "EIT header: table=%02x section_length=%d, ssi=%d, sec_no=%d, last_sec_no=%d", 
                    eit->table_id, 
                    eit->section_length, 
                    eit->section_syntax_indicator,
                    eit->section_number,
                    eit->last_section_number
            );
            eit_result = eitdecoder_events(ts, pac, eit);
            if (!eit_result) {
                printferr("Problem decoding events, aborting EIT decoding procedure.");
                break;
            }
            /*printf("EIT:");
            for (i = 0; i < TSPACKET_PAYLOAD_SIZE; i++) printf("%c", pac->payload[i]);
            printf("\n");*/
        }
    }
    //tsdecoder_print_packets(ts);
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
    decode(args[optind]);
	return EXIT_SUCCESS;
}


// EOF
