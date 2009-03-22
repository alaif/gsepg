#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/tsdecoder.h"

void tsdecoder_print_tsheader(ts_packet_header* p_hdr) {
    printfdbg(
        "TS PH: pid=%d sync=0x%x tei=%d pusi=%d tp=%d scram=%d adapt=%d payload=%d cont=%d", 
        p_hdr->pid, 
        p_hdr->sync,
        p_hdr->tei, 
        p_hdr->pusi,
        p_hdr->tp,
        p_hdr->scram,
        p_hdr->adapt,
        p_hdr->payload,
        p_hdr->continuity
    );
}

void tsdecoder_print_tsheader_adapt(ts_adaptation_field* a_field) {
    printfdbg(
        "TS PH adapt. field: len=%d disc=%d random=%d es_prio=%d pcr=%d opcr=%d splic=%d priv=%d ext=%d",
        a_field->length,
        a_field->discontinuity,
        a_field->random,
        a_field->es_priority,
        a_field->pcr,
        a_field->opcr,
        a_field->splicing_point,
        a_field->private_data,
        a_field->adapt_extension
    );
}

transport_stream* tsdecoder_new(char* filename, int pid) {
    transport_stream* ts;
    FILE *fo;
    //TODO check file valid path/existence
    fo = fopen(filename, "rb");
	ts = (transport_stream*) malloc(sizeof(transport_stream));
    if (ts == NULL) {
        printferr(_("Memory allocation failed when creating transport_stream."));
        exit(EXIT_MEM);
    }
    ts->pid = pid;
    ts->filename = filename;
    ts->fo = fo;
    ts->buffer[0] = '\0';
    ts->previous_buffer_length = 0;
    ts->buffer_length = 0;
    ts->position = 1;
    ts->start_found = FALSE;
    ts->end_reached = FALSE;
    return ts;
}

void tsdecoder_free(transport_stream** ts) {
    transport_stream* p_ts = (transport_stream*)*ts;
    fclose(p_ts->fo);
	free((void*) *ts);
	*ts = NULL;
}

static bool tsdecoder_is_packet_start(char b) {
    return (b == 0x47);
}


bool tsdecoder_packet_header(transport_stream* ts, ts_packet_header* header) {
    size_t status, one_element = 1;
    char buff[TSPACKET_HEADER_SIZE];
    // load packet header
    status = fread(&buff, TSPACKET_HEADER_SIZE, one_element, ts->fo);
    if (status != one_element) {
        return FALSE;
    }
    //printfdbg("Header: %02x %02x %02x %02x", buff[0], buff[1], buff[2], buff[3]);
    header->sync = buff[0];
    header->pid = (buff[1] << 8 | buff[2]) & 0x1fff;
    header->tei = buff[1] & 0x80 ? 1 : 0;
    header->pusi = buff[1] & 0x40 ? 1 : 0;
    header->tp = buff[1] & 0x20 ? 1 : 0;
    header->scram = (buff[3] & 0xc0) >> 6;
    header->adapt = buff[3] & 0x40 ? 1 : 0;
    header->payload = buff[3] & 0x10 ? 1 : 0;
    header->continuity = buff[3] & 0xf;
    return TRUE;
}

bool tsdecoder_packet_header_adapt(transport_stream* ts, ts_adaptation_field* field) {
    size_t status, one_element = 1;
    char buff[TSPACKET_ADAPT_FIELD_SIZE];
    // load adaptation field
    status = fread(&buff, TSPACKET_ADAPT_FIELD_SIZE, one_element, ts->fo);
    if (status != one_element) {
        return FALSE;
    }
    field->length = buff[0];
    field->discontinuity = buff[1] & 0x80 ? 1: 0;
    field->random = buff[1] & 0x40 ? 1: 0;
    field->es_priority = buff[1] & 0x20 ? 1: 0;
    field->pcr = buff[1] & 0x10 ? 1: 0;
    field->opcr = buff[1] & 0x8 ? 1: 0;
    field->splicing_point = buff[1] & 0x4 ? 1: 0;
    field->private_data = buff[1] & 0x2 ? 1: 0;
    field->adapt_extension = buff[1] & 0x1 ? 1: 0;
    return TRUE;
}

void tsdecoder_fill_buffer(transport_stream* ts) {
    ts_packet_header packet_header;
    ts_packet_header* p_hdr = &packet_header;
    char b;
    char payload[TSPACKET_PAYLOAD_SIZE];
    size_t status, byte_len = 1, one_element = 1;
    bool found;
    int i;
    ts->position = 1;
    ts->buffer_length = 0;
    for (i = 0; i < TSPAYLOAD_BUFFER_SIZE; i++) ts->buffer[i] = '\0';
    while ( fread(&b, byte_len, one_element, ts->fo) == one_element ) {
        if ( !tsdecoder_is_packet_start(b) ) continue;
        fseek(ts->fo, -1, SEEK_CUR);
        // load packet header, if unavailable (false) go to next byte
        found = tsdecoder_packet_header(ts, p_hdr);
        if ( !found ) continue;
        status = fread(payload, TSPACKET_PAYLOAD_SIZE, one_element, ts->fo);
        if (status != one_element) break; // no packets left to read (whole packets)
        // don't care scrambled packets, packets without payload (are they usable for reading EPG?)
        if (p_hdr->pid != EPG_GETSTREAM_PID || p_hdr->scram) continue;
        if ( p_hdr->pusi == 0 && !ts->start_found ) continue;
        // adaptation field (optional) is set
        if (p_hdr->adapt == 1) continue;

        tsdecoder_print_tsheader(p_hdr);
        ts->start_found = TRUE;
        //strncat(ts->buffer, payload, TSPACKET_PAYLOAD_SIZE);
        for (i = ts->buffer_length; i < ts->buffer_length + TSPACKET_PAYLOAD_SIZE; i++) {
            ts->buffer[i] = payload[i - ts->buffer_length];
        }
        ts->buffer_length += TSPACKET_PAYLOAD_SIZE;
        // buffer is filled enough
        if (ts->buffer_length + TSPACKET_PAYLOAD_SIZE >= TSPAYLOAD_BUFFER_SIZE) break;
    }
    /*printfdbg("Buffer filled, ts->buffer_length=%d", ts->buffer_length);
    for (i = 0; i < ts->buffer_length; i++) printf("%c", ts->buffer[i]);*/
}

bool tsdecoder_get_byte(transport_stream* ts, char* buff) {
    if (ts->position > ts->buffer_length) {
        tsdecoder_fill_buffer(ts);
    }
    if (ts->position > ts->buffer_length) {
        // no data available at the moment
        ts->end_reached = TRUE;
        return FALSE;
    }
    *buff = ts->buffer[ts->position - 1];
    ts->position++; 
    return TRUE;
}

// Fills-up *buff with payload bytes of TS packets.
// Returns TRUE if bytes were successfuly read otherwise FALSE.
bool tsdecoder_get_data(transport_stream* ts, char* buff, int buff_length) {
    bool result;
    int i;
    for (i = 0; i < buff_length; i++) {
        result = tsdecoder_get_byte(ts, &buff[i]);
        if (!result) return FALSE;
    }
    return TRUE;
}

// Prints out decoded paket headers in order they came from input FILE*.
void tsdecoder_print_pakets(transport_stream* ts) {
    ts_packet_header packet_header;
    ts_packet_header* p_hdr = &packet_header;
    ts_adaptation_field adapt_field;
    ts_adaptation_field* a_field = &adapt_field;
    char b;
    char payload[TSPACKET_PAYLOAD_SIZE];
    size_t status, byte_len = 1, one_element = 1;
    bool found;
    while ( fread(&b, byte_len, one_element, ts->fo) == one_element ) {
        if ( !tsdecoder_is_packet_start(b) ) continue;
        fseek(ts->fo, -1, SEEK_CUR);
        // load packet header, if unavailable (false) go to next byte
        found = tsdecoder_packet_header(ts, p_hdr);
        if ( !found ) continue;
        tsdecoder_print_tsheader(p_hdr);
        // adaptation field (optional)
        if (p_hdr->adapt == 1) {
            status = tsdecoder_packet_header_adapt(ts, a_field);
            tsdecoder_print_tsheader_adapt(a_field);
            status = fread(payload, TSPACKET_PAYLOAD_SIZE - TSPACKET_ADAPT_FIELD_SIZE, one_element, ts->fo);
        } else {
            // as there is no optional field, remaining packet data can be read.
            status = fread(payload, TSPACKET_PAYLOAD_SIZE, one_element, ts->fo);
        }
        // don't care scrambled packets, packets without payload (are they usable for reading EPG?)
        //if (p_hdr->pid != EPG_GETSTREAM_PID || p_hdr->scram) continue;
    }
}
