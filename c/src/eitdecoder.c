#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/eitdecoder.h"

// Decodes from 14Bytes of data EIT.
// Returns TRUE if data were successfuly decoded, otherwise FALSE.
bool eitdecoder_decode(eitable* eit, unsigned char* buff) {
    int i;
    printfdbg("EIT data:");
    for (i = 0; i < EITABLE_SIZE; i++) printf("%02x ", buff[i]);
    eit->table_id = buff[1];
    printfdbg("\nEIT table_id=%02x", eit->table_id);
    return TRUE;
}

/*
1) actual TS, present/following event information = table_id = "0x4E";
2) other TS, present/following event information = table_id = "0x4F";
3) actual TS, event schedule information = table_id = "0x50" to "0x5F";
4) other TS, event schedule information = table_id = "0x60" to "0x6F".
*/
