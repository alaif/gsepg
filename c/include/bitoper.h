#ifndef __BITOPER_H
#define __BITOPER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char* data;         // data to be read bit by bit
    int bit_length;     // length of data (in bits)
    int data_position;  // position in data buffer
    int read_bits;      // how many bits already have been read from byte at data[data_position]
    int total_read_bits;
} bitoper;

void bitoper_init(bitoper* op, unsigned char* buff, int length);
int bitoper_get_number(unsigned char* data, int bit_len, int offset);
int bitoper_walk_number(bitoper* op, int bit_len);

#ifdef __cplusplus
}
#endif

#endif
