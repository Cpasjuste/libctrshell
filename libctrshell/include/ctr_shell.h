#ifndef _ctr_shell_h_
#define _ctr_shell_h_

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

int ctr_shell_init(char *app_path, int port);
void ctr_shell_exit();
void ctr_shell_print(const char *fmt, ...);
void ctr_shell_print_res(Result res);
void ctr_shell_update();
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _ctr_shell_h_
