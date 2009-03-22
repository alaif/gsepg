#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C" {
#endif


#define TRUE  1
#define FALSE 0
typedef int bool;

// program exit codes
#define EXIT_SUCCESS     0
#define EXIT_ARGS        1
#define EXIT_ARGS_FILE   2
#define EXIT_MEM         3  // exiting due to system has not enough memory for malloc() etc.
// i18n
#define I18N_PACKAGE    "gsepg" 
#define I18N_LOCALEDIR  "./locale"
#define _(String) gettext (String)
#define gettext_noop(String) (String)
#define N_(String) gettext_noop (String)
// common io descriptors
#define FD_STDIN     0
#define FD_STDOUT    1
#define FD_STDERR    2

// EPG common
#define EPG_GETSTREAM_PID    0x12 // 18dec, PID used for EPG in getstream produced packets.
#define PAT_GETSTRAM_PID     0x0  // program association table PID
#define NIT_GETSTRAM_PID     0x10 // program association table PID



#ifdef __cplusplus
}
#endif

#endif
