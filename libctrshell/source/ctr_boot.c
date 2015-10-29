#include <3ds.h>

#include <stdio.h>
#include <string.h>

#include "ctr_shell.h"
#include "ctr_boot.h"
#include "ctr_utility.h"
#include "ctr_fileutils.h"

extern void (*__system_retAddr)(void);

static Handle ctr_hbFileHandle;
static u32 ctr_argbuffer[0x200];
static u32 ctr_argbuffer_length = 0;

// ninjhax 1.x
void (*ctr_callBootloader_1x)(Handle hb, Handle file);
void (*setArgs_1x)(u32* src, u32 length);

static void ctr_launchFile_1x(void)
{
	// jump to bootloader
	ctr_callBootloader_1x(0x00000000, ctr_hbFileHandle);
}

// ninjhax 2.0+
typedef struct
{
	int processId;
	bool capabilities[0x10];
}ctr_processEntry_s;

void (*ctr_callBootloader_2x)(Handle file, u32* argbuf, u32 arglength) = (void*)0x00100000;
void (*ctr_callBootloaderNewProcess_2x)(int processId, u32* argbuf, u32 arglength) = (void*)0x00100008;
void (*ctr_callBootloaderRunTitle_2x)(u8 mediatype, u32* argbuf, u32 argbuflength, u32 tid_low, u32 tid_high) = (void*)0x00100010;
void (*ctr_getBestProcess_2x)(u32 sectionSizes[3], bool* requirements, int num_requirements, ctr_processEntry_s* out, int out_size, int* out_len) = (void*)0x0010000C;

int ctr_targetProcessId = -1;
ctr_titleInfo_s ctr_target_title;

static void ctr_launchFile_2x(void)
{
	// jump to bootloader
	if(ctr_targetProcessId == -1)ctr_callBootloader_2x(ctr_hbFileHandle, ctr_argbuffer, ctr_argbuffer_length);
	else if(ctr_targetProcessId == -2)ctr_callBootloaderRunTitle_2x(ctr_target_title.mediatype, ctr_argbuffer, ctr_argbuffer_length, ctr_target_title.title_id & 0xffffffff, (ctr_target_title.title_id >> 32) & 0xffffffff);
	else ctr_callBootloaderNewProcess_2x(ctr_targetProcessId, ctr_argbuffer, ctr_argbuffer_length);
}

bool ctr_isNinjhax2(void)
{
	Result ret = hbInit();
	if(!ret)
	{
		hbExit();
		return false;
	}else return true;
}

int ctr_bootApp(const char* executablePath, ctr_executableMetadata_s* em)
{
	// open file that we're going to boot up
	fsExit();
	fsInit();

	FSUSER_OpenFileDirectly(NULL, &ctr_hbFileHandle, ctr_sdmcArchive, FS_makePath(PATH_CHAR, executablePath), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	fsExit();

	// set argv/argc
	ctr_argbuffer[0] = 0;
	ctr_argbuffer_length = 0x200*4;
	// TEMP
	// if(netloader_boot) {
	// 	char *ptr = netloaded_commandline;
	// 	char *dst = (char*)&argbuffer[1];
	// 	while (ptr < netloaded_commandline + netloaded_cmdlen) {
	// 		char *arg = ptr;
	// 		strcpy(dst,ptr);
	// 		ptr += strlen(arg) + 1;
	// 		dst += strlen(arg) + 1;
	// 		argbuffer[0]++;
	// 	}
	// }else{
		ctr_argbuffer[0]=1;
		snprintf((char*)&ctr_argbuffer[1], 0x200*4 - 4, "sdmc:%s", executablePath);
		ctr_argbuffer_length = strlen((char*)&ctr_argbuffer[1]) + 4 + 1; // don't forget null terminator !
	// }

	// figure out the preferred way of running the 3dsx
	if(!hbInit())
	{
		// ninjhax 1.x !
		// grab bootloader addresses
		HB_GetBootloaderAddresses((void**)&ctr_callBootloader_1x, (void**)&setArgs_1x);
		hbExit();

		// set argv
		setArgs_1x(ctr_argbuffer, 0x200*4);

		// override return address to homebrew booting code
		__system_retAddr = ctr_launchFile_1x;
	}else{
		// ninjhax 2.0+
		// override return address to homebrew booting code
		__system_retAddr = ctr_launchFile_2x;

		if(em)
		{
			if(em->scanned && ctr_targetProcessId == -1)
			{
				// this is a really shitty implementation of what we should be doing
				// i'm really too lazy to do any better right now, but a good solution will come
				// (some day)
				ctr_processEntry_s out[4];
				int out_len = 0;
				ctr_getBestProcess_2x(em->sectionSizes, (bool*)em->servicesThatMatter, NUM_SERVICESTHATMATTER, out, 4, &out_len);

				// temp : check if we got all the services we want
				if(em->servicesThatMatter[0] <= out[0].capabilities[0] && em->servicesThatMatter[1] <= out[1].capabilities[1] && em->servicesThatMatter[2] <= out[2].capabilities[2] && em->servicesThatMatter[3] <= out[3].capabilities[3])
				{
					ctr_targetProcessId = out[0].processId;
				}else{
					// temp : if we didn't get everything we wanted, we search for the candidate that has as many highest-priority services as possible
					int i, j;
					int best_id = 0;
					int best_sum = 0;
					for(i=0; i<out_len; i++)
					{
						int sum = 0;
						for(j=0; j<NUM_SERVICESTHATMATTER; j++)
						{
							sum += (em->servicesThatMatter[j] == 1) && out[i].capabilities[j];
						}

						if(sum > best_sum)
						{
							best_id = i;
							best_sum = sum;
						}
					}
					ctr_targetProcessId = out[best_id].processId;
				}

			}else if(ctr_targetProcessId != -1) ctr_targetProcessId = -2;
		}
	}
	ctr_shell_exit();
	return 0;
}
