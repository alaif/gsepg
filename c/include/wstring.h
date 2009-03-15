#ifndef __WSTRING_H
#define __WSTRING_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct{
	char* str;
	int len;
	int mem;
} WString;

WString* wstr_new_size(int size);
WString* wstr_new(char *init);
WString* wstr_resize(WString* str, int size);
WString* wstr_append(WString* str, char* append);
WString* wstr_append_char(WString* str, int chr);
WString* wstr_assign(WString* str, char* assign);
WString* wstr_sprintf(WString* str, const char* format, ...);
WString* wstr_truncate(WString *str, int len);
WString* wstr_lshift(WString* str, int shift);
WString* wstr_rshift(WString* str, int shift, int fill);
WString* wstr_rtrim(WString* str);
WString* wstr_ltrim(WString* str);
WString* wstr_trim(WString* str);
WString* wstr_fill(WString* str, int chr, int length);
void wstr_free(WString* str);
void wstr_free_total(WString** str);
void wstr_info(WString*);

#ifdef __cplusplus
}
#endif

#endif
