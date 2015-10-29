#include <3ds.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#include "ctr_shell.h"
#include "ctr_internal.h"

/*
u64 ctr_str_to_u64(char *str)
{
	return strtoull(str, NULL, 16);
}
*/

void ctr_chopN(char *str, size_t n)
{
    if(n==0 || str==0) return ;

    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

int ctr_recvall(int sock, void *buffer, int size, int flags)
{
	int len, sizeleft = size;

	while (sizeleft)
	{
		len = recv(sock,buffer,sizeleft,flags);

		if (len == 0)
		{
			size = 0;
			break;
		};

		if (len == -1)
		{
			if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				ctr_debug("recvall %s\n", strerror(errno));
				break;
			}
		}
		else
		{
			sizeleft -=len;
			buffer +=len;
		}
	}
	return size;
}

void ctr_debug_error(Result res, const char *fmt, ...)
{
	char s[512];
	va_list args;
	va_start( args, fmt );
	vsprintf( s, fmt, args );
	va_end( args );

	ctr_shell_print("%s: %08lx\n", s, res);
	ctr_shell_print("%s: module: %i\n", s, GET_BITS((u32)res, 10, 17));
	ctr_shell_print("%s: summary: %i\n", s, GET_BITS((u32)res, 21, 26));
	ctr_shell_print("%s: description: %i\n", s, GET_BITS((u32)res, 0, 9));
}

void ctr_debug(const char *fmt, ...)
{
#ifdef DEBUG
	char s[512];
	va_list args;
	va_start( args, fmt );
	vsprintf( s, fmt, args );
	va_end( args );
	printf(s);
#endif
}
