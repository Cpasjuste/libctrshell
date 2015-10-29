#include <3ds.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ctr_shell.h"
#include "ctr_utility.h"
#include "ctr_fileutils.h"
#include "ctr_boot.h"
#include "ctr_title.h"
#include "ctr_internal.h"

// socks
void *ctr_soc_buffer = NULL;
int ctr_listen_socket;
int ctr_sockfd = -1;
int ctr_shell_port = 3333;

// threads
#define CTR_STACK (128*1024)
Handle ctr_threadHandle;
volatile bool ctr_threadExit = false;
u32 *ctr_threadStack;

// stuff
bool ctr_is_cia = false;
int ctr_running = 0;
char ctr_now_path[512];
char ctr_app_path[512];

typedef struct {
    int type;
    char args[4][512];
} cmd_s;
cmd_s *cmd_buf;

enum cmd_t {
	CMD_NONE=0,
	CMD_LS,
	CMD_CD,
	CMD_RM,
	CMD_RMDIR,
	CMD_MKDIR,
	CMD_MV,
	CMD_PWD,
	CMD_TITLE_LIST,
	CMD_TITLE_INFO,
	CMD_TITLE_DEL,
	CMD_TITLE_INSTALL,
	CMD_TITLE_EXEC,
	CMD_TITLE_SEND,
	CMD_PUT,
	CMD_MEMR,
	CMD_MEMW,
	CMD_MENU,
	CMD_RESET,
	CMD_EXIT,
	CMD_HELP
};

void ctr_recovery()
{
    // recovery
    int sleep = 0;
    bool recovery = false;
	while (aptMainLoop())
	{
		hidScanInput();
		u32 k = hidKeysHeld();
		if (k & KEY_L) {
            recovery = true;
		}
		if (k & KEY_B) {
            recovery = false;
		}
        ctr_sleep(500);
        sleep++;
        if(sleep>1 && !recovery) {
            break;
        }
	}
}

void ctr_abs_path(char* inPath)
{
    if(inPath==NULL || strlen(inPath) < 1)
		strncpy(inPath, ctr_now_path, 512);

    if(inPath[0]=='/')
        return;

    if(!ctr_endWith(ctr_now_path, '/'))
        strcat(ctr_now_path, "/");

    char tmp[512];
    snprintf(tmp, 512, "%s%s", ctr_now_path, inPath);

	if(strlen(tmp)>1 && ctr_endWith(tmp, '/'))
		tmp[strlen(tmp)-1] = '\0';
    strncpy(inPath, tmp, 512);
}

void ctr_shell_print(const char *fmt, ...)
{
	if(ctr_sockfd >= 0)
	{
		char s[512];
		memset(s, 0, 512);
		va_list args;
		va_start( args, fmt );
		int size = vsprintf( s, fmt, args );
		va_end( args );
		send(ctr_sockfd, s, size, 0);
	}
}

void ctr_shell_print_res(Result res)
{
	ctr_shell_print("result: %08lx\n", res);
	ctr_shell_print("result: module: %i\n", GET_BITS((u32)res, 10, 17));
	ctr_shell_print("result: summary: %i\n", GET_BITS((u32)res, 21, 26));
	ctr_shell_print("result: description: %i\n", GET_BITS((u32)res, 0, 9));
}

void cmd_put_file(int size, char *dest)
{
    ctr_abs_path(dest);

	ctr_debug("cmd_put_file: dest=%s, size=%i\n", dest, size);

    /* BUG ?!
	char* path = strrchr(dest, '/');
	if (path)
	{
		*path = '\0';
	}
	ctr_debug("parentDir: %s\n", path);
	if(ctr_create_dir_rec(path) != 0)
	{
		ctr_debug("Failed to create dir: %s\n", strerror(errno));
	}
	*/

	FILE *file = fopen(dest, "w");
	if (file == NULL)
	{
		ctr_debug("Failed to open file: %s\n", strerror(errno));
		return;
	}

	int bufSize = CTR_BUFSIZE;
	char buffer[bufSize];
	int remain_data = size;
	int progress = 0;
	ssize_t len;

	while (remain_data > 0)
	{
		if(remain_data<bufSize) bufSize = remain_data;
		len = ctr_recvall(ctr_sockfd, buffer, bufSize, 0);
		fwrite(buffer, sizeof(char), len, file);
		remain_data -= len;
		progress += len;
		//ctr_debug("%d/%d bytes\n", progress, size);
		ctr_shell_print("Received: %d/%d\n", progress, size);
	}
	fclose(file);
	ctr_debug("cmd_put_file: end\n");
	ctr_shell_print("File received (%d/%d bytes)\n", progress, size);
}

void cmd_ls(char *path)
{
    ctr_abs_path(path);

	if(!ctr_dir_exist(path))
	{
		ctr_shell_print("Path not found: %s\n", path);
		return;
	}

	ctr_shell_print("Listing directory: %s\n\n", path );

	Handle dirHandle;
	FS_path dirPath=FS_makePath(PATH_CHAR, path);
	FSUSER_OpenDirectory(NULL, &dirHandle, ctr_sdmcArchive, dirPath);

	char fullPath[1024];
	u32 entriesRead;
	u32 entries = 0;

	for(;;)
	{
		FS_dirent entry;
		memset(&entry,0,sizeof(FS_dirent));
		entriesRead=0;
		FSDIR_Read(dirHandle, &entriesRead, 1, &entry);

		if(!entriesRead) break;

		if(!ctr_endWith(path, '/'))
			snprintf(fullPath, 1024, "%s/", path);
		else
			snprintf(fullPath, 1024, "%s", path);

		int n=strlen(fullPath);
		ctr_unicodeToChar(&fullPath[n], entry.name, 1024-n);
		if(entry.isDirectory) //directories
		{
			ctr_shell_print("%s\n", fullPath);
		}
		else //file
		{
			ctr_shell_print("%s\n", fullPath);
		}
		entries++;
	}
	FSDIR_Close(dirHandle);
	if(entries==0)
	{
		ctr_shell_print("Directory is empty...\n");
	}
}

void cmd_title_exec(u64 titleId)
{
    if(titleId<=0)
    {
        ctr_shell_print("Oups, invalid titleid: 0x%016llx\n", titleId);
        return;
    }

    if(ctr_title_exist(1, titleId))
    {
        if(!ctr_title_exec(1, titleId))
        {
            ctr_shell_print("Oups, couldn't launch titleid: 0x%016llx\n", titleId);
        }
    }
    else if(ctr_title_exist(0, titleId))
    {
        if(!ctr_title_exec(0, titleId))
        {
            ctr_shell_print("Oups, couldn't launch titleid: 0x%016llx\n", titleId);
        }
    }
    else
    {
        ctr_shell_print("Oups, titleid not found: 0x%016llx\n", titleId);
    }
}

CIA cmd_title_install(char *ciaPath)
{
    ctr_abs_path(ciaPath);

    CIA cia;
	cia.titleId = 0;

	if(!ctr_file_exist(ciaPath))
	{
		ctr_shell_print("Oups, file doesn't exist: %s\n", ciaPath);
		return cia;
	}

    cia = ctr_cia_info(ciaPath, 1);
	if(cia.titleId<=0)
	{
		ctr_shell_print("Oups, couldn't get cia information\n");
		return cia;
	}

	ctr_shell_print("Installing cia: %s (0x%016llx)\n", cia.shortDescription, cia.titleId);
	if(ctr_install_cia(ciaPath) != 0)
	{
		ctr_shell_print("Oups, something went wrong...\n");
		cia.titleId = 0;
		return cia;
	}

    ctr_shell_print("Installation success: %s (0x%016llx)\n", cia.shortDescription, cia.titleId);
	return cia;
}

void cmd_put_file_exec(int size)
{
	cmd_put_file(size, "/tmp.cia");
    CIA cia = cmd_title_install("/tmp.cia");
    ctr_del_file("/tmp.cia");
    if(cia.titleId<=0) return;
	if(!ctr_title_exist(1, cia.titleId))
	{
		ctr_shell_print("Oups, something went wrong...\n");
		return;
	}
	ctr_shell_print("Launching %s...\n", cia.shortDescription);
	cmd_title_exec(cia.titleId);
}

void cmd_title_list(int mediaType)
{
	u32 titleCount = ctr_title_count(mediaType);
	if(titleCount <= 0)
	{
		ctr_shell_print("Found 0 titles\n");
		return;
	}
	ctr_shell_print("Found %i titles :\n", titleCount );

    u64 titleIds[titleCount];
	memset(&titleIds, 0, sizeof(titleIds));
	ctr_titleid_list(mediaType, titleCount, titleIds);
	if(titleIds == NULL)
	{
		return;
	}

	TitleList titleList[titleCount];
	memset(&titleList, 0, sizeof(titleList));
	ctr_title_list(mediaType, titleCount, titleIds, titleList);

	u32 i;
	for(i = 0; i < titleCount; i++)
	{
		ctr_shell_print(
            "id:0x%016llx - uid:0x%u - v:%u - size:%llu\n",
            titleList[i].titleID,
            (u32)titleList[i].titleID,
            titleList[i].titleVersion,
            titleList[i].size);
	}
}

void cmd_title_info(char *path)
{
    ctr_abs_path(path);

    CIA cia = ctr_cia_info(path, 1);
	if(cia.titleId<=0)
	{
		ctr_shell_print("Oups, couldn't get cia information\n");
		return;
	}
    ctr_shell_print("\n==== TITLE INFO ====\n");
    ctr_shell_print("titleId: 0x%016llx (uid:0x%x)\n", cia.titleId, cia.uniqueId);
    ctr_shell_print("shortDescription: %s\n", cia.shortDescription);
    ctr_shell_print("longDescription: %s\n", cia.longDescription);
    ctr_shell_print("productCode: %s\n", cia.productCode);
    ctr_shell_print("publisher: %s\n", cia.publisher);
    ctr_shell_print("size: %llu\n", cia.size);
    ctr_shell_print("==== TITLE INFO ====\n");
}

void cmd_title_delete(u64 titleId)
{
    if(ctr_title_exist(1, titleId))
    {
        if(!ctr_title_delete(1, titleId))
        {
            ctr_shell_print("Oups, couldn't delete title: 0x%016llx\n", titleId);
            return;
        }
    }
    else if(ctr_title_exist(0, titleId))
    {
        if(!ctr_title_delete(0, titleId))
        {
           ctr_shell_print("Oups, couldn't delete title: 0x%016llx\n", titleId);
           return;
        }
    }
    else
    {
        ctr_shell_print("Oups, titleid not found: 0x%016llx\n", titleId);
        return;
    }
    ctr_shell_print("title deleted: 0x%016llx\n", titleId);
}

void cmd_memr(u32 addr, int len)
{
	u32* p = (u32*)addr;
	int i=0;
	for(i=0; i<len; i++)
	{
		ctr_shell_print("0x%08x: %08x\n", p, *p);
		p++;
	}
	ctr_shell_print("\n");
}

void cmd_memw(u32 addr, u32 data)
{
	u32* p = (u32*)addr;
	*p = data;
}

void cmd_cd(char *path)
{
	if(strncmp(path, "..", 2) == 0)
	{
		if(strlen(ctr_now_path)<=1)
			return;
        if(ctr_endWith(ctr_now_path, '/'))
            ctr_now_path[strlen(ctr_now_path)-1] = '\0';
		char *slash = strrchr(ctr_now_path, '/');
		if(slash == NULL)
			return;
		int len = (int)(slash-ctr_now_path);
		ctr_now_path[len] = '\0';
		ctr_shell_print("Working directory: %s\n", ctr_now_path);
		return;
	}

	ctr_abs_path(path);
	if(!ctr_dir_exist(path))
	{
		ctr_shell_print("%s doesn't exist or isn't a directory\n", path);
		return;
	}
	strncpy(ctr_now_path, path, 512);
    ctr_shell_print("Working directory: %s\n", ctr_now_path);
}

void cmd_rm(char *file)
{
    ctr_abs_path(file);

	if(!ctr_file_exist(file))
	{
		ctr_shell_print("%s doesn't exist or is not a file\n", file);
        return;
	}

	if(ctr_del_file(file))
		ctr_shell_print("%s deleted\n", file);
	else
		ctr_shell_print("%s could not be deleted\n", file);
}

void cmd_mv(char *src, char *dst)
{
    ctr_abs_path(src);
    ctr_abs_path(dst);

    if(ctr_file_exist(src)) {
        if(ctr_rename_file(src, dst)) {
            ctr_shell_print("Moved %s to %s\n", src, dst);
        } else {
            ctr_shell_print("Couldn't move %s to %s\n", src, dst);
        }
    }
}

void cmd_rmdir(char *path)
{
    ctr_abs_path(path);

	if(!ctr_dir_exist(path))
	{
		ctr_shell_print("%s doesn't exist or is not a directory\n", path);
        return;
	}

	if(ctr_del_dir(path))
		ctr_shell_print("%s deleted\n", path);
	else
		ctr_shell_print("%s could not be deleted\n", path);
}

void cmd_mkdir(char *path)
{
    ctr_abs_path(path);

	if(ctr_dir_exist(path))
	{
		ctr_shell_print("%s already exist\n", path);
        return;
	}

	if(ctr_create_dir_rec(path))
		ctr_shell_print("%s created\n", path);
	else
		ctr_shell_print("%s could not be created\n", path);
}

void cmd_welcome()
{
	ctr_shell_print("\n\n\t_________   __          _________.__           .__  .__   \n");
	ctr_shell_print("\t\\_   ___ \\_/  |________/   _____/|  |__   ____ |  | |  |  \n");
	ctr_shell_print("\t/    \\  \\/\\   __\\_  __ \\_____  \\ |  |  \\_/ __ \\|  | |  |  \n");
	ctr_shell_print("\t\\     \\____|  |  |  | \\/        \\|   Y  \\  ___/|  |_|  |__\n");
	ctr_shell_print("\t \\______  /|__|  |__| /_______  /|___|  /\\___  >____/____/\n");
	ctr_shell_print("\t        \\/                    \\/      \\/     \\/ b.%s          \n\n", VERSION);
}

void handle_cmd(cmd_s *cmd)
{
	switch(cmd->type)
	{
		case CMD_NONE:
		break;

		case CMD_LS:
			cmd_ls(cmd->args[0]);
		break;

		case CMD_CD:
			cmd_cd(cmd->args[0]);
		break;

        case CMD_MV:
            cmd_mv(cmd->args[0], cmd->args[1]);
        break;

		case CMD_RM:
			cmd_rm(cmd->args[0]);
		break;

		case CMD_RMDIR:
			cmd_rmdir(cmd->args[0]);
		break;

        case CMD_MKDIR:
			cmd_mkdir(cmd->args[0]);
		break;

		case CMD_PWD:
			ctr_shell_print("%s\n", ctr_now_path);
		break;

		case CMD_TITLE_LIST:
			cmd_title_list(atoi(cmd->args[0]));
		break;

		case CMD_TITLE_INFO:
			cmd_title_info(cmd->args[0]);
		break;

		case CMD_TITLE_DEL:
			cmd_title_delete(strtoull(cmd->args[0], NULL, 16));
		break;

		case CMD_TITLE_INSTALL:
			cmd_title_install(cmd->args[0]);
		break;

		case CMD_TITLE_EXEC:
            cmd_title_exec(strtoull(cmd->args[0], NULL, 16));
		break;

        case CMD_TITLE_SEND:
			cmd_put_file_exec(atoi(cmd->args[0]));
		break;

		case CMD_PUT:
			cmd_put_file(atoi(cmd->args[0]), cmd->args[1]);
		break;

		case CMD_MEMR:
			cmd_memr(strtoul(cmd->args[0], NULL, 16), atoi(cmd->args[1]));
		break;

		case CMD_MEMW:
			cmd_memw(strtoul(cmd->args[0], NULL, 16),
				strtoul(cmd->args[1], NULL, 16));
		break;

		case CMD_MENU:
			aptReturnToMenu();
		break;

		case CMD_RESET:
			if(!ctr_is_cia)
				ctr_bootApp(ctr_app_path, NULL);
            else
                aptReturnToMenu();
		break;

		case CMD_HELP:
		break;

		default:
		break;
	}
}

bool ctr_init_socket()
{
	ctr_soc_buffer = memalign(0x1000, 0x100000);
	if(ctr_soc_buffer == NULL)
	{
		ctr_debug("Err: shell_init:memalign\n");
		ctr_shell_exit();
		return false;
	}

	Result ret = SOC_Initialize(ctr_soc_buffer, 0x100000);
	if(ret != 0)
	{
		ctr_debug("Err: shell_init:SOC_Initialize\n");
		ctr_shell_exit();
		return false;
	}

	ctr_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(ctr_listen_socket == -1)
	{
		ctr_debug("Err: shell_init:socket\n");
		ctr_shell_exit();
		return false;
	}

	int flags = fcntl(ctr_listen_socket, F_GETFL);
	fcntl(ctr_listen_socket, F_SETFL, flags | O_NONBLOCK);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ctr_shell_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(ctr_listen_socket, (struct sockaddr *)&addr, sizeof(addr));
	if(ret != 0)
	{
		ctr_debug("Err: shell_init:socket\n");
		ctr_shell_exit();
		return false;
	}

	ret = listen(ctr_listen_socket, 1);
	if(ret == -1)
	{
		ctr_debug("Err: shell_init:socket\n");
		ctr_shell_exit();
		return false;
	}

	u32 ip = gethostid();
	ctr_debug("ip: %d.%d.%d.%d\n", (int)ip & 0xFF, (int)(ip>>8)&0xFF, (int)(ip>>16)&0xFF, (int)(ip>>24)&0xFF);
	return true;
}

char ctr_buf[512];
void ctr_cmd_thread(void *arg)
{
#ifdef THREADED
	while(!ctr_threadExit)
#endif
	{
		// lower thread priority
		//svcSleepThread(10000000);

		int s = accept(ctr_listen_socket, NULL, NULL);
		if(s < 0)
		{
			//ctr_debug("Err: shell_init:accept: %i\n", errno);
		}
		else
		{
			ctr_debug("shell_init:accepted\n");

			ctr_sockfd = s;
			cmd_welcome();
#ifdef THREADED
			while(!ctr_threadExit)
#else
            while(!ctr_threadExit && aptMainLoop())
#endif
			{
				// lower thread priority
				//svcSleepThread(10000000);

				int len = recv(ctr_sockfd, ctr_buf, sizeof(ctr_buf), 0);

				if(len>0) {
					//ctr_debug("recv:%s (%i)\n", buffer, len);
				}
				if(len==1 || len==2)
				{
					ctr_debug("=== cmd_start ===\n");

					int type = atoi(ctr_buf);
					if(type == 0) continue;
					int args_count = 0;
					memset(cmd_buf, 0, sizeof(cmd_s));
					cmd_buf->type = type;
					ctr_debug("cmd_buf->type=%i\n", cmd_buf->type);
					memset(ctr_buf, 0, 512);
					while(!ctr_threadExit && aptMainLoop() && args_count!=4)
					{
						len = recv(ctr_sockfd, ctr_buf, sizeof(ctr_buf), 0);
						if(len>0)
						{
							if(strncmp(ctr_buf, "cmdend", 6) == 0) break;
							strcpy(cmd_buf->args[args_count], ctr_buf);
							ctr_debug("cmd_buf->args[%i]=%s\n", args_count, cmd_buf->args[args_count]);
							memset(ctr_buf, 0, 512);
							args_count++;
						}
					}
#ifdef THREADED
					if(cmd_buf->type == CMD_EXIT)
					{
						closesocket(ctr_sockfd);
						ctr_sockfd = -1;
						break;
					}
#endif
					handle_cmd(cmd_buf);
					ctr_shell_print("\n");
					memset(ctr_buf, 0, 512);
					ctr_debug("=== cmd_end ===\n");
				}
			}
		}
	}
#ifdef THREADED
	svcExitThread();
#endif
}

void ctr_shell_update()
{
    ctr_cmd_thread(NULL);
}

int ctr_shell_init(char *app_path, int port)
{
	ctr_debug("ctr_shell_init: start\n");

	if(ctr_running)
	{
		ctr_debug("Err: shell_init:ctr_running=1\n");
		return -1;
	}

	ctr_shell_port = port;
	memset(ctr_app_path, 0, 512);
    memset(ctr_now_path, 0, 512);
	strcpy(ctr_now_path, "/");

	if(app_path != NULL)
	{
		strncpy(ctr_app_path, app_path, 512);
		//ctr_chopN(ctr_app_path, 5);
		ctr_debug("shell_init: %s\n", ctr_app_path);
	}
	else
	{
		ctr_is_cia = true;
		ctr_debug("shell_init: launched as CIA\n");
	}

	srvInit();
	aptInit();
    fsInit();
	sdmcInit();
	amInit();
	ctr_smdcArchiveInit();

	ctr_debug("Waiting for wifi");
	while(osGetWifiStrength()<=0) {
		ctr_debug(".");
		ctr_sleep(1000);
		if(ctr_threadExit)
			return -1;
	}
	ctr_sleep(1000);
	ctr_debug("\n");
	//svcSleepThread(1000000000ULL);

    cmd_buf = malloc(sizeof(cmd_s));

	if(!ctr_init_socket())
	{
        ctr_debug("Err: ctr_init_socket\n");
        ctr_shell_exit();
        return 0;
	}

#ifdef THREADED
    ctr_threadStack = memalign(32, CTR_STACK);
    memset(ctr_threadStack, 0, CTR_STACK);
	Result ret = svcCreateThread(&ctr_threadHandle, ctr_cmd_thread, 0, (u32*)&ctr_threadStack[CTR_STACK/4], 0x30, 0);
	if(ret != 0)
	{
		ctr_debug_error(ret, "shell_init:svcCreateThread\n");
		ctr_shell_exit();
	}
	ctr_recovery();
#endif

    ctr_running = 1;
	ctr_debug("ctr_shell_init: end\n");
	return 0;
}

void ctr_shell_exit()
{
#ifdef THREADED
	// threads
	ctr_threadExit = true;
	svcSleepThread(10000000ULL);
	svcCloseHandle(ctr_threadHandle);
	if(ctr_threadStack)
        free(ctr_threadStack);
#endif

	SOC_Shutdown();
	if(ctr_soc_buffer)
		free(ctr_soc_buffer);
	ctr_soc_buffer = NULL;
    ctr_smdcArchiveExit();
    free(cmd_buf);
	amExit();
	sdmcExit();
	fsExit();
	aptExit();
	srvExit();

	ctr_running = 0;
}
