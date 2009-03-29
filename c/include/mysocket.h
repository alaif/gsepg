#ifndef __MYSOCKET_H
#define __MYSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

// sockets
#define OK             0
#define SOCK_ERROR         -1
#define SOCK_READ_ERROR    -2
#define SOCK_SELECT_ERROR  -3
#define SOCK_TIMEOUT       -4
#define SOCK_CREATE_ERROR     -5
#define SOCK_HOSTNAME_ERROR   -6
#define SOCK_BIND_ERROR       -7
#define SOCK_BUFFER_SIZE     1024

#define SOCK_TIMEOUT_SEC     30
#define SOCK_TIMEOUT_USEC     0

extern int sock2server;

int sock_init(char *, int, int *);
int sock_udp_server_init(int , int *);
int sock_udp_client_init(char *, int , int *, struct sockaddr_in *);
void sock_nonblocking(int);
void sock_write(char *buff);
int sock_read(char *buff, int buf_len);

#ifdef __cplusplus
}
#endif

#endif
