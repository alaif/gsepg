#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <wchar.h>
#include <time.h>
// i18n
#include <libintl.h>
#include <locale.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/tsdecoder.h"
#include "../include/eitdecoder.h"
#include "../include/bitoper.h"
#include "../include/crc32.h"
#include "../include/dvbchar.h"

// descriptor handler registration array
bool descriptor_handlers_registered = FALSE;
descriptor_handler descriptor_handlers[DESCRIPTOR_HANDLER_COUNT];
// output
FILE *output_stream;
bool output_stream_set = FALSE;
int output_indent = 0;
//  abort decoding before program exit
bool abort_decode = FALSE;


void eitdecoder_init() {
    abort_decode = FALSE;
    output_stream_set = FALSE;
    output_indent = 0;
}

void eitdecoder_trigger_abort() {
    abort_decode = TRUE;
}

void eitdecoder_set_output_stream(FILE *ostream) {
    output_stream = ostream;
    output_stream_set = TRUE;
}

void eitdecoder_output_indent_add(int addition) {
    output_indent += addition;
    if (output_indent < 0)
        output_indent = 0;
}

void eitdecoder_output(const char *format, ...) {
	va_list ap; 
	va_start(ap, format);
    if (output_stream_set) {
        int i;
        for (i = 1; i <= output_indent; i++)
            fprintf(output_stream, " ");
        vfprintf(output_stream, format, ap);
    }
	va_end(ap); 
}

void eitdecoder_raw_data(const unsigned char *data, int len) {
    int i;
    char *pdata;
    pdata = (char*)malloc(len + 1);
    if (pdata == NULL) {
        // fail silently, this is debug func. only
        return;
    }
    memcpy(pdata, data, len);
    for (i = 0; i < len; i++) {
        if (!isalpha(pdata[i]))
            pdata[i] = '.';
    }
    pdata[len] = '\0'; // better safe than sorry
    printfdbg("Raw data: [%s]", pdata);
    free((void*)pdata);
    pdata = NULL;
}


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
    printfdbg("EIT table found: pointer=%02x table_id=%02x", pointer, table_id);
    return TRUE;
}

// Decodes EIT from 14 bytes length buff.
// Returns TRUE if data were successfuly decoded, otherwise FALSE.
bool eitdecoder_table(ts_packet *packet, eitable *eit) {
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    unsigned char *payload = packet->payload; payload++; //ommitting 0x00 pointer byte.

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

void eitdecoder_decode_event(unsigned char *payload, eitable_event *evt) {
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

// -----------------------------------------------
// Descriptor callbacks, registration and dispatch
// -----------------------------------------------

// short event descriptor described in ETSI EN 300 468, page 69
// Output:
// "id": "short event",
// "name": "event name",
// "lang": "language",
// "text": "event text, if present"
void descriptor_short_event(int tag, int length, unsigned char *data) {
    printfdbg("=== BEGIN OF SHORT EVENT DESC. length=%d", length);
    unsigned char *event_data;
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, data, SHORT_EVENT_LANG_AND_NAME * 8);

    char lang[4];
    lang[0] = bitoper_walk_number(bit_op, 8);
    lang[1] = bitoper_walk_number(bit_op, 8);
    lang[2] = bitoper_walk_number(bit_op, 8);
    lang[3] = '\0';
    int name_len = bitoper_walk_number(bit_op, 8);
    event_data = data + (SHORT_EVENT_LANG_AND_NAME/8);

    unsigned char text[255];
    wchar_t decoded[255];
    // event name
    memset(text, '\0', 255);
    memcpy(text, event_data, name_len);
    printfdbg("copying event_name to text, lang=%s, len=%d, text=[%s]", lang, name_len, text);
    dvbchar_decode(text, decoded, name_len);
    printfdbg("Name (decoded iso6937/2): %ls", decoded);
    eitdecoder_output("\"id\": \"short event\",\n");
    eitdecoder_output("\"name\": \"%ls\",\n", decoded);
    eitdecoder_output("\"lang\": \"%s\",\n", lang);
    // event description
    event_data += name_len;
    int text_len = (int)event_data[0];
    if (text_len == 0) {
        printfdbg("No event data.");
        printfdbg("--- END");
        eitdecoder_output("\"text\": \"\"\n", lang);
        return;
    }
    event_data++;
    printfdbg("copying event_data to text, lang=%s, len=%d", lang, text_len);
    memcpy(text, event_data, text_len);
    dvbchar_decode(text, decoded, text_len);
    printfdbg("Text (decoded iso6937/2): %ls", decoded);
    eitdecoder_output("\"text\": \"%ls\"\n", decoded);
    printfdbg("--- END");
}

// PDC descriptor, ETSI EN 300 468, page 65
// Output:
// "id": "pdc",
// "day": 20,
// "month": 4,
// "hour": 10,
// "minute": 58
void descriptor_pdc(int tag, int length, unsigned char *data) {
    printfdbg("=== BEGIN PDC DESC. len=%d", length);
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, data, PDC_LENGTH * 8);
    bitoper_walk_number(bit_op, 4); // reserved 4b for future use
    // PIL ... program identification label
    int day = bitoper_walk_number(bit_op, 5);
    int month = bitoper_walk_number(bit_op, 4);
    int hour = bitoper_walk_number(bit_op, 5);
    int minute = bitoper_walk_number(bit_op, 6);
    printfdbg("PDC raw: day=%d month=%d hour=%d minute=%d", day, month, hour, minute);
    eitdecoder_output("\"id\": \"pdc\",\n");
    eitdecoder_output("\"day\": %d,\n", day);
    eitdecoder_output("\"month\": %d,\n", month);
    eitdecoder_output("\"hour\": %d,\n", hour);
    eitdecoder_output("\"minute\": %d\n", minute);
    printfdbg("--- END");
}

// Parental rating descriptor
// Output:
// "id": "parental rating",
// "ratings": [{"country": "CZE", "age": 90, "custom": 1}, {"country": "GER", "age": 18}]
void descriptor_parental(int tag, int length, unsigned char *data) {
    printfdbg("=== BEGIN PARENTAL RATING DESC. len=%d", length);
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    unsigned char *event_data;
    int i;
    char country_code[4];
    int rating;
    int sec_length = length / PARENTAL_COUNTRY_AND_RATING;
    event_data = data;
    country_code[3] = '\0';
    eitdecoder_output("\"id\": \"parental rating\",\n");
    eitdecoder_output("\"ratings\": [\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);
    for (i = 0; i < sec_length; i++) {
        eitdecoder_output("{\n");
        eitdecoder_output_indent_add(OUTPUT_INDENT);

        bitoper_init(bit_op, event_data, PARENTAL_COUNTRY_AND_RATING * 8);
        country_code[0] = bitoper_walk_number(bit_op, 8);
        country_code[1] = bitoper_walk_number(bit_op, 8);
        country_code[2] = bitoper_walk_number(bit_op, 8);
        rating = bitoper_walk_number(bit_op, 8);
        eitdecoder_output("\"country\": \"%s\",\n", country_code);
        if (rating >= 0x01 && rating <= 0x0F) {
            printfdbg("in country [%s] the content is not recommended under age %d", country_code, rating + 3);
            eitdecoder_output("\"age\": %d\n", rating + 3);
        } else if (rating == 0) {
            printfdbg("No rating for country [%s].", country_code);
            eitdecoder_output("\"age\": 0\n");
        } else {
            printfdbg("Custom defined rating=%d (0x%02x).", rating, rating);
            eitdecoder_output("\"age\": %d,\n", rating);
            eitdecoder_output("\"custom\": 1\n");
        }
        event_data += PARENTAL_COUNTRY_AND_RATING;
        eitdecoder_output_indent_add(-OUTPUT_INDENT);
        if ((i + 1) < sec_length) {
            eitdecoder_output("},\n");
        } else  {
            eitdecoder_output("}\n");
        }
    }
    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("]\n");
    printfdbg("--- END");
}

// Extended event descriptor
// Output:
// "id": "extended event",
// "descriptor_number": 0,
// "last_descriptor_number": 10,
void descriptor_extended_event(int tag, int length, unsigned char *data) {
    printfdbg("=== BEGIN EXTENDED EVENT DESC. len=%d", length);
    unsigned char *event_data;
    bitoper _bit_op;
    bitoper* bit_op = &_bit_op;
    bitoper_init(bit_op, data, EXTENDED_EVENT_HEADER * 8);
    int descriptor_number = bitoper_walk_number(bit_op, 4);
    int last_descriptor_number = bitoper_walk_number(bit_op, 4);
    char lang[4];
    lang[0] = bitoper_walk_number(bit_op, 8);
    lang[1] = bitoper_walk_number(bit_op, 8);
    lang[2] = bitoper_walk_number(bit_op, 8);
    lang[3] = '\0';
    int items_length = bitoper_walk_number(bit_op, 8);
    printfdbg("desc_no=%d  last_desc_no=%d", descriptor_number, last_descriptor_number);
    event_data = data;
    event_data += EXTENDED_EVENT_HEADER;
    int i = 0;
    printfdbg("items_length=%d", items_length);
    // output
    eitdecoder_output("\"id\": \"extended event\",\n");
    eitdecoder_output("\"descriptor_number\": %d,\n", descriptor_number);
    eitdecoder_output("\"last_descriptor_number\": %d,\n", last_descriptor_number);
    eitdecoder_output("\"descriptions\":[\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);
    wchar_t decoded[255];

    while (i <= items_length || i == 0) {
        eitdecoder_output("{\n");
        eitdecoder_output_indent_add(OUTPUT_INDENT);
        unsigned char item_desc[255];
        int item_desc_length = (int)event_data[0];
        //if (item_desc_length >= items_length) break;
        event_data++;
        i++;
        memcpy(item_desc, event_data, item_desc_length);
        event_data += item_desc_length;
        i += item_desc_length;
        item_desc[item_desc_length] = '\0';
        printfdbg("len=%d item_desc=[%s] ", item_desc_length, item_desc);

        int item_length = (int)event_data[0];
        unsigned char item[255];
        if (item_length >= (items_length - item_desc_length)) {
            item_length = 0;
        }
        event_data++;
        i++;
        memcpy(item, event_data, item_length);
        event_data += item_length;
        i += item_length;
        item[item_length] = '\0';
        printfdbg("len=%d item=[%s] ", item_length, item);
        if (item_desc_length > 0)  {
            dvbchar_decode(item_desc, decoded, item_desc_length);
            printfdbg("item_desc (decoded iso6937/2): %ls", decoded);
            eitdecoder_output("\"description\": \"%ls\",\n", decoded);
        } else {
            eitdecoder_output("\"description\": \"\",\n");
        }
        dvbchar_decode(item, decoded, item_length);
        printfdbg("item (decoded iso6937/2): %ls", decoded);
        eitdecoder_output("\"item\": \"%ls\"\n", decoded);
        eitdecoder_output_indent_add(-OUTPUT_INDENT);
        if (i <= items_length) {
            eitdecoder_output("},\n");
        } else {
            eitdecoder_output("}\n");
        }
    }
    // optional text field
    /*int text_length = (int)event_data[0];
    unsigned char text[255];
    event_data++;
    memcpy(text, event_data, text_length);
    dvbchar_decode(text, decoded, text_length);
    printfdbg("len=%d text=[%s]", text_length, text);
    eitdecoder_output("\"text\": \"%ls\"\n", decoded);*/

    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("]\n");
    printfdbg("--- END");
}

// Register all descriptor handlers
void eitdecoder_descriptor_handler_registration() {
    if (descriptor_handlers_registered) 
        return;
    descriptor_handler *hdl = descriptor_handlers;
    // short event descriptor
    hdl->tag = DESC_SHORT_EVENT;
    hdl->callback = descriptor_short_event;
    hdl++;
    // network descriptor
    hdl->tag = DESC_PARETAL_RATING;
    hdl->callback = descriptor_parental;
    hdl++;
    // extended event descriptor
    hdl->tag = DESC_EXTENDED_EVENT;
    hdl->callback = descriptor_extended_event;
    hdl++;
    // PDC descriptor ... something like VPS system, useful for capturing movies from DVB at time.
    hdl->tag = DESC_PDC;
    hdl->callback = descriptor_pdc;
    hdl++;
    // TODO other descriptor callback registrations place here..
    descriptor_handlers_registered = TRUE;
}

bool eitdecoder_descriptor_dispatcher(int tag, int length, unsigned char *data) {
    eitdecoder_descriptor_handler_registration(); // perform registration if neccessary
    int i;
    descriptor_handler *hdl = descriptor_handlers;
    for (i = 0; i < DESCRIPTOR_HANDLER_COUNT; i++) {
        if (hdl->tag == tag) {
            hdl->callback(tag, length, data);
            return TRUE;
        }
        hdl++;
    }
    printfdbg("Unregistered handler for descriptor %02x len=%d", tag, length);
    eitdecoder_raw_data(data, length);
    return FALSE;
}


// Get y-m-d from modified julian date, algorithm specified in ETSI EN 300 468, page 99
void eitdecoder_decode_mjd(int julian, int *year, int *month, int *day) {
    long int mjd = julian;
    *year = (int) ((mjd - 15078.2) / 365.25);
    *month = (int) ((mjd - 14956.1 - (int) (*year * 365.25)) / 30.6001);
    *day = mjd - 14956 - (int) (*year * 365.25) - (int) (*month * 30.6001);
    int i;
    if (*month == 14 || *month == 15)
        i = 1;
    else
        i = 0;
    *year += i +1900;
    *month = *month - 1 - (i * 12);
    printfdbg("Event mjd: year=%d month=%d day=%d", *year, *month, *day);
}

bool _event_loop_condition(int total_len, int read_len) {
    return (read_len < total_len && (total_len - read_len) > EITABLE_EVENT_SIZE);
}

// Iterating over descriptors within EIT Events
void eitdecoder_event_descriptors(unsigned char *section_data, int total_len) {
    eitable_event _evt;
    eitable_event *evt = &_evt;
    int read_len = 0;
    int year, month, day;
    unsigned char *payload;
    payload = section_data;
    printfdbg("section_data len=%d", total_len);
    eitdecoder_raw_data(section_data, total_len);

    eitdecoder_output("\"events\": [\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);

    while (_event_loop_condition(total_len, read_len)) {
        // 12 bytes of EIT event table (ETSI EN 300 468: page 25)
        eitdecoder_decode_event(payload, evt);
        read_len += EITABLE_EVENT_SIZE;
        payload += EITABLE_EVENT_SIZE;
        printfdbg(
          "= Event id=%d running=%d free_ca=%d desc_length=%d",
          evt->event_id,
          evt->running_status,
          evt->free_ca_mode,
          evt->descriptors_loop_length
        );
        // date/time
        eitdecoder_decode_mjd(evt->mjd, &year, &month, &day);
        if (year < 1999 || month < 0 || day < 0) {
            read_len += evt->descriptors_loop_length;
            payload += evt->descriptors_loop_length;
            continue;
        }
        // output
        eitdecoder_output("{\n");
        eitdecoder_output_indent_add(OUTPUT_INDENT);

        eitdecoder_output("\"running_status\": %d,\n", evt->running_status);
        eitdecoder_output("\"free_ca_mode\": %d,\n", evt->free_ca_mode);
        eitdecoder_output("\"date\": \"%04d-%02d-%02d\",\n", year, month, day);
        printfdbg(
            "Start: %02d:%02d:%02d   duration: %02d:%02d:%02d",
            bcdtoint(evt->start_time_h),
            bcdtoint(evt->start_time_m),
            bcdtoint(evt->start_time_s),
            bcdtoint(evt->duration_h),
            bcdtoint(evt->duration_m),
            bcdtoint(evt->duration_s)
        );
        eitdecoder_output(
            "\"start_time\": \"%02d:%02d:%02d\",\n", 
            bcdtoint(evt->start_time_h),
            bcdtoint(evt->start_time_m),
            bcdtoint(evt->start_time_s)
        );
        eitdecoder_output(
            "\"duration\": \"%02d:%02d:%02d\",\n", 
            bcdtoint(evt->duration_h),
            bcdtoint(evt->duration_m),
            bcdtoint(evt->duration_s)
        );
        int desc_read_len = 0;
        bool desc_result = FALSE;
        bool previous_item = FALSE;
        eitdecoder_output("\"descriptors\": [\n");
        eitdecoder_output_indent_add(OUTPUT_INDENT);
        while (desc_read_len < evt->descriptors_loop_length) {
            unsigned char descriptor_tag = payload[0];
            if (descriptor_tag == TSPACKET_STUFFING_BYTE) {
                // stuffing data occured, skip them all (TODO optimalization -> change read_len
                // to total_len and break, due to stuffing data occurs only at the end of packet payload.
                // Verify this approach first.)
                desc_read_len++;
                read_len++; //added 20090415
                payload++; //added 20090415
                continue;
            }
            if (previous_item)
                eitdecoder_output(",\n");
            unsigned char descriptor_len = payload[1];
            payload += 2; // move to descriptor data only for convenience
            desc_read_len += 2;
            // prepare JSON dict for descriptor's output
            eitdecoder_output("{\n");
            eitdecoder_output_indent_add(OUTPUT_INDENT);

            // for descriptor coding see ETSI EN 300 468, page 31
            desc_result = eitdecoder_descriptor_dispatcher(descriptor_tag, descriptor_len, payload);

            eitdecoder_output_indent_add(-OUTPUT_INDENT);
            eitdecoder_output("}\n");

            desc_read_len += descriptor_len;
            payload += descriptor_len;
            previous_item = TRUE;
        }
        eitdecoder_output_indent_add(-OUTPUT_INDENT);
        eitdecoder_output("]\n");
        read_len += evt->descriptors_loop_length;
        printfdbg("- Event id=%d end.", evt->event_id);
        eitdecoder_output_indent_add(-OUTPUT_INDENT);
        // handle abort
        if (abort_decode) {;
            eitdecoder_output("}\n");
            break;
        }
        if (_event_loop_condition(total_len, read_len)) {
            eitdecoder_output("},\n");
        } else { 
            // last item
            eitdecoder_output("}\n");
        }
    }

    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("]\n");
}

// 0x40 6.2.27 Network name descriptor ... page 64
// 0x55 6.2.28 Parental Descriptor ... page
// 0x54 6.2.9  Content Descriptor ... page 39 (genres, etc.)
// 0x4e 6.2.15 Extended event descriptor ... page 50
// 0x7f 6.2.16 Extension descriptor ... page 51
// Image icon descriptor ... page 78

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
    ts_packet *pac = &packetts;
    int total_len = eit->section_length - EITABLE_SL_REMAINING;
    int read_len = 0;
    bool error_flag = FALSE;
    unsigned char *payload = current_packet->payload;
    unsigned char section_data[4096]; // 4096 bytes is section_data max length permitted by ETSI spec.
    unsigned char *section_data_pos = section_data;

    payload += EITABLE_SIZE + 1; // skip EIT header and the Pointer byte.

    // Loading all data "section length" long.
    read_len = TSPACKET_PAYLOAD_SIZE - (EITABLE_SIZE + 1); // remaining payload data length
    section_data_pos += read_len;
    memcpy(section_data, payload, read_len); // copy remaining data from packet at actual position
    printfdbg("Remaining packet data copied. len=%d", read_len);

    eitdecoder_output("{\n");
    eitdecoder_output_indent_add(OUTPUT_INDENT);
    eitdecoder_output("\"service_id\": %d,\n", eit->service_id);
    eitdecoder_output("\"version_number\": %d,\n", eit->version_number);
    if (eit->table_id == EITABLE_PRESENT_FOLLOWING_ACTUAL_TS ||
        eit->table_id == EITABLE_PRESENT_FOLLOWING_OTHER_TS) {
        eitdecoder_output("\"section\": \"present/following\",\n");
    } else if (eit->table_id >= EITABLE_SCHEDULE_ACTUAL_TS_MIN &&
               eit->table_id <= EITABLE_SCHEDULE_OTHER_TS_MAX) {
        eitdecoder_output("\"section\": \"schedule\",\n");
    }

    while (read_len < total_len) {
        if (!tsdecoder_get_packet(ts, pac)) {
            printferr("tsdecoder was unable to get TS packet.");
            error_flag = TRUE;
            break;
        }
        memcpy(section_data_pos, pac->payload, TSPACKET_PAYLOAD_SIZE);
        section_data_pos += TSPACKET_PAYLOAD_SIZE;
        read_len += TSPACKET_PAYLOAD_SIZE;
        //printfdbg("Added next packet's payload to section_data.");
    }
    if (error_flag) {
        // finish output section
        // empty events section in JSON when error occured
        eitdecoder_event_descriptors("", 0);
        eitdecoder_output_indent_add(-OUTPUT_INDENT);
        eitdecoder_output("}\n");
        return FALSE;
    }
    printfdbg("section_data read_len=%d total_len=%d", read_len, total_len);
    // handle event descriptors
    eitdecoder_event_descriptors(section_data, total_len);

    // finish output section
    eitdecoder_output_indent_add(-OUTPUT_INDENT);
    eitdecoder_output("}\n");
    // handle bitoper error
    if (bitoper_err != BITOPER_OK) return FALSE; // during event descriptor hanlding bitoper error occured => affects eitdecoder
    return TRUE;
}
