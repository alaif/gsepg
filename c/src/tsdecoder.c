#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/bitoper.h"
#include "../include/tsdecoder.h"

void tsdecoder_print_tsheader(ts_packet_header* p_hdr) {
    printfdbg(
        "TS PH: pid=%d sync=0x47 tei=%d pusi=%d tp=%d scram=%d adapt=%d payload=%d cont=%d", 
        p_hdr->pid, 
        //p_hdr->sync,
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

bool tsdecoder_init(transport_stream* ts, char* filename, int pid) {
    FILE *fo;
    if (strcmp(filename, "-") == 0) {
        fo = stdin;
    } else if ( (fo = fopen(filename, "rb")) == NULL) {
        printferr("Cannot open file [%s]", filename);
        return FALSE;
    }
    ts->pid = pid;
    ts->filename = filename;
    ts->fo = fo;
    ts->buffer_length = 0;
    ts->position = 1;
    ts->start_found = FALSE;
    ts->end_reached = FALSE;
    return TRUE;
}

void tsdecoder_free(transport_stream** ts) {
    transport_stream* p_ts = (transport_stream*)*ts;
    fclose(p_ts->fo);
	free((void*) *ts);
	*ts = NULL;
}

transport_stream* tsdecoder_new(char* filename, int pid) {
    transport_stream* ts;
	ts = (transport_stream*) malloc(sizeof(transport_stream));
    if (ts == NULL) {
        printferr(_("Memory allocation failed when creating transport_stream."));
        return NULL;
    }
    if (!tsdecoder_init(ts, filename, pid)) {
        tsdecoder_free(&ts);
        return NULL;
    }
    return ts;
}

static bool tsdecoder_is_packet_start(char b) {
    return (b == 0x47);
}


bool tsdecoder_packet_header(transport_stream* ts, ts_packet_header* header) {
    size_t status, one_element = 1;
    char buff[TSPACKET_HEADER_SIZE];
    // load packet header
    status = fread(buff, TSPACKET_HEADER_SIZE, one_element, ts->fo);
    if (status != one_element) {
        return FALSE;
    }
    //printfdbg("Header: %02x %02x %02x %02x", buff[0], buff[1], buff[2], buff[3]);
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, buff, TSPACKET_HEADER_SIZE * 8);
    //header->sync = bitoper_walk_number(bit_op, 8); //disabling this due to fseek problem when reading stdin.
    header->tei = bitoper_walk_number(bit_op, 1);
    header->pusi = bitoper_walk_number(bit_op, 1);
    header->tp = bitoper_walk_number(bit_op, 1);
    header->pid = bitoper_walk_number(bit_op, 13);
    header->scram = bitoper_walk_number(bit_op, 2);
    header->adapt = bitoper_walk_number(bit_op, 1);
    header->payload = bitoper_walk_number(bit_op, 1);
    header->continuity = bitoper_walk_number(bit_op, 4);
    return TRUE;
}

bool tsdecoder_packet_header_adapt(transport_stream* ts, ts_adaptation_field* field) {
    size_t status, one_element = 1;
    char buff[TSPACKET_ADAPT_FIELD_SIZE];
    // load adaptation field
    status = fread(buff, TSPACKET_ADAPT_FIELD_SIZE, one_element, ts->fo);
    if (status != one_element) {
        return FALSE;
    }
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, buff, TSPACKET_ADAPT_FIELD_SIZE * 8);
    field->length = bitoper_walk_number(bit_op, 8);
    field->discontinuity = bitoper_walk_number(bit_op, 1);
    field->random = bitoper_walk_number(bit_op, 1);
    field->es_priority = bitoper_walk_number(bit_op, 1);
    field->pcr = bitoper_walk_number(bit_op, 1);
    field->opcr = bitoper_walk_number(bit_op, 1);
    field->splicing_point = bitoper_walk_number(bit_op, 1);
    field->private_data = bitoper_walk_number(bit_op, 1);
    field->adapt_extension = bitoper_walk_number(bit_op, 1);
    return TRUE;
}

void tsdecoder_fill_buffer(transport_stream* ts) {
    ts_packet_header* p_hdr;
    char b;
    char payload[TSPACKET_PAYLOAD_SIZE];
    size_t status, byte_len = 1, one_element = 1;
    bool found;
    int i;
    ts->position = 1;
    ts->buffer_length = 0;
    printfdbg("filling tsdecoder's buffer...");
    while ( fread(&b, byte_len, one_element, ts->fo) == one_element ) {
        if ( !tsdecoder_is_packet_start(b) ) continue;
        // load packet header, if unavailable (false) go to next byte
        p_hdr = &(ts->buffer[ ts->buffer_length ].header);
        found = tsdecoder_packet_header(ts, p_hdr);
        if ( !found ) continue;
        status = fread(payload, TSPACKET_PAYLOAD_SIZE, one_element, ts->fo);
        if (status != one_element) break; // no packets left to read (whole packets)
        // don't care scrambled packets, packets without payload (are they usable for reading EPG?)
        if (p_hdr->pid != EPG_GETSTREAM_PID || p_hdr->scram) continue;
        //don't need this here (possibly loss of information) if ( p_hdr->pusi == 0 && !ts->start_found ) continue;
        // filter packets with adaptation field (optional) set to 0x01
        if (p_hdr->adapt == 1) continue;
        // TODO watch continuity field!

        tsdecoder_print_tsheader(p_hdr);
        //ts->start_found = TRUE;
        memcpy(ts->buffer[ ts->buffer_length ].payload, payload, TSPACKET_PAYLOAD_SIZE);
        ts->buffer_length++;
        // buffer is filled enough
        if (ts->buffer_length == TSPACKET_BUFFER_SIZE) break;
    }
    printfdbg("tsdecoder's buffer filled.");
    /*printfdbg("Buffer filled, ts->buffer_length=%d", ts->buffer_length);
    for (i = 0; i < ts->buffer_length; i++) printf("%c", ts->buffer[i]);*/
}

// Gets one packet from buffer. Returns TRUE if operation performed successfully otherwise FALSE.
bool tsdecoder_get_packet(transport_stream* ts, ts_packet* packet) {
    if (ts->position > ts->buffer_length) {
        tsdecoder_fill_buffer(ts);
    }
    if (ts->position > ts->buffer_length) {
        // no data available at the moment
        ts->end_reached = TRUE;
        return FALSE;
    }
    printfdbg("Packet on position %d", ts->position-1);
    *packet = ts->buffer[ts->position - 1];
    ts->position++; 
    return TRUE;
}

// Prints out decoded paket headers in order they came from input FILE*.
void tsdecoder_print_packets(transport_stream* ts) {
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
