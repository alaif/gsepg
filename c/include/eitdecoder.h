#ifndef __EITDECODER_H
#define __EITDECODER_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
   u_char table_id                               :8;
   u_char section_syntax_indicator               :1;
   u_char                                        :3;
   u_char section_length_hi                      :4;
   u_char section_length_lo                      :8;
   u_char service_id_hi                          :8;
   u_char service_id_lo                          :8;
   u_char                                        :2;
   u_char version_number                         :5;
   u_char current_next_indicator                 :1;
   u_char section_number                         :8;
   u_char last_section_number                    :8;
   u_char transport_stream_id_hi                 :8;
   u_char transport_stream_id_lo                 :8;
   u_char original_network_id_hi                 :8;
   u_char original_network_id_lo                 :8;
   u_char segment_last_section_number            :8;
   u_char segment_last_table_id                  :8;
} eitable;
#define EITABLE_SIZE 14


typedef struct {
   u_char event_id_hi                            :8;
   u_char event_id_lo                            :8;
   u_char mjd_hi                                 :8;
   u_char mjd_lo                                 :8;
   u_char start_time_h                           :8;
   u_char start_time_m                           :8;
   u_char start_time_s                           :8;
   u_char duration_h                             :8;
   u_char duration_m                             :8;
   u_char duration_s                             :8;
   u_char running_status                         :3;
   u_char free_ca_mode                           :1;
   u_char descriptors_loop_length_hi             :4;
   u_char descriptors_loop_length_lo             :8;
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


bool eitdecoder_decode(eitable* eit, unsigned char* buff);

#ifdef __cplusplus
}
#endif

#endif
