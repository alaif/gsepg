/*
 * mysocket.c
 *
 * Jonas Fiala, 2007
 *
 * Modul pro praci se sockety, umoznuje praci s jednim pripojenim (pro jednoduchost). 
 *
 */

#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/dbglib.h"
#include "../include/mysocket.h"
#include "../include/common.h"
#include "../include/wstring.h"

int sock2server = 0;

/**
 * Inicializuje socket pripojujici se na TCP server.
 * Deskriptor otevreneho socketu se vraci odkazem
 * parametrem socksrv.
 */
int sock_init(char *host, int port, int *socksrv) {
	// zahajeni socketovych operaci
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		printferr(_("Cannot create socket!"));
		return 2;
	}
	struct hostent *hostip = gethostbyname(host);
	if (!hostip) {
		printferr(_("Cannot translate hostname [%s]"), host);
		return 3;
	}
	struct sockaddr_in cl_addr;
	cl_addr.sin_family = AF_INET;
	cl_addr.sin_port = htons( port );
	cl_addr.sin_addr = *(struct in_addr*) hostip->h_addr_list[0];
	if ( connect(sock, (struct sockaddr*) &cl_addr, sizeof(cl_addr)) < 0 ) {
		printferr(_("Cannot establish connection to server!"));
		return 4;
	}
	unsigned int lsa = sizeof( cl_addr );
	// ziskani vlastni identifikace
	getsockname(sock, (struct sockaddr*) &cl_addr, &lsa);
	printfdbg("My IP: '%s'  port: %d",
		inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
	// ziskani informaci o serveru
	getpeername(sock, (struct sockaddr*) &cl_addr, &lsa);
	printfdbg("Server IP: '%s'  port: %d",
		inet_ntoa(cl_addr.sin_addr), ntohs(cl_addr.sin_port));
	printfdbg("Connection established.");
	*socksrv = sock;
	return OK;
}


/**
 * Inicializace UDP socketu serveroveho.
 */
int sock_udp_server_init(int port, int *socksrv) {
	int sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		printferr("Cannot create socket!");
		return 2;
	}
	struct sockaddr_in clAddr;
	clAddr.sin_family = AF_INET;
	clAddr.sin_port = htons( port );
	clAddr.sin_addr.s_addr = INADDR_ANY;
  if ( bind(sock, (struct sockaddr *) &clAddr, sizeof(clAddr)) == -1 ) {
    printferr("bind() failed with error %d\n", errno);
    close(sock);
		return 4;
  }

	*socksrv = sock;
	return OK;
}


/**
 * host ... server IP address
 */
int sock_udp_client_init(char *host, int port, int *socksrv, struct sockaddr_in *servA) {
	int sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		printferr("Cannot create socket!");
		return SOCK_CREATE_ERROR;
	}
	struct hostent *hostip = gethostbyname(host);
	if (!hostip) {
		printferr("Cannot translate hostname [%s]", host);
		return SOCK_HOSTNAME_ERROR;
	}
	struct sockaddr_in clAddr, servAddr;
	clAddr.sin_family = AF_INET;
	clAddr.sin_port = htons( 0 ); // local port number assigned by OS
	clAddr.sin_addr.s_addr = INADDR_ANY;
  if ( bind(sock, (struct sockaddr *) &clAddr, sizeof(clAddr)) == -1 ) {
    printferr("bind() failed with error %d\n", errno);
    close(sock);
		return SOCK_BIND_ERROR;
  }

  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(port);
  servAddr.sin_addr = *(struct in_addr*) hostip->h_addr_list[0];
	*servA = servAddr;
	*socksrv = sock;
	return EXIT_SUCCESS;
}


// kod nasledujici procedury prevzat z http://www.kegel.com/dkftpbench/nonblocking.html
void sock_nonblocking(int fd) {
	int flags;
	// If they have O_NONBLOCK, use the Posix way to do it 
#if defined(O_NONBLOCK)
	// Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. 
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
			flags = 0;
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	// Otherwise, use the old way of doing it
	flags = 1;
	ioctl(fd, FIOBIO, &flags);
#endif
}

/**
 * Zapise do socketu data.
 */
void sock_write(char *buff) {
	int l;
	l = write( sock2server, buff, strlen(buff) );
	if ( l < 0 ) {
			printfdbg("Cannot write to socket.");
	} else {
			printfdbg("Sent %d bytes to server.", l);
	}
}

/**
 * Procedura precte ze socketu max buf_len. Jako parametr obdrzi buffer pro cteni dat.
 * Ceka na naplneni bufferu a pak se ukonci.
 */
int sock_read(char *buff, int buf_len) {
	fd_set read_wait_set;
	struct timeval timeout;
	int res;
    int bytes_to_read;
    char data[SOCK_BUFFER_SIZE];
    char tmp[100 * SOCK_BUFFER_SIZE]; //FIXME
    int tmp_len = 0;
    char* act_tmp;

    memset(buff, '\0', buf_len);
	FD_ZERO( &read_wait_set );
	FD_SET( sock2server, &read_wait_set );
	timeout.tv_sec = SOCK_TIMEOUT_SEC;
	timeout.tv_usec= SOCK_TIMEOUT_USEC;
    bytes_to_read = SOCK_BUFFER_SIZE;

	while (1) {
		res = select( FD_SETSIZE, &read_wait_set, NULL, NULL, &timeout );
		if (res < 0) {
			printferr("select() returned error!");
			return SOCK_SELECT_ERROR;
		}
		if (res == 0) {
			printferr("Receive timeout.");
			return SOCK_TIMEOUT;
		}
		if ( FD_ISSET(sock2server, &read_wait_set) ) {
			// cteni dat ze serveru
			int l = read( sock2server, data, bytes_to_read );
			if (l < 0) {
				printfdbg("Cannot read from socket.");
				break;
			} else if (l > 0) {
				// pridat prijata data do bufferu
                act_tmp = &tmp[tmp_len];
                memcpy(act_tmp, data, l);
                tmp_len += l;
				printfdbg("Appending to buff %d bytes. tmp->len = %d", l, tmp_len);
                if (tmp_len + SOCK_BUFFER_SIZE >= buf_len) {
                    bytes_to_read = buf_len - tmp_len;
                    printfdbg("Last %d bytes", bytes_to_read);
                }
                if (bytes_to_read <= 0) break;
			}
		}
	}
    // copy data to buffer
    printfdbg("Copying %d bytes to buff.", tmp_len);
    memcpy(buff, tmp, tmp_len);
    return OK;
}

// EOF
