/*
 * extract.c
 *
 * Jonas Fiala, EPG information extractor.
 *
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
#include <errno.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/dbglib.h"
#include "../include/common.h" // common macros and constants
#include "../include/tsdecoder.h"
#include "../include/eitdecoder.h"

char output_filename[1024];
bool output_opened = FALSE;
bool no_output = FALSE;
FILE *output_fp;
bool abort_program = FALSE;
bool abort_file = FALSE;
char pid_filename[1024];


void print_usage(char **args) {
    printf(_("Usage: %s filename\n"), args[0]);
    printf(_("filename is mandatory. From this file input is read. Special filename is - for stdin\n\n"));
    printf(_("%s -h     prints this.\n"), args[0]);
    printf(_("%s -v     enables verbose mode(i.e. printing debug messages).\n"), args[0]);
    printf(_("%s -o     output file. Use - for stdout (default value).\n"), args[0]);
    printf(_("%s -n     no output (suitable for debugging purposes together with -v parameter).\n"), args[0]);
    printf(_("%s -p     PID file path (optional).\n"), args[0]);
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
    if (output_opened) {
        printfdbg("Output file closed.");
        fclose(output_fp);
    }
	printfdbg("done. Bye!");
	exit(0);
}

// Handles SIG_USR1. Finishes output (valid) and end execution as soon as possible.
void handle_usr(int sig) {
    eitdecoder_trigger_abort();
    abort_program = TRUE;
}

// Handles SIG_USR2. Restarts output.
void handle_reload(int sig) {
    eitdecoder_trigger_abort();
    abort_file = TRUE;
}

void decode(char *filename) {
    transport_stream tab_ts;
    transport_stream* ts = &tab_ts;
    ts_packet packetts;
    ts_packet* pac = &packetts;
    bool result;
    bool eit_result;
    // move abort variables to suitable states
    abort_file = abort_program = FALSE;

    result = tsdecoder_init(ts, filename, EPG_GETSTREAM_PID);
    if (!result) {
        printferr("TS decoder init failed.");
        return;
    }
   
    // initialize eitdecoder module (useful esp. when using SIG_USR2 to restart output)
    eitdecoder_init();
    // set output stream (file, stdout) by command line argument!
    if (strlen(output_filename) > 0) {
        if (strcmp("-", output_filename) == 0) {
            eitdecoder_set_output_stream(stdout); 
            output_opened = FALSE;
        } else {
            output_fp = fopen(output_filename, "w");
            eitdecoder_set_output_stream(output_fp); 
            output_opened = TRUE;
        }
    } else if (!no_output) {
        eitdecoder_set_output_stream(stdout); 
        output_opened = FALSE;
    }
    // top level JSON dict
    eitdecoder_output("{\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);
    eitdecoder_output("\"data\": [\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);
    // find all EIT headers
    while ( (result = tsdecoder_get_packet(ts, pac)) == TRUE ) {
        if ( !eitdecoder_detect_eit(pac) ) 
            continue;
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
            abort_program = TRUE;
            break;
        } else if (abort_program || abort_file) {
            printfdbg("Aborting.");
            break;
        }
        eitdecoder_output(",\n");
        /*printf("EIT:");
        for (i = 0; i < TSPACKET_PAYLOAD_SIZE; i++) printf("%c", pac->payload[i]);
        printf("\n");*/
    }
    // end of top level JSON dict
    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("]\n");
    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("}\n");
    //tsdecoder_print_packets(ts);
    if (output_opened)
        fclose(output_fp);
}

void create_pid_file() {
    if (strlen(pid_filename) <= 0) 
        return;
    FILE *f;
    f = fopen(pid_filename, "w");
    if (f == NULL) {
        printferr("Problem opening PID file. Please check pid filename argument is valid path and/or permissions.");
        printferr("Errno: %d. %s", errno, strerror(errno));
        exit(EXIT_ARGS);
    }
    pid_t my_pid = getpid();
    char pid_str[50];
    sprintf(pid_str, "%d\n", my_pid);
    fwrite(pid_str, strlen(pid_str), 1, f);
    fclose(f);
}

int main(int arg_count, char **args) {
    // signal handling
	signal(SIGINT, handle_sig);
	signal(SIGPIPE, handle_sig);
	signal(SIGUSR1, handle_usr);
	signal(SIGUSR2, handle_reload);
    // i18n
    setlocale (LC_ALL, "");
    bindtextdomain(I18N_PACKAGE, I18N_LOCALEDIR);
    textdomain(I18N_PACKAGE);

    if (arg_count == 0) {
        print_usage(args);
        return 0;
    }

    opterr = 0;
    output_filename[0] = '\0';
    pid_filename[0] = '\0';
    int c;
    while ( (c = getopt(arg_count, args, "p:vho:n")) != -1 ) {
        switch (c) {
            case 'v':
                dbglib_set_verbose(1);
                printfdbg(_("Verbose mode enabled.\n"));
                break;
            case 'h':
                print_usage(args);
                return 0;
            case 'o':
                if (strlen(optarg) > 1023) {
                    printferr(_("filename is too long."));
                    return EXIT_ARGS;
                }
                strcpy(output_filename, optarg);
                break;
            case 'n':
                no_output = TRUE;
                break;
            case 'p':
                if (strlen(optarg) > 1023) {
                    printferr(_("PID filename is too long."));
                    return EXIT_ARGS;
                }
                strcpy(pid_filename, optarg);
                break;
            case '?':
               printferr(_("Unknown option character '%c'.\n"), optopt);
                return EXIT_ARGS;
            default:
                return EXIT_ARGS;
        }
    }
    if (optind >= arg_count) { //there are unprocessed args i.e. filename
        printferr(_("Filename to read missing. Use %s -h  to display usage."), args[0]);
        return EXIT_ARGS_FILE;
    }
    create_pid_file();
	printfdbg(_("Processing file %s"), args[optind]);
    while (!abort_program) 
        decode(args[optind]);
	return EXIT_SUCCESS;
}


// EOF
