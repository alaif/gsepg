/*
 * wstring.c
 *
 * Jonas Fiala
 *
 * Knihovna pro praci s retezci menici podle potreby velikost.
 * Knihovna vznikla z duvodu nemoznosti zkompilovat glib 
 * pod SCO UNIX starsiho data vyroby.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "../include/wstring.h"

#define WSTR_MIN_MEMORY 8
#define WSTR_BUFFER_LENGTH 64536


int wstr_limit(int size) {
	int counter, zero;

	size = abs(size);
	if (size < WSTR_MIN_MEMORY) {
		return WSTR_MIN_MEMORY;
	}
	counter = 0;
	zero = 1;
	while (size) {
		counter++;
		if ((size > 1) && (size & 1)) {
			zero = 0;
		}
		size >>= 1;
	}
	return 1 << (counter - zero);
}

void wstr_info(WString* str) {
	if (str == NULL) {
		printf("NULL\n");
	} else if (str->str == NULL) {
		printf("[NULL] len=%d mem=%d\n", str->len, str->mem);
	} else {
		printf("[%s] len=%d mem=%d\n", str->str, str->len, str->mem);
	}
}

WString* wstr_new_size(int size) {
	WString* s;

	size = abs(size);
	s = (WString*) malloc(sizeof(WString));
	if (s == NULL) {
		fprintf(stderr, "Cannot malloc memory in wstr_new_size\n");
		exit(1);
	}
	s->len = 0;
	s->mem = wstr_limit(size + 1);
	s->str = (char*) malloc(s->mem);
	s->str[0] = '\0';
	return s;
}

WString* wstr_new(char *init) {
	WString* s;
	int len;

	len = strlen(init);
	s = wstr_new_size(len);
	strcpy(s->str, init);
	s->len = len;
	return s;
}

WString* wstr_resize(WString* str, int size) {
	 void *p;

	 size = wstr_limit(size);
	 if (size < str->len) {
	 	str->len = size;
	 	str->str[size] = '\0';
	 }
	 p = realloc((void*) str->str, size);
	 if (p == NULL) {
		 fprintf(stderr, "Cannot reallocate memory in wstr_resize\n");
		 exit(1);
	 }
	 str->mem = size;
	 str->str = (char*) p;
	 return str;
}

void wstr_correct(WString *str) {
	if ((str->mem > WSTR_MIN_MEMORY) && (str->mem / 4 > str->len + 1)) {
		wstr_resize(str, str->len);
	}
}

WString* wstr_append(WString* str, char* append) {
	int len;
	len = str->len + strlen(append) + 1; // bude se to pridavat jako string, takze 2 prvkovy pole
	str->len = len;
	if (len > str->mem) {
		wstr_resize(str, len);
	}
	strcat(str->str, append);
	return str;
}

WString* wstr_append_char(WString* str, int chr) {
	char tmp[2];

	tmp[0] = chr;
	tmp[1] = '\0';
	return wstr_append(str, tmp);
}

WString* wstr_assign(WString* str, char* assign) {
	int len;

	len = strlen(assign);
	if (len + 1> str->mem) {
		wstr_resize(str, len);
	}
	strcpy(str->str, assign);
	str->len = len;
	wstr_correct(str);
	return str;
}

WString* wstr_sprintf(WString* str, const char* format, ...) {
	va_list ap;
	char buff[WSTR_BUFFER_LENGTH];

	va_start(ap, format);
	vsnprintf (buff, WSTR_BUFFER_LENGTH, format, ap);
	va_end(ap);
	wstr_assign(str, buff);
	return str;
}

WString* wstr_truncate(WString *str, int len) {
	len = abs(len);
	if (len >= str->len) {
		return str;
	}
	str->str[len] = '\0';
	str->len = len;
	wstr_correct(str);
	return str;
}

WString* wstr_lshift(WString* str, int shift) {
	int low, high;

	shift = abs(shift);
	if (shift > str->len) {
		return wstr_truncate(str, 0);
	}
	for (low = 0, high = shift; high < str->len; low++, high++) {
		str->str[low] = str->str[high];
	}
	str->len -= shift;
	str->str[str->len] = '\0';
	wstr_correct(str);
	return str;
}

WString* wstr_rshift(WString* str, int shift, int fill) {
	int newlen, high, low, i;

	shift = abs(shift);
	newlen = str->len + shift;
	if (str->mem < newlen + 1) {
		wstr_resize(str, newlen + 1);
	}
	low = str->len - 1;
	high = low + shift;
	for (i = 0; i < shift && low >= 0; i++, low--, high--) {
		str->str[high] = str->str[low];
	}
	str->str[newlen] = '\0';
	str->len = newlen;
	memset((void*) str->str, fill, shift);
	return str;
}

WString* wstr_rtrim(WString* str) {
	int offset;

	for (offset = strlen(str->str) - 1; offset >= 0; offset--) {
		if (!isspace(str->str[offset])) {
			offset++;
			break;
		}
	}
	if (offset != str->len) {
		wstr_truncate(str, offset);
	}
	return str;
}

WString* wstr_ltrim(WString* str) {
	int offset;

	for (offset = 0; offset < str->len; offset++) {
		if (!isspace(str->str[offset])) {
			break;
		}
	}
	if (offset > 0) {
		wstr_lshift(str, offset);
	}
	return str;
}

WString* wstr_trim(WString* str) {
	wstr_rtrim(str);
	wstr_ltrim(str);
	return str;
}

WString* wstr_fill(WString* str, int chr, int length) {
	length = abs(length);
	if (length + 1 > str->len) {
		wstr_resize(str, length);
	}
	memset((void*) str->str, chr, length);
	str->len = length;
	str->str[length] = '\0';
	return str;
}

void wstr_free(WString* str) {
	free((void*) str->str);
	str->str = NULL;
	str->len = 0;
	str->mem = 0;
}

void wstr_free_total(WString** str) {
	wstr_free(*str);
	free((void*) *str);
	*str = NULL;
}


// EOF
