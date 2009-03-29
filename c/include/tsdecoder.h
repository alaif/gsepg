#ifndef __TSDECODER_H
#define __TSDECODER_H

#ifdef __cplusplus
extern "C" {
#endif


// Structure of packet header and adaptation field can be found on http://en.wikipedia.org/wiki/MPEG_transport_stream
// packet header
typedef struct {
    // sync Byte
    //unsigned char sync   : 8;          // 0x47
    // 2 Bytes
    unsigned char tei    : 1;          // transport error indicator
    unsigned char pusi   : 1;          // payload unit start indicator
    unsigned char tp     : 1;          // transport priority
    unsigned int pid    :13;          // packet id
    // 1 Byte
    unsigned char scram  : 2;          // scrambling control
    unsigned char adapt  : 1;          // adaptation field exist
    unsigned char payload: 1;          // payload data exist in packet
    unsigned char continuity: 4;       // packet continuity counter
} ts_packet_header; // (3 Bytes + 1 Byte 0x47 sync = total 4 Bytes)
#define TSPACKET_HEADER_SIZE   3
#define TSPACKET_PAYLOAD_SIZE  184 // 188 - 4 (ts_packet_header + sync byte)

// optional adaptation field
typedef struct {
    // 1 Byte
    unsigned  int length          : 8;          // number of bytes in the adapt. fld. following this byte
    // 1 Byte
    unsigned char discontinuity   : 1;          // discont. indicator
    unsigned char random          : 1;          // random access indicator
    unsigned char es_priority     : 1;          // elementary stream priority indic.
    unsigned char pcr             : 1;          // pcr flag
    unsigned char opcr            : 1;          // opcr flag
    unsigned char splicing_point  : 1;          // splicing point flag
    unsigned char private_data    : 1;          // transport private data flag
    unsigned char adapt_extension : 1;          // adaptation field extension flag
    // optional fields:
    //unsigned char pcr_data[33b+9b];        // pcr - program clock reference
    //unsigned char opcr_data[33b+9b];       // opcr - original program clock reference
    //unsigned  int splice_count  : 8;     // splice countdown
} ts_adaptation_field;
#define TSPACKET_ADAPT_FIELD_SIZE     2 // optional field lenghts excluded

// TS packet
typedef struct {
    ts_packet_header header;
    char payload[TSPACKET_PAYLOAD_SIZE];
} ts_packet;

// TS representation
#define TSPACKET_BUFFER_SIZE 128 
typedef struct {
    int pid;
	char* filename;
	FILE *fo;
    ts_packet buffer[TSPACKET_BUFFER_SIZE];  // TS packet buffer (probably better for finding nested tables in packets etc.)
    int buffer_length;
    int position;      // byte position in payload-data buffer.
    bool start_found;   // start of readable data (first usable packet)
    bool end_reached;   // end of stream (end of TransportStream->fo file)
} transport_stream;


transport_stream* tsdecoder_new(char* filename, int pid);
bool tsdecoder_init(transport_stream* ts, char* filename, int pid);
void tsdecoder_free(transport_stream** ts);
bool tsdecoder_packet_header(transport_stream* ts, ts_packet_header* header);
bool tsdecoder_packet_header_adapt(transport_stream* ts, ts_adaptation_field* field);
void tsdecoder_print_packets(transport_stream* ts);
bool tsdecoder_get_packet(transport_stream* ts, ts_packet* packet);

#ifdef __cplusplus
}
#endif

#endif
