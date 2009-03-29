#include <stdio.h>
#include "../include/bitoper.h"
#include "../include/dbglib.h"

void bitoper_init(bitoper* op, char* buff, int length) {
    op->read_bits = 0;
    op->total_read_bits = 0;
    op->data_position = 0;
    op->data = buff;
    op->bit_length = length;
}

int bitoper_get_number(char* data, int bit_len, int offset) {
    // TODO this function is not required for epgextract project at the moment
    return 0;
}

// reads one bit and increments bit position, eventualy byte position
int bitoper_get_bit(bitoper* op) {
    if (op->read_bits == 8) {
        op->read_bits = 0;
        op->data_position++;
    }
    if (op->total_read_bits == op->bit_length) {
        printferr("No bits left to read.");
        return 0;
    }
    int bits_to_read;
    int mask;
    bits_to_read = 8 - (op->read_bits % 8);
    mask = 0x1 << (bits_to_read - 1);
    op->read_bits++;
    op->total_read_bits++;
    return (op->data[op->data_position] & mask ? 1 : 0);
}

int bitoper_walk_number(bitoper* op, int bit_len) {
    int out = 0;
    int i;
    for (i = bit_len; i > 0; i--) {
        if (bitoper_get_bit(op) == 1) {
            out += 0x1 << (i - 1);
        }
    }
    return out;
}
