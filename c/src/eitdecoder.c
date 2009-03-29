#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/eitdecoder.h"
#include "../include/bitoper.h"
#include "../include/crc32.h"

/*
1) actual TS, present/following event information = table_id = "0x4E";
2) other TS, present/following event information = table_id = "0x4F";
3) actual TS, event schedule information = table_id = "0x50" to "0x5F";
4) other TS, event schedule information = table_id = "0x60" to "0x6F".
*/

// Decodes EIT from 14 bytes length buff.
// Returns TRUE if data were successfuly decoded, otherwise FALSE.
bool eitdecoder_decode(eitable* eit, char* buff) {
    int i;
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    printfdbg("EIT data:");
    for (i = 0; i < EITABLE_SIZE; i++) printf("%02x ", buff[i]);
    printf("\n");

    bitoper_init(bit_op, buff, EITABLE_SIZE * 8);
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
    printfdbg("EIT table_id=%02x", eit->table_id);
	if( dvb_crc32((long*)buff, 184) != 0)  {
        printfdbg("EIT crc error.");
    }

    return TRUE;
}
