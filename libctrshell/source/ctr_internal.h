#ifndef _ctr_internal_h_
#define _ctr_internal_h_

#include <3ds.h>

#define MAX_LINES ((240-8)/8)
#define __swap16(v) ((((v) & 0xFF) << 8) | ((v) >> 8))
#define GET_BITS(v, s, e) (((v) >> (s)) & ((1 << ((e) - (s) + 1)) - 1))

#define CTR_BUFSIZE 64*1024

//u64 ctr_str_to_u64(char *str);
void ctr_chopN(char *str, size_t n);
int ctr_recvall(int sock, void *buffer, int size, int flags);
void ctr_debug(const char *fmt, ...);
void ctr_debug_error(Result res, const char *fmt, ...);

#endif // _ctr_internal_h_
