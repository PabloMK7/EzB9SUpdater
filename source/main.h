#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include "3ds.h"
#include "3ds/types.h"
#include "graphics.h"
#include "drawableObject.h"
#include "csvc.h"
#include "curl/curl.h"


#define check_prim(result, err) if ((result) != 0) {g_primary_error = err; \
    goto error; }
#define check_sec(result, err) if ((result) != 0) {g_secondary_error = err; \
    goto error; }
#define check_third(result, err) if ((result) != 0) {g_third_error = err; \
    goto error; }

#define CURRENT_PROCESS_HANDLE  (0xffff8001)
#define RESULT_ERROR            (1)
#define TMPBUFFER_SIZE          (0x20000)
#define URL_MAX 1024

#define READREMOTEMEMORY_TIMEOUT    (char *)s_error[0]
#define OPENPROCESS_FAILURE         (char *)s_error[1]
#define PROTECTMEMORY_FAILURE       (char *)s_error[2]
#define REMOTECOPY_FAILURE          (char *)s_error[3]
#define CFGU_INIT_FAILURE           (char *)s_error[4]
#define CFGU_SECURE_FAILURE         (char *)s_error[5]
#define WRONGREGION                 (char *)s_error[6]
#define SMPATCH_FAILURE             (char *)s_error[7]
#define FSPATCH_FAILURE             (char *)s_error[8]
#define FILEOPEN_FAILURE            (char *)s_error[9]
#define NULLSIZE                    (char *)s_error[10]
#define LINEARMEMALIGN_FAILURE      (char *)s_error[11]
#define ACCESSPATCH_FAILURE         (char *)s_error[12]
#define UNKNOWN_FIRM                (char *)s_error[13]
#define UNKNOWN_HOMEMENU            (char *)s_error[14]
#define USER_ABORT                  (char *)s_error[15]
#define FILE_COPY_ERROR             (char *)s_error[16]
#define LOAD_FAILED                 (char *)s_error[17]
#define NTR_ALREADY_LAUNCHED        (char *)s_error[18]
#define CUSTOM_PM_PATCH_FAIL        (char *)s_error[19]

static const char* const s_error[] =
{
	"READREMOTEMEMORY_TIMEOUT",
	"OPENPROCESS_FAILURE",
	"PROTECTMEMORY_FAILURE",
	"REMOTECOPY_FAILURE",
	"CFGU_INIT_FAILURE",
	"CFGU_SECURE_FAILURE",
	"WRONGREGION",
	"SMPATCH_FAILURE",
	"FSPATCH_FAILURE",
	"FILEOPEN_FAILURE",
	"NULL SIZE",
	"LINEARMEMALIGN_FAILURE",
	"ACCESSPATCH_FAILURE",
	"UNKNOWN_FIRM",
	"UNKNOWN_HOMEMENU",
	"USER_ABORT",
	"FILE_COPY_ERROR",
	"LOAD_AND_EXECUTE",
	"NTR is already running",
	"CUSTOM_PM_PATCH_FAIL"
};

#define CIA_ALREADY_EXISTS 0xC8E083FC

typedef struct  updateData_s
{
    char            url[URL_MAX];
    u32             responseCode;
}               updateData_t;

typedef void(*funcType)();
typedef struct  s_BLOCK
{
    u32         buffer[4];
}               t_BLOCK;

typedef enum    version_e
{
    V32 = 0,
    V33 = 1,
    V36 = 2,
}               version_t;
/*
* main.c
*/
extern bool isN3DS;
/*
** misc.s
*/
void    kFlushDataCache(void *, u32);
void    flushDataCache(void);
void    FlushAllCache(void);
void    InvalidateEntireInstructionCache(void);
void    InvalidateEntireDataCache(void);
s32     backdoorHandler();

/*
** files.c
*/
int     copy_file(char *old_filename, char  *new_filename);
bool    fileExists(const char *path);
int     createDir(const char *path);
Result deleteDirectory(char* sdDir);
Result existsDirectory(char *sdDir);
Result renameDir(char* src, char* dst);
Result getFreeSpace(u64* out);

/*
** common_functions.c
*/
bool    abort_and_exit(void);
u32     getCurrentProcessHandle(void);
void    flushDataCache(void);
void    doFlushCache(void);
void    doStallCpu(void);
void    doWait(void);
void    strJoin(char *dst, const char *s1, const char *s2);
void    strInsert(char *dst, char *src, int index);
void    strncpyFromTail(char *dst, char *src, int nb);
bool    inputPathKeyboard(char *dst, char *hintText, char *initialText, int bufSize);
void    waitAllKeysReleased(void);
void    wait(int seconds);
void    debug(char *str, int seconds);
void customBreak(u32 r0, u32 r1, u32 r2, u32 r3);

/*
** updater.c
*/
extern char	CURL_lastErrorCode[CURL_ERROR_SIZE];
int		downloadFile(const char* URL, char* filepath);
int     downloadString(const char* URL, char** out);
char*	getProgText(float prog, int index);
FILE* fopen_mkdir(const char* name, const char* mode);
u64 ezB9SPerform();
void ezB9SCleanup();

/*
* stubsvc.c
*/
void StubDebugSvc();
void UnStubDebugSvc();
extern bool beingDebugged;

/*
 * aptextension.c
 */
u32 APT_Reboot(u64 titleID, FS_MediaType mediatype, u32 firmtidlow);


#endif