#ifndef __DVBCRC32_H
#define __DVBCRC32_H

#ifdef __cplusplus
extern "C" {
#endif

//uint32_t dvb_crc32(const uint8_t *data, size_t len);
long dvb_crc32(const long *data, size_t len);


#ifdef __cplusplus
}
#endif

#endif
