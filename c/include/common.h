#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#define FD_STDIN     0
#define FD_STDOUT    1
#define MESSAGE_SIZE     1024
#define CLIENT_NAME_SIZE 50
// hlasky a prikazy:
#define THANKS  "Diky za chatovani, ted te odpojim.\n"
#define WELCOME "Vitej v chatu!\n\nPro odeslani zpravy ostatnim v mistnosti,\nstaci napsat zpravu a stisknout enter.\n"
#define CMD_QUIT "/quit"

typedef struct {
	char message[MESSAGE_SIZE];
	int shutdown;
} shared_data;


#ifdef __cplusplus
}
#endif

#endif
