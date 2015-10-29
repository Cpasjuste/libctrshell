#ifndef BOOT_H
#define BOOT_H

#include <3ds.h>
#include "smdh.h"

#define NUM_SERVICESTHATMATTER 4

typedef struct
{
	u8 mediatype;
	u64 title_id;
	smdh_s* icon;
}ctr_titleInfo_s;

typedef struct
{
	bool scanned;
	u32 sectionSizes[3];
	u8 servicesThatMatter[NUM_SERVICESTHATMATTER];
}ctr_executableMetadata_s;

extern int ctr_targetProcessId;
extern ctr_titleInfo_s target_title;

bool ctr_isNinjhax2(void);
int ctr_bootApp(const char* executablePath, ctr_executableMetadata_s* em);

#endif
