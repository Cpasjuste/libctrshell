#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ctr_shell.h>
#include <ctr_utility.h>
#include <ctr_fileutils.h>

#ifdef _SYS
#define SHELL_PORT 4444
#else
#define SHELL_PORT 3333
#endif

int main(int argc, char **argv)
{
	// init ctr shell
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	ctr_shell_init(NULL, SHELL_PORT);

	// Main loop
	while (1)
	{
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START || kDown & KEY_B)
			break; // break in order to return to hbmenu

        if (kDown & KEY_A) {
            /*
            Result res = nsInit();
            if(res!=0) {
                ctr_shell_print("Err: nsInit\n");
                ctr_shell_print_res(res);
                continue;
            }
            u32 procid = 0;
            res = NS_LaunchTitle(0x0004000000980300, 0, &procid);
            if(res!=0) {
                ctr_shell_print("Err: NS_LaunchTitle\n");
                ctr_shell_print_res(res);
                continue;
            }
            ctr_shell_print("NS_LaunchTitle: procid=%i\n", procid);
            */
        }
        ctr_sleep(100);
        consoleClear();
        u32 ip = gethostid();
        printf("ip: %d.%d.%d.%d\n", (int)ip & 0xFF, (int)(ip>>8)&0xFF, (int)(ip>>16)&0xFF, (int)(ip>>24)&0xFF);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
	}

    gfxExit();
	ctr_shell_exit();

	return 0;
}
