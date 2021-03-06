#ifndef __EITDECODER_H
#define __EITDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/tsdecoder.h"

#define OUTPUT_INDENT            4            // 4 spaces indent is default

// Descriptors (selected ones)
#define DESC_NETWORK_NAME        0x40
#define DESC_SHORT_EVENT         0x4d
#define SHORT_EVENT_LANG_AND_NAME   (24 + 8)  // byte length
#define DESC_EXTENDED_EVENT      0x4e
#define EXTENDED_EVENT_HEADER    5            // byte length
#define DESC_COMPONENT           0x50
#define DESC_CONTENT             0x54
#define DESC_PARETAL_RATING      0x55
#define DESC_PDC                 0x69
#define PDC_LENGTH               3            // byte length
#define PARENTAL_COUNTRY_AND_RATING 4         // byte length (24b + 8b)

#define DESCRIPTOR_HANDLER_COUNT    4         // number of descriptor registrations

// macro suitable for time information conversion
#define bcdtoint(i) ((((i & 0xf0) >> 4) * 10) + (i & 0x0f))

typedef struct {
    unsigned char tag; //descriptor tag (i.e. DESC_SHORT_EVENT)
    void (*callback)(int tag, int length, unsigned char *data);
} descriptor_handler;


typedef struct {
   unsigned int table_id                               :8;
   unsigned int section_syntax_indicator               :1;
   unsigned int                                        :3;
   unsigned int section_length                         :12;
   unsigned int service_id                             :16;
   unsigned int                                        :2;
   unsigned int version_number                         :5;
   unsigned int current_next_indicator                 :1;
   unsigned int section_number                         :8;
   unsigned int last_section_number                    :8;
   unsigned int transport_stream_id                    :16;
   unsigned int original_network_id                    :16;
   unsigned int segment_last_section_number            :8;
   unsigned int segment_last_table_id                  :8;
} eitable;
#define EITABLE_SIZE 14
#define EITABLE_SL_REMAINING   10   // 10 bytes remaining after section_length field.
#define EITABLE_PRESENT_FOLLOWING_ACTUAL_TS            0x4e
#define EITABLE_PRESENT_FOLLOWING_OTHER_TS             0x4f
#define EITABLE_SCHEDULE_ACTUAL_TS_MIN                 0x50
#define EITABLE_SCHEDULE_ACTUAL_TS_MAX                 0x5f
#define EITABLE_SCHEDULE_OTHER_TS_MIN                  0x60
#define EITABLE_SCHEDULE_OTHER_TS_MAX                  0x6f


typedef struct {
   unsigned int event_id                               :16;
   unsigned int mjd                                    :16;
   unsigned int start_time_h                           :8;
   unsigned int start_time_m                           :8;
   unsigned int start_time_s                           :8;
   unsigned int duration_h                             :8;
   unsigned int duration_m                             :8;
   unsigned int duration_s                             :8;
   unsigned int running_status                         :3;
   unsigned int free_ca_mode                           :1;
   unsigned int descriptors_loop_length                :12;
} eitable_event;
#define EITABLE_EVENT_SIZE 12



typedef struct {
   u_char table_id                               :8; 
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
   u_char section_length_lo                      :8;
   u_char utc_mjd_hi                             :8;
   u_char utc_mjd_lo                             :8;
   u_char utc_time_h                             :8;
   u_char utc_time_m                             :8;
   u_char utc_time_s                             :8;
} timedatetable;
#define TIMEDATETABLE_SIZE 8

typedef struct {
   u_char table_id                               :8; 
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
   u_char section_length_lo                      :8;
   u_char utc_mjd_hi                             :8;
   u_char utc_mjd_lo                             :8;
   u_char utc_time_h                             :8;
   u_char utc_time_m                             :8;
   u_char utc_time_s                             :8;
   u_char                                        :4;
   u_char descriptors_loop_length_hi             :4;
   u_char descriptors_loop_length_lo             :8;
} timeoffsettable;
#define TIMEOFFSETTABLE_SIZE 10


bool eitdecoder_detect_eit(ts_packet* packet);
bool eitdecoder_table(ts_packet *packet, eitable *eit);
void eitdecoder_decode_event(unsigned char *payload, eitable_event *evt);
bool eitdecoder_events(transport_stream *ts, ts_packet *current_packet, eitable *eit);
void eitdecoder_output(const char *, ...);
void eitdecoder_set_output_stream(FILE *ostream);
void eitdecoder_output_indent_add(int addition);
void eitdecoder_raw_data(const unsigned char *data, int len);
void eitdecoder_trigger_abort();
void eitdecoder_init();

#ifdef __cplusplus
}
#endif

#endif
