#ifndef _ctr_fileutils_h_
#define _ctr_fileutils_h_

#ifdef __cplusplus
extern "C" {
#endif

FS_archive ctr_sdmcArchive;

void ctr_smdcArchiveInit();
void ctr_smdcArchiveExit();
u64 ctr_file_size(const char *path);
bool ctr_file_exist(const char *file);
bool ctr_dir_exist(const char *path);
bool ctr_create_dir(const char *path);
bool ctr_create_dir_rec(const char *path);
bool ctr_del_dir(const char *path);
bool ctr_del_file(const char *path);
bool ctr_rename_dir(const char *src, const char *dst);
bool ctr_rename_file(const char *src, const char *dst);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _ctr_fileutils_h_
