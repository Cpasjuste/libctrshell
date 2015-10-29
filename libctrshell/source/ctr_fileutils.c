#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ctr_fileutils.h"
#include "ctr_utility.h"
#include "ctr_internal.h"

void ctr_smdcArchiveInit()
{
    ctr_sdmcArchive = (FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 0, NULL}};
	FSUSER_OpenArchive(NULL, &ctr_sdmcArchive);
}

void ctr_smdcArchiveExit()
{
    FSUSER_CloseArchive(NULL, &ctr_sdmcArchive);
}

bool ctr_file_exist(const char *file)
{
    Handle fileHandle;
	FS_path filePath=FS_makePath(PATH_CHAR, file);
	bool exist = FSUSER_OpenFileDirectly(
                    NULL, &fileHandle, ctr_sdmcArchive,
                    filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE) == 0;
    if(exist) {
        FSFILE_Close(fileHandle);
    }
    return exist;
}

bool ctr_dir_exist(const char *path)
{
    if(path==NULL || strlen(path)<=0)
        return false;

	struct stat st;
	stat(path, &st);
	return S_ISDIR(st.st_mode);
}

u64 ctr_file_size(const char *path)
{
    struct stat st;
    stat(path, &st);
    return st.st_size;
}

bool ctr_create_dir(const char *path)
{
    FS_path filePath=FS_makePath(PATH_CHAR, path);
    return FSUSER_CreateDirectory(NULL, ctr_sdmcArchive, filePath) == 0;
}

bool ctr_create_dir_rec(const char *path)
{
    char tmp[512];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",path);
    len = strlen(tmp);
    if(tmp[len - 1] == '/')
            tmp[len - 1] = 0;
    for(p = tmp + 1; *p; p++)
        if(*p == '/') {
        *p = 0;
        if(!ctr_create_dir(tmp))
            return false;
        *p = '/';
    }
    return ctr_create_dir(tmp);
}

bool ctr_del_dir(const char *path)
{
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	return FSUSER_DeleteDirectoryRecursively(NULL,ctr_sdmcArchive,filePath) == 0;
}

bool ctr_rename_dir(const char *src, const char *dst)
{
	FS_path filePath=FS_makePath(PATH_CHAR, src);
	FS_path filePath2=FS_makePath(PATH_CHAR, dst);
	return FSUSER_RenameDirectory(NULL,ctr_sdmcArchive,filePath,ctr_sdmcArchive,filePath2) == 0;
}

bool ctr_del_file(const char *path)
{
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	return FSUSER_DeleteFile(NULL,ctr_sdmcArchive,filePath)  == 0;
}

bool ctr_rename_file(const char *src, const char *dst)
{
	FS_path filePath=FS_makePath(PATH_CHAR, src);
	FS_path filePath2=FS_makePath(PATH_CHAR, dst);
	return FSUSER_RenameFile(NULL,ctr_sdmcArchive,filePath,ctr_sdmcArchive,filePath2) == 0;
}

