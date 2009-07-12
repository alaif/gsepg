/* -*- coding: UTF-8 -*-
 * dvbchar.c
 *
 * Module contains functions for decoding "coded character sets for text communication ISO6937/2".
 * European characters are decoded at the moment.
 *
 * This module should be compiled with -fexec-charset=utf-8 gcc parameter. (see makefile)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "../include/common.h"
#include "../include/dbglib.h"
#include "../include/dvbchar.h"

// Private variables holding mapping to unicode chars
dvbchar_mapping acutes[25];
dvbchar_mapping carons[18];
dvbchar_mapping rings[4];

wchar_t dvbchar_get_ring(unsigned char letter) {
    wchar_t out = L'\0';
    switch (letter) {
        case 'u': out = L'ů'; break;
        case 'U': out = L'Ů'; break;
    }
    return out;
}

wchar_t dvbchar_get_caron(unsigned char letter) {
    wchar_t out = L'\0';
    switch (letter) {
        case 'C': out = L'Č'; break;
        case 'D': out = L'Ď'; break;
        case 'E': out = L'Ě'; break;
        case 'N': out = L'Ň'; break;
        case 'R': out = L'Ř'; break;
        case 'S': out = L'Š'; break;
        case 'T': out = L'Ť'; break;
        case 'Z': out = L'Ž'; break;
        case 'c': out = L'č'; break;
        case 'd': out = L'ď'; break;
        case 'e': out = L'ě'; break;
        case 'n': out = L'ň'; break;
        case 'r': out = L'ř'; break;
        case 's': out = L'š'; break;
        case 't': out = L'ť'; break;
        case 'z': out = L'ž'; break;
    }
    return out;
}

wchar_t dvbchar_get_acute(unsigned char letter) {
    wchar_t out = L'.';
    switch (letter) {
        case 'A': out = L'Á'; break;
        case 'E': out = L'É'; break;
        case 'I': out = L'Í'; break;
        case 'O': out = L'Ó'; break;
        case 'U': out = L'Ú'; break;
        case 'Y': out = L'Ý'; break;
        case 'a': out = L'á'; break;
        case 'e': out = L'é'; break;
        case 'i': out = L'í'; break;
        case 'o': out = L'ó'; break;
        case 'u': out = L'ú'; break;
        case 'y': out = L'ý'; break;
    }
    return out;
}

wchar_t dvbchar_get_umlaut(unsigned char letter) {
    wchar_t out = L'.';
    switch (letter) {
        case 'A': out = L'Ä'; break;
        case 'E': out = L'Ë'; break;
        case 'O': out = L'Ö'; break;
        case 'U': out = L'Ü'; break;
        case 'a': out = L'ä'; break;
        case 'e': out = L'ë'; break;
        case 'o': out = L'ö'; break;
        case 'u': out = L'ü'; break;
    }
    return out;
}

bool dvbchar_can_output(unsigned char byte) {
    if (
        iscntrl(byte) ||
        byte == '{' ||
        byte == '}' ||
        byte == '[' ||
        byte == ']'
    ) 
        return FALSE;
    if (
        isalnum(byte) ||
        isspace(byte) ||
        ispunct(byte) /* ispuct does not avoid control chars like 0x12 */
    ) 
        return TRUE;
    return FALSE;
}

// FIXME Function considers czech literals only (few accents).
// Decodes characters from src and places decoded string to dest.
// dest parameter has to be large engough (same length as src is most safe).
bool dvbchar_decode(const unsigned char *src, wchar_t *dest, int len) {
    int i, d = 0;
    bool acute = FALSE;
    bool caron = FALSE;
    bool ring = FALSE;
    bool umlaut = FALSE;
    for (i = 0; i < len; i++) {
        if (acute) {
            dest[d] = dvbchar_get_acute(src[i]);
            d++;
        } else if (caron) {
            dest[d] = dvbchar_get_caron(src[i]);
            d++;
        } else if (ring) {
            dest[d] = dvbchar_get_ring(src[i]);
            d++;
        } else if (umlaut) {
            dest[d] = dvbchar_get_umlaut(src[i]);
            d++;
        }
        if (acute || caron || ring || umlaut) {
            acute =  caron = ring =  umlaut = FALSE;
        } else {
            switch (src[i]) {
                case ISO6937_ACUTE:
                    acute = TRUE;
                    break;
                case ISO6937_CARON:
                    caron = TRUE;
                    break;
                case ISO6937_RING:
                    ring = TRUE;
                    break;
                case ISO6937_UMLAUT:
                    umlaut = TRUE;
                    break;
                case '"': // escape \" char
                    dest[d] = '\\'; d++;
                    dest[d] = '"'; d++;
                    break;
                case '\\': // escape \\ char
                    dest[d] = '\\'; d++;
                    dest[d] = '\\'; d++;
                    break;
                default:
                    if (!dvbchar_can_output(src[i])) {
                        dest[d] = '.';
                        d++; 
                        break;
                    }
                    dest[d] = src[i];
                    d++;
            }
        }
    }
    dest[d] = L'\0';
    return TRUE;
}
