#ifndef _ctr_title_h_
#define _ctr_title_h_

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t char16_t;

typedef enum {
	NAND = 0,
	SD = 1
} MediaType;

typedef enum {
	LANGUAGE_JAPANESE = 0,
	LANGUAGE_ENGLISH = 1,
	LANGUAGE_FRENCH = 2,
	LANGUAGE_GERMAN = 3,
	LANGUAGE_ITALIAN = 4,
	LANGUAGE_SPANISH = 5,
	LANGUAGE_SIMPLIFIED_CHINESE = 6,
	LANGUAGE_KOREAN = 7,
	LANGUAGE_DUTCH = 8,
	LANGUAGE_PORTUGUESE = 9,
	LANGUAGE_RUSSIAN = 10,
	LANGUAGE_TRADITIONAL_CHINESE = 11
} SMDHLanguage;

typedef enum {
	RATING_CERO = 0,
	RATING_ESRB = 1,
	RATING_USK = 3,
	RATING_PEGI_GEN = 4,
	RATING_PEGI_PRT = 6,
	RATING_PEGI_BBFC = 7,
	RATING_COB = 8,
	RATING_GRB = 9,
	RATING_CGSRR = 10
} SMDHRating;

typedef enum {
	FLAG_VISIBILITY = 0x0001,
	FLAG_AUTO_BOOT = 0x0002,
	FLAG_ALLOW_3D = 0x0004,
	FLAG_REQUIRE_EULA = 0x0008,
	FLAG_SAVE_ON_EXIT = 0x0010,
	FLAG_USE_EXTENDED_BANNER = 0x0020,
	FLAG_RATING_REQUIRED = 0x0040,
	FLAG_USE_SAVE_DATA = 0x0080,
	FLAG_RECORD_USAGE = 0x0100,
	FLAG_DISABLE_SAVE_BACKUPS = 0x0400
} SMDHFlag;

 typedef enum {
	REGION_JAPAN = 0x01,
	REGION_NORTH_AMERICA = 0x02,
	REGION_EUROPE = 0x04,
	REGION_AUSTRALIA = 0x08,
	REGION_CHINA = 0x10,
	REGION_KOREA = 0x20,
	REGION_TAIWAN = 0x40,
	REGION_FREE = 0x7FFFFFFF
} SMDHRegion;

typedef struct {
	char16_t shortDescription[0x40];
	char16_t longDescription[0x80];
	char16_t publisher[0x40];
} SMDHTitle;

typedef struct {
	u8 ratings[0x10];
	u32 region;
	u32 matchMakerId;
	u64 matchMakerBitId;
	u32 flags;
	u16 eulaVersion;
	u16 reserved;
	u32 optimalBannerFrame;
	u32 streetpassId;
} SMDHSettings;

typedef struct {
	char magic[0x04];
	u16 version;
	u16 reserved1;
	SMDHTitle titles[0x10];
	SMDHSettings settings;
	u64 reserved2;
	u8 smallIcon[0x480];
	u8 largeIcon[0x1200];
} SMDH;

typedef struct {
	u64 titleId;
	u32 uniqueId;
	char productCode[16];
	MediaType mediaType;
	u16 version;
	u64 size;
	char shortDescription[512];
	char longDescription[512];
	char publisher[512];
} CIA;

SMDH ctr_smdh(const char *ciaFile);

u32 ctr_title_count(MediaType mediaType);
void ctr_titleid_list(MediaType mediaType, u32 titleCount, u64 *titleIds);
void ctr_title_list(MediaType mediaType, u32 titleCount, u64 *titleIds, TitleList *titleList);
bool ctr_title_exist(MediaType mediaType, u64 titleId);
bool ctr_title_delete(MediaType mediaType, u64 titleId);
bool ctr_title_exec(MediaType mediaType, u64 titleId);
int ctr_install_cia(const char *path);
CIA ctr_cia_info(const char *file, SMDHLanguage lang);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _ctr_title_h_
