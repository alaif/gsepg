#ifndef __DVBCHAR_H
#define __DVBCHAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>

#define ISO6937_GRAVE   0xc1
#define ISO6937_ACUTE   0xc2
#define ISO6937_CIRCUM  0xc3   // circumflex
#define ISO6937_TILDE   0xc4
#define ISO6937_MACRON  0xc5
#define ISO6937_BREVE   0xc6
#define ISO6937_DOT     0xc7
#define ISO6937_UMLAUT  0xc8
#define ISO6937_RING    0xca
#define ISO6937_CEDIL   0xcb  // cedilla
#define ISO6937_DACUTE  0xcd  //double acute
#define ISO6937_OGONEK  0xce
#define ISO6937_CARON   0xcf

typedef struct {
    char letter;
    wchar_t wletter;
} dvbchar_mapping;

bool dvbchar_decode(const unsigned char *src, wchar_t *dest, int len);

#ifdef __cplusplus
}
#endif

#endif
