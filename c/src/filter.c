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
#include "../include/mysocket.h"

#define ARG_BUFF_SIZE    1024
#define FILTER_BUFF_SIZE 4096
#define HTTPGET_SIZE 2048   // should be longer than ARG_BUFF_SIZE (because host_name[ARG_BUFF_SIZE] is placed into str[HTTPGET_SIZE])


void handle_sig(int sig) {
	if (sig == SIGINT) {
		printfdbg("Catched SIGINT...");
	} else if (sig == SIGPIPE) {
		printfdbg("Catched SIGPIPE...");
	} else {
		printfdbg("Catched signal [%d]", sig);
	}
    close(sock2server);
	printfdbg("done. Bye!");
	exit(0);
}

void print_usage(char **args) {
    printf(_("Usage:\n%s -h     prints this.\n"), args[0]);
    printf(_("%s -v     enables verbose mode(i.e. printing debug messages).\n"), args[0]);
    printf(_("%s -f     file to read).\n"), args[0]);
    //printf(_("%s -u     URI to read (HTTP only).\n"), args[0]);
    printf(_("%s -u     Hostname to read (HTTP only).\n"), args[0]);
    printf(_("%s -p     Port.\n"), args[0]);
	printf("\n");
}

int detect_http_header(char* buff, int len) {
    bool http = FALSE;
    char* s;
    int i;
    s = buff;
    for (i = 0; i < len; i++) {
        if (http) {
            if (strncmp(s, "\r\n\r\n", 4) == 0) {
                printfdbg("Payload found at %d byte.", i);
                return i + 4;
            }
            if (strncmp(s, "\n\n", 2) == 0) {
                printfdbg("Payload found at %d byte.", i);
                return i + 2;
            }
        }
        if (strncmp(s, "HTTP", 4) == 0) {
            printfdbg("HTTP found.");
            http = TRUE;
        }
        s++;
        //printfdbg("%02x %02x | %s", s[0], s[1], s);
    }
    return FALSE;
}

void filter(char* filename) {
    FILE *fo;
    size_t one_element = 1;
    int i;
    char buff[FILTER_BUFF_SIZE];

    if ( (fo = fopen(filename, "rb")) == NULL) {
        printferr(_("Cannot open file [%s] for reading."), filename);
        return;
    }
    if (fread(buff, FILTER_BUFF_SIZE, one_element, fo) != one_element) {
        printferr(_("Cannot read file. Aborting."));
        fclose(fo);
        return;
    }
    // find within 4k HTTP data (payload) begin.
    int payload = detect_http_header(buff, FILTER_BUFF_SIZE);
    if (payload == FALSE) {
        fclose(fo);
        return;
    }
    fseek(fo, 0, SEEK_SET);
    fseek(fo, i + 2, SEEK_SET);
    while (fread(buff, FILTER_BUFF_SIZE, one_element, fo) == one_element) {
        for (i = 0; i < FILTER_BUFF_SIZE; i++) putchar(buff[i]);
    }
    fclose(fo);
}

int filter_socket(char* host, int port) {
    int result;
    int i;
    char buff[FILTER_BUFF_SIZE];
    char http_req[HTTPGET_SIZE];

	result = sock_init(host, port, &sock2server);
	if (result != OK) {
		printferr("sock_init() error!");
        close(sock2server);
		return result;
	}
    // HTTP GET request
    // GET /path/file.html HTTP/1.1\n
    // Host: www.host1.com:80\n\n
    sprintf(http_req, "GET / HTTP/1.1\nHost: %s:%d\n\n", host, port);
    sock_write(http_req);

    result = sock_read(buff, FILTER_BUFF_SIZE);
    if (result != OK) {
        printferr("Problem reading socket. %d", result);
        close(sock2server);
        return result;
    }
    // find within 4k HTTP data (payload) begin.
    int payload = detect_http_header(buff, FILTER_BUFF_SIZE);
    if (payload == FALSE) {
        close(sock2server);
        printferr("No http data found (%d)", payload);
        return payload;
    }
    // output remaining data from buffer
    for (i = payload; i < FILTER_BUFF_SIZE; i++) {
        putchar(buff[i]);
    }
    result = sock_read(buff, FILTER_BUFF_SIZE);
    while (result == OK) {
        for (i = 0; i < FILTER_BUFF_SIZE; i++) putchar(buff[i]);
        result = sock_read(buff, FILTER_BUFF_SIZE);
    }

    close(sock2server);
    return result;
}

int main(int arg_count, char **args) {
    int c;
    int port = -1;
    bool fset, hset, pset;
    char host_name[ARG_BUFF_SIZE];
    char file_name[ARG_BUFF_SIZE];

    host_name[0]='\0';
    file_name[0]='\0';
    fset = hset = pset = FALSE;
    opterr = 0;
    while ( (c = getopt(arg_count, args, "hvf:u:p:")) != -1 ) {
        switch (c) {
            case 'v':
                dbglib_set_verbose(1);
                printfdbg(_("Verbose mode enabled.\n"));
                break;
            case 'p':
                port = atoi(optarg);
                printfdbg("Port set to %d", port);
                pset = TRUE;
                break;
            case 'f':
                if (strlen(optarg) >= ARG_BUFF_SIZE) {
                    printferr(_("Filename is too long. Cannot be longer than %d bytes."), ARG_BUFF_SIZE-1);
                    return EXIT_ARGS;
                }
                strcpy(file_name, optarg);
                file_name[ strlen(optarg) ] = '\0'; // better safe than sorry ;-)
                printfdbg("Filename set to %s", file_name);
                fset = TRUE;
                break;
            case 'u':
                if (strlen(optarg) >= ARG_BUFF_SIZE) {
                    printferr(_("Host name is too long. Cannot be longer than %d bytes."), ARG_BUFF_SIZE-1);
                    return EXIT_ARGS;
                }
                strcpy(host_name, optarg);
                host_name[ strlen(optarg) ] = '\0'; // better safe than sorry ;-)
                printfdbg("Host set to %s", host_name);
                hset = TRUE;
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

    if (arg_count == 1) {
        print_usage(args);
        return 0;
    }

    // signal handling
	signal(SIGINT, handle_sig);
	signal(SIGPIPE, handle_sig);

    if (fset) {
        printfdbg(_("Processing file [%s]"), file_name);
        filter(file_name);
    } else if (pset && hset) {
        printfdbg(_("Processing host [%s] , port %d"), host_name, port);
        return filter_socket(host_name, port);
    } else {
        printf(_("Please specify either -f filename parameter or \n-u hostname -p port parameters, otherwise I've no idea what to do."));
        return EXIT_ARGS;
    }
	return EXIT_SUCCESS;
}
