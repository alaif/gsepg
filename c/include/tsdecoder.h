#ifndef __TSDECODER_H
#define __TSDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    int pid;
	char* filename;
	FILE *fo;
    char buffer[4096];
    long previous_buffer_length;
    long position;
    bool start_found;   // start of readable data (first usable packet)
    bool end_reached;   // end of stream (end of TransportStream->fo file)
} TransportStream;

typedef struct {
    // 2 Bytes
    unsigned int tei    : 1;          // transport error indicator
    unsigned int pusi   : 1;          // payload unit start indicator
    unsigned int tp     : 1;          // transport priority
    unsigned int pid    :13;          // packet id
    // 1 Byte
    unsigned int scram  : 2;          // scrambling control
    unsigned int adapt  : 1;          // adaptation field list
    unsigned int payload: 1;          // payload data exist in packet
    unsigned int continuity: 4;       // packet continuity counter
} TSPacketHeader;

TransportStream* tsdecoder_new(char* filename, int pid);
void tsdecoder_free(TransportStream** ts);
TSPacketHeader* tsdecoder_packet_header(TransportStream* ts, TSPacketHeader* header);

#ifdef __cplusplus
}
#endif

#endif
