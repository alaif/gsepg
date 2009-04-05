#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/tsdecoder.h"
#include "../include/eitdecoder.h"
#include "../include/bitoper.h"
#include "../include/crc32.h"

/*
1) actual TS, present/following event information = table_id = "0x4E";
2) other TS, present/following event information = table_id = "0x4F";
3) actual TS, event schedule information = table_id = "0x50" to "0x5F";
4) other TS, event schedule information = table_id = "0x60" to "0x6F".
*/

bool eitdecoder_detect_eit(ts_packet* packet) {
    if (packet->header.pusi == 0) return FALSE;
    if (packet->header.payload == 0) return FALSE;
    if (packet->header.scram != 0) return FALSE;
    unsigned char pointer = packet->payload[0];
    unsigned char table_id = packet->payload[1];
    if (pointer != 0) return FALSE;
    printfdbg("EIT table found? pointer=%02x table_id=%02x", pointer, table_id);
    return TRUE;
}

// Decodes EIT from 14 bytes length buff.
// Returns TRUE if data were successfuly decoded, otherwise FALSE.
bool eitdecoder_table(ts_packet *packet, eitable *eit) {
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    char *payload = packet->payload; payload++; //ommitting 0x00 pointer byte.

    bitoper_init(bit_op, payload, EITABLE_SIZE * 8);
    eit->table_id = bitoper_walk_number(bit_op, 8);
    eit->section_syntax_indicator = bitoper_walk_number(bit_op, 1);
    bitoper_walk_number(bit_op, 3); //reserved
    eit->section_length = bitoper_walk_number(bit_op, 12);
    eit->service_id = bitoper_walk_number(bit_op, 16);
    bitoper_walk_number(bit_op, 2); //reserved
    eit->version_number = bitoper_walk_number(bit_op, 5);
    eit->current_next_indicator = bitoper_walk_number(bit_op, 1);
    eit->section_number = bitoper_walk_number(bit_op, 8);
    eit->last_section_number = bitoper_walk_number(bit_op, 8);
    eit->transport_stream_id = bitoper_walk_number(bit_op, 16);
    eit->original_network_id = bitoper_walk_number(bit_op, 16);
    eit->segment_last_section_number = bitoper_walk_number(bit_op, 8);
    eit->segment_last_table_id = bitoper_walk_number(bit_op, 8);
    /*long crc_result = dvb_crc32((long*)buff, 184); //crc of all EIT data including last 32 bits with crc
	if( crc_result != 0)  {
        printfdbg("EIT crc error. result=%ld", crc_result);
    }*/

    return TRUE;
}

void eitdecoder_decode_event(char *payload, eitable_event *evt) {
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, payload, EITABLE_EVENT_SIZE * 8);
    evt->event_id = bitoper_walk_number(bit_op, 16);
    evt->mjd = bitoper_walk_number(bit_op, 16);
    evt->start_time_h = bitoper_walk_number(bit_op, 8);
    evt->start_time_m = bitoper_walk_number(bit_op, 8);
    evt->start_time_s = bitoper_walk_number(bit_op, 8);
    evt->duration_h = bitoper_walk_number(bit_op, 8);
    evt->duration_m = bitoper_walk_number(bit_op, 8);
    evt->duration_s = bitoper_walk_number(bit_op, 8);
    evt->running_status = bitoper_walk_number(bit_op, 3);
    evt->free_ca_mode = bitoper_walk_number(bit_op, 1);
    evt->descriptors_loop_length = bitoper_walk_number(bit_op, 12);
}

void eitdecoder_event_descriptors(char *section_data, int total_len) {
    eitable_event _evt;
    eitable_event *evt = &_evt;
    int read_len = 0;
    int i;
    unsigned char *payload;
    // Iterating over descriptors and EIT Events
    payload = section_data;
    while (read_len < total_len) {
        // 12 bytes of EIT event table, page 25
        eitdecoder_decode_event(payload, evt);
        printfdbg(
          "Event id=%d running=%d free_ca=%d desc_length=%d",
          evt->event_id,
          evt->running_status,
          evt->free_ca_mode,
          evt->descriptors_loop_length
        );
        read_len += EITABLE_EVENT_SIZE;
        payload += EITABLE_EVENT_SIZE;
        int desc_read_len = 0;
        while (desc_read_len < evt->descriptors_loop_length) {
            unsigned char descriptor_tag = payload[0];
            if (descriptor_tag == TSPACKET_STUFFING_BYTE) {
                // stuffing data occured
                desc_read_len++;
                continue;
            }
            unsigned char descriptor_len = payload[1];
            payload += 2; // move to descriptor data only for convenience
            desc_read_len += 2;
            printfdbg("Descriptor %02x len=%d", descriptor_tag, descriptor_len);
            for (i = 0; i < descriptor_len; i++) {
                //printf("%02x ", payload[i]);
                desc_read_len++;
            }
        }
        read_len += desc_read_len;
        // move pointer to the next event
        payload += desc_read_len;
    }
}

// From ETSI EN 300 468, page 18, table 2:
// ---- EIT:
// 0x4e  event information section. ACTUAL transport stream, PRESENT/FOLLOWING
// 0x4f  event information section. OTHER transport stream, PRESENT/FOLLOWING
// 0x50..0x5f  event information section. ACTUAL transport stream, SCHEDULE
// 0x60..0x6f  event information section. OTHER transport stream, SCHEDULE
// section_syntax_indicator should has value of 1
// section_length: number of bytes of the section, starting immediat. following the section_length field. <= 4093
//                 (remaining 10 EIT header bytes)
// service_id: program number from PMT
// version_number: should be incremented when content of subtable is changed
//                 when current_next_indicator == 0 then version_number shall be that of the next applicable subtable.
//                 when current_next_indicator == 1 then version_number shall be taht of currently applicable subtable.
// transport_stream_id
// ---- Event subtable(s):
// event_id: id of described event
// start_time: 40bit julian
// duration: 24bit, 6digits, 4bit BCD = 24bit
// running_status: 0..undef
//                 1..not running
//                 2..starts in a few seconds
//                 3..pausing
//                 4..running
//                 5..service off-air
// free_ca_mode:  0..all component streams are not scrambled , 1..access to one or more streams is controlled by CA system.
// descriptors_loop_length: total length in bytes of the (following) descriptors
// Descriptors in EIT described in ETSI TR 101 211, page 26. Technical specification in ETSI EN 300 468.
bool eitdecoder_events(transport_stream *ts, ts_packet *current_packet, eitable *eit) {
    ts_packet packetts;
    ts_packet* pac = &packetts;
    int total_len = eit->section_length - EITABLE_SL_REMAINING;
    int read_len = 0;
    char *payload = current_packet->payload;
    char section_data[4096]; // 4096 bytes is section_data max length permitted by ETSI spec.
    char *section_data_pos = section_data;

    payload += EITABLE_SIZE + 1; // skip EIT header and one Pointer byte.

    // Loading all data "section length" long.
    section_data_pos += TSPACKET_PAYLOAD_SIZE - read_len;
    memcpy(section_data, payload, TSPACKET_PAYLOAD_SIZE - read_len); // copy remaining data from packet at actual position
    read_len = EITABLE_SIZE + 1;
    printfdbg("Remaining packet data copied.");
    while (read_len < total_len) {
        if (!tsdecoder_get_packet(ts, pac)) break;
        memcpy(section_data_pos, pac->payload, TSPACKET_PAYLOAD_SIZE);
        section_data_pos += TSPACKET_PAYLOAD_SIZE;
        read_len += TSPACKET_PAYLOAD_SIZE;
        //printfdbg("Added next packet's payload to section_data.");
    }
    printfdbg("section_data read_len=%d total_len=%d", read_len, total_len);
    eitdecoder_event_descriptors(section_data, total_len);
    return TRUE;
}
