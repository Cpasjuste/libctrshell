#include <3ds.h>
#include <string.h>
#include <stdio.h>

#include "ctr_utility.h"
#include "ctr_internal.h"

void ctr_sleep(u64 millis)
{
    svcSleepThread(millis*1000000LL);
}

bool ctr_endWith(const char *str, const char c)
{
    return (str && *str && str[strlen(str) - 1] == c) ? true : false;
}

void ctr_unicodeToChar(char* dst, u16* src, int max)
{
	if(!src || !dst)return;
	int n=0;
	while(*src && n<max-1){*(dst++)=(*(src++))&0xFF;n++;}
	*dst=0x00;
}
