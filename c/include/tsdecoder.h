#ifndef __TSDECODER_H
#define __TSDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    int pid;
	char* filename;
	FILE fo;
    long previous_buffer_length;
    long position;
    bool start_found;
    bool end_reached;
} WString;
void tsdecoder_new(char* filename, int pid);

#ifdef __cplusplus
}
#endif

#endif
