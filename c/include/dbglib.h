#ifndef __DBGLIB_H
#define __DBGLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR   -1
#define OK       0

void dbglib_set_verbose(int);
void printfdbg(const char *, ...);
void printferr(const char *, ...);


#ifdef __cplusplus
}
#endif

#endif
