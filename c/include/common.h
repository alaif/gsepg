#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define I18N_PACKAGE    "gsepg" 
#define I18N_LOCALEDIR  "./locale"
#define FD_STDIN     0
#define FD_STDOUT    1
#define _(String) gettext (String)
#define gettext_noop(String) (String)
#define N_(String) gettext_noop (String)



#ifdef __cplusplus
}
#endif

#endif
