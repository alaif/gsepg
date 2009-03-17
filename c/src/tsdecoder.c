#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/tsdecoder.h"

TransportStream* tsdecoder_new(char* filename, int pid) {
    TransportStream* ts;
    FILE *fo;
    //TODO check file valid path/existence
    fo = fopen(filename, "rb");
	ts = (TransportStream*) malloc(sizeof(TransportStream));
    if (ts == NULL) {
        printferr(_("Memory allocation failed when creating TransportStream."));
        exit(EXIT_MEM);
    }
    ts->pid = pid;
    ts->filename = filename;
    ts->fo = fo;
    ts->buffer[0] = '\0';
    ts->previous_buffer_length = 0;
    ts->position = 0;
    ts->start_found = FALSE;
    ts->end_reached = FALSE;
    return ts;
}

void tsdecoder_free(TransportStream** ts) {
    TransportStream* p_ts = (TransportStream*)*ts;
    fclose(p_ts->fo);
	free((void*) *ts);
	*ts = NULL;
}


TSPacketHeader* tsdecoder_packet_header(TransportStream* ts, TSPacketHeader* header) {
    return NULL;
}
