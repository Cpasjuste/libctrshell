#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>

#include <ctr_shell.h>
#include <ctr_title.h>
#include <ctr_utility.h>
#include <ctr_fileutils.h>
#include "ctr_internal.h"

SMDH ctr_smdh(const char *ciaFile)
{
	SMDH smdh;

    FILE* fd = fopen(ciaFile, "rb");
    if(!fd)
	{
		printf("Err: ctr_ciaSMDH: fopen(%s)", ciaFile);
        return smdh;
    }

    if(fseek(fd, -0x36C0, SEEK_END) != 0)
	{
        fclose(fd);
		printf("Err: ctr_ciaSMDH: fseek");
        return smdh;
    }

    size_t bytesRead = fread(&smdh, sizeof(SMDH), 1, fd);
    if(bytesRead < 0)
	{
        fclose(fd);
		printf("Err: ctr_ciaSMDH: fread");
		return smdh;
    }

    fclose(fd);

    if(smdh.magic[0] != 'S' || smdh.magic[1] != 'M' || smdh.magic[2] != 'D' || smdh.magic[3] != 'H') {
		printf("Err: ctr_ciaSMDH: invalid magic");
		return smdh;
    }
    return smdh;
}

u32 ctr_title_count(MediaType mediaType)
{
	u32 count = 0;
	Result res = AM_GetTitleCount(mediaType, &count);
    if(res != 0)
	{
		ctr_debug_error(res, "AM_GetTitleCount" );
    }
    return count;
}

void ctr_titleid_list(MediaType mediaType, u32 titleCount, u64 *titleIds)
{
	Result res = AM_GetTitleIdList(mediaType, titleCount, titleIds);
    if(res != 0)
	{
		ctr_debug_error(res, "AM_GetTitleIdList" );
    }
}

void ctr_title_list(MediaType mediaType, u32 titleCount, u64 *titleIds, TitleList *titleList)
{
	Result res = AM_ListTitles(mediaType, titleCount, titleIds, titleList);
    if(res != 0)
	{
		ctr_debug_error(res, "AM_ListTitles" );
    }
}

bool ctr_title_exist(MediaType mediaType, u64 titleId)
{
	u32 titleCount = ctr_title_count(mediaType);
	if(titleCount <= 0) return false;

    u64 titleIds[titleCount];
	ctr_titleid_list(mediaType, titleCount, titleIds);
	if(titleIds == NULL) return false;

	u32 i;
	for(i = 0; i < titleCount; i++)
	{
		if(titleIds[i] == titleId)
		{
			return true;
		}
	}
	return false;
}

bool ctr_title_delete(MediaType mediaType, u64 titleId)
{
	Result res;

	// System app
	if(titleId>>32 & 0xFFFF)
	{
        res = AM_DeleteTitle(mediaType, titleId);
        if(res != 0)
        {
            ctr_debug_error(res, "AM_DeleteTitle" );
            return false;
        }
    }
    else
    {
        res = AM_DeleteAppTitle(mediaType, titleId);
        if(res != 0)
        {
            ctr_debug_error(res, "AM_DeleteAppTitle" );
            return false;
        }
    }
    return true;
}


bool ctr_title_exec(MediaType mediaType, u64 titleId)
{
    aptOpenSession();

	Result res = APT_PrepareToDoAppJump(NULL, 0, titleId, mediaType);
    if(res != 0)
	{
		ctr_debug_error(res, "APT_PrepareToDoAppJump" );
		aptCloseSession();
		return false;
    }

	res = APT_DoAppJump(NULL, 0, 0, NULL, NULL);
    if(res != 0)
	{
		ctr_debug_error(res, "APT_DoAppJump" );
		aptCloseSession();
		return false;
    }
    aptCloseSession();
    return true;
}

TitleList ctr_cia_info_fast(const char *path)
{
    TitleList titleInfo;

    Handle handle = 0;
    Result openResult = FSUSER_OpenFile(NULL, &handle, ctr_sdmcArchive, FS_makePath(PATH_CHAR, path), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
    if(openResult != 0)
	{
		ctr_debug_error(openResult, "FSUSER_OpenFile" );
        return titleInfo;
    }

    Result infoResult = AM_GetCiaFileInfo(mediatype_SDMC, &titleInfo, handle);
    if(infoResult != 0)
	{
		ctr_debug_error(infoResult, "AM_GetCiaFileInfo" );
    }
    FSFILE_Close(handle);
    return titleInfo;
}

CIA ctr_cia_info(const char *file, SMDHLanguage lang)
{
	CIA cia;
	cia.titleId = 0;

    TitleList titleInfo = ctr_cia_info_fast(file);
    if(titleInfo.titleID <=0)
    {
        ctr_shell_print("Oups, ctr_cia_info_fast failed...\n");
        return cia;
    }

	cia.titleId = titleInfo.titleID;
	cia.uniqueId = (u32)titleInfo.titleID;
	strcpy(cia.productCode, "<N/A>");
	cia.mediaType = mediatype_SDMC;
	cia.version = titleInfo.titleVersion;
	cia.size = titleInfo.size;

	SMDH smdh = ctr_smdh(file);

	if(smdh.titles!=NULL)
	{
		char t[512];
		if(smdh.titles[(int)lang].shortDescription != NULL)
		{
			memset(t, 0, 512);
			ctr_unicodeToChar(t, smdh.titles[(int)lang].shortDescription, 512);
			strcpy(cia.shortDescription, t);
		}
		if(smdh.titles[(int)lang].longDescription != NULL)
		{
			memset(t, 0, 512);
			ctr_unicodeToChar(t, smdh.titles[(int)lang].longDescription, 512);
			strcpy(cia.longDescription, t);
		}
		if(smdh.titles[(int)lang].publisher != NULL)
		{
			memset(t, 0, 512);
			ctr_unicodeToChar(t, smdh.titles[(int)lang].publisher, 512);
			strcpy(cia.publisher, t);
		}
	}
	return cia;
}

int ctr_install_cia(const char *path)
{
	Handle fileHandle, ciaHandle = NULL;
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	Result res = FSUSER_OpenFileDirectly(NULL, &fileHandle, ctr_sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if (res != 0)
	{
		ctr_debug_error(res, "FSUSER_OpenFileDirectly");
		return -1;
	}

    char cia_buffer[CTR_BUFSIZE];
    memset(cia_buffer, 0, CTR_BUFSIZE);
    u64 size, bytesToRead, i = 0; u32 bytes;
	FSFILE_GetSize(fileHandle, &size);

	res = AM_StartCiaInstall(mediatype_SDMC, &ciaHandle);
	if (res != 0)
	{
		ctr_debug_error(res, "AM_StartCiaInstall");
		return -1;
	}

    while (i < size)
    {
        if	(i+CTR_BUFSIZE > size)
            bytesToRead = size - i;
        else
            bytesToRead = CTR_BUFSIZE;

        res = FSFILE_Read(fileHandle, &bytes, i, cia_buffer, bytesToRead);
        ctr_debug("bytes: r=%u ... ", bytes );
        if (res != 0)
        {
            AM_CancelCIAInstall(&ciaHandle);
            FSFILE_Close(fileHandle);
            ctr_debug_error(res, "FSFILE_Read");
            return -1;
        }

        res = FSFILE_Write(ciaHandle, &bytes, i, cia_buffer, bytesToRead, FS_WRITE_NOFLUSH);
        ctr_debug("w=%u\n", bytes );
        if (res != 0 && res != 0xc8e083fc)// 0xc8e083fc=ALREADY_EXISTS
        {
            AM_CancelCIAInstall(&ciaHandle);
            FSFILE_Close(fileHandle);
            ctr_debug_error(res, "FSFILE_Write");
            return -1;
        }

        i += bytesToRead;
        ctr_shell_print("ctr_install_cia: %llu/%llu\n", i, size);
	}

	res = AM_FinishCiaInstall(mediatype_SDMC, &ciaHandle);
	if (res != 0)
	{
        AM_CancelCIAInstall(&ciaHandle);
		ctr_debug_error(res, "AM_FinishCiaInstall");
		return -1;
	}

	FSFILE_Close(fileHandle);
	return 0;
}
