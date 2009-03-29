#ifndef __EITDECODER_H
#define __EITDECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/tsdecoder.h"

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


typedef struct {
   unsigned int event_id_hi                            :16;
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

/*
 *
 *    4) Running Status Table (RST):
 *
 *       - the RST gives the status of an event (running/not running). The RST
 *         updates this information and allows timely automatic switching to
 *         events.
 *
 */
    /* TO BE DONE */
/*
 *
 *    5) Time and Date Table (TDT):
 *
 *       - the TDT gives information relating to the present time and date.
 *         This information is given in a separate table due to the frequent
 *         updating of this information.
 *
 */


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

/*
 *
 *    6) Time Offset Table (TOT):
 *
 *       - the TOT gives information relating to the present time and date and
 *         local time offset. This information is given in a separate table due
 *         to the frequent updating of the time information.
 *
 */

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
bool eitdecoder_decode(eitable* eit, char* buff);

#ifdef __cplusplus
}
#endif

#endif
