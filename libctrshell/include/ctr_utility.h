#ifndef _ctr_utility_h_
#define _ctr_utility_h_

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

void ctr_sleep(u64 millis);
bool ctr_endWith(const char *str, const char c);
void ctr_unicodeToChar(char* dst, u16* src, int max);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _ctr_utility_h_
