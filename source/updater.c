#include "main.h"
#include "errno.h"
#include "sys/stat.h"
#include "graphics.h"
#include "drawableObject.h"
#include "clock.h"
#include <malloc.h>
#include "Unicode.h"
#include "parson.h"
#include "Archives.h"

typedef struct downFileInfo_s
{
	char* fileName;
	u32	  updateIndex;
	char  mode;
} downFileInfo_t;

static char *str_result_buf = NULL;
static size_t str_result_sz = 0;
static size_t str_result_written = 0;

char CURL_lastErrorCode[CURL_ERROR_SIZE];
static u64 showTimer = -1;
static u32 alreadyGotFile = 0;
static int fileDownCnt = 0;
static int totFileDownCnt = 0;
char updatingVer[30] = { 0 };
char* updatingFile = NULL;
FILE *downfile = NULL;
static int retryingFromError = 0;
char progTextBuf[2][20];

char* versionList[100] = { NULL };
extern char g_modversion[15];
char* changelogList[100][100] = { NULL };
char* baseDataURL = NULL;
char* baseFilelistURL = NULL;

bool g_ciaInstallFailed = false;
bool g_allowUserCancel = false;
bool g_userHasCancelled = false;
bool g_hasInstalledCiaFile = false;

s64 g_sdFreeSpace = 0;
bool g_fileDontFit = false;

bool isWifiAvailable() {
	return osGetWifiStrength() > 0;
}

void    DisableSleep(void)
{
	u8  reg;

	if (R_FAILED(mcuHwcInit()))
		return;

	if (R_FAILED(MCUHWC_ReadRegister(0x18, &reg, 1)))
		return;

	reg |= 0x6C; ///< Disable home btn & shell state events
	MCUHWC_WriteRegister(0x18, &reg, 1);
	mcuHwcExit();
}

void    EnableSleep(void)
{
	u8  reg;

	if (R_FAILED(mcuHwcInit()))
		return;

	if (R_FAILED(MCUHWC_ReadRegister(0x18, &reg, 1)))
		return;

	reg &= ~0x6C; ///< Enable home btn & shell state events
	MCUHWC_WriteRegister(0x18, &reg, 1);
	mcuHwcExit();
}

char* getProgText(float prog, int index) {
	if (prog < (1024 * 1024)) {
		sprintf(progTextBuf[index], "%.2f KB", prog / 1024);
		return progTextBuf[index];
	}
	else {
		sprintf(progTextBuf[index], "%.2f MB", prog / (1024 * 1024));
		return progTextBuf[index];
	}
}

void updateTop(curl_off_t dlnow, curl_off_t dltot, float speed) {
	clearTop();
	newAppTop(DEFAULT_COLOR, CENTER | BOLD | MEDIUM, updatingVer);
	newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "Downloading Files");
	newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%d / %d", fileDownCnt, totFileDownCnt);
	if (retryingFromError) {
		newAppTop(COLOR_YELLOW, CENTER | MEDIUM, "\nRecovered from error. Retries: %d/30", retryingFromError);
		newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%s / %s", getProgText(dlnow, 0), getProgText(dltot, 1));
	}
	else {
		newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "\n%s / %s", getProgText(dlnow, 0), getProgText(dltot, 1));
	}
	if (g_allowUserCancel) {
		newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%.2f KB/s", speed);
	}
	else
		newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%.2f KB/s\n", speed);
	if (updatingFile) newAppTopMultiline(DEFAULT_COLOR, CENTER | SMALL, updatingFile);
}

static size_t handle_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
	(void)userdata;
	const size_t bsz = size * nmemb;
	
	if (!str_result_buf) {
		str_result_sz = 1 << 9;
		str_result_buf = memalign(0x1000, 1 << 9);
	}
	bool needrealloc = false;
	while (bsz + str_result_written >= str_result_sz) {
		str_result_sz <<= 1;
		needrealloc = true;
	}
	if (needrealloc) {
		str_result_buf = realloc(str_result_buf, str_result_sz);
		if (!str_result_buf) {
			return 0;
		}
	}
	if (!str_result_buf) return 0;
	memcpy(str_result_buf + str_result_written, ptr, bsz);
	str_result_written += bsz;
	return bsz;
}

int file_progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	if (!(dltotal && dlnow) || dltotal < 1024) return 0;
	static float oldprogress = 0;
	static u64 timer = 0;
	static u64 timer2 = 0;
	dltotal += alreadyGotFile;
	dlnow += alreadyGotFile;
	if (showTimer == -1) { //Not initialized
		oldprogress = 0;
		timer2 = 0;
		timer = Timer_Restart();
		showTimer = Timer_Restart();
	}
	if (getTimeInMsec(Timer_Restart() - showTimer) > 100) {
		timer2 = showTimer =  Timer_Restart();
		u64 progmsec = getTimeInMsec(timer2 - timer);
		timer = Timer_Restart();
		if (!progmsec) progmsec = 1;
		curl_off_t progbytes = dlnow - oldprogress;
		updateTop(dlnow, dltotal, ((float)progbytes) / ((float)progmsec));
		updateUI();
		oldprogress = dlnow;
	}
	if (dltotal > (g_sdFreeSpace - 10000000)) {
		g_fileDontFit = true;
		return 1;
	}
	if (aptMainLoop()) return 0;
	else return 1;
};

char* getFileFromPath(char* file) {
	char* ret = file;
	while (*file) {
		if (*file++ == '/') ret = file;
	}
	return ret;
}

char *strdup(const char *s) {
	char *d = malloc(strlen(s) + 1);   // Space for length plus nul
	if (d == NULL) return NULL;          // No memory
	strcpy(d, s);                        // Copy the characters
	return d;                            // Return the new string
}

FILE* fopen_mkdir(const char* name, const char* mode)
{
	char*	_path = strdup(name);
	char    *p;
	FILE*	retfile = NULL;

	errno = 0;
	for (p = _path + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = '\0';
			if (mkdir(_path, 777) != 0)
				if (errno != EEXIST) goto error;
			*p = '/';
		}
	}
	retfile = fopen(name, mode);
error:
	free(_path);
	return retfile;
}

void    memcpy32(void *dest, const void *src, size_t count)
{
	u32         lastbytes = count & 3;
	u32         *dest_ = (u32 *)dest;
	const u32   *src_ = (u32*)src;

	count >>= 2;

	while (count--)
		*dest_++ = *src_++;

	u8  *dest8_ = (u8 *)dest_;
	u8  *src8_ = (u8 *)src_;

	while (lastbytes--)
		*dest8_++ = *src8_++;
}

static size_t file_buffer_pos = 0;
static size_t file_toCommit_size = 0;
static char* g_buffers[2] = { NULL };
static u8 g_index = 0;
static Thread fsCommitThread;
static LightEvent readyToCommit;
static LightEvent waitCommit;
static bool killThread = false;
static bool writeError = false;
#define FILE_ALLOC_SIZE 0x20000

bool filecommit() {
	if (!downfile) return false;
	if (!file_toCommit_size) return true;
	fseek(downfile, 0, SEEK_END);
	u32 byteswritten = fwrite(g_buffers[!g_index], 1, file_toCommit_size, downfile);
	if (byteswritten != file_toCommit_size) return false;
	file_toCommit_size = 0;
	return true;
}

static void commitToFileThreadFunc(void* args) {
	LightEvent_Signal(&waitCommit);
	while (true) {
		LightEvent_Wait(&readyToCommit);
		LightEvent_Clear(&readyToCommit);
		if (killThread) threadExit(0);
		writeError = !filecommit();
		LightEvent_Signal(&waitCommit);
	}
}

static size_t file_handle_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
	(void)userdata;
	const size_t bsz = size * nmemb;
	size_t tofill = 0;
	if (writeError) return 0;
	if (!g_buffers[g_index]) {

		LightEvent_Init(&waitCommit, RESET_STICKY);
		LightEvent_Init(&readyToCommit, RESET_STICKY);

		s32 prio = 0;
		svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
		fsCommitThread = threadCreate(commitToFileThreadFunc, NULL, 0x8000, prio - 1, -2, true);

		g_buffers[0] = memalign(0x1000, FILE_ALLOC_SIZE);
		g_buffers[1] = memalign(0x1000, FILE_ALLOC_SIZE);

		if (!fsCommitThread || !g_buffers[0] || !g_buffers[1]) return 0;
	}
	if (file_buffer_pos + bsz >= FILE_ALLOC_SIZE) {
		tofill = FILE_ALLOC_SIZE - file_buffer_pos;
		memcpy(g_buffers[g_index] + file_buffer_pos, ptr, tofill);
		
		LightEvent_Wait(&waitCommit);
		LightEvent_Clear(&waitCommit);
		file_toCommit_size = file_buffer_pos + tofill;
		file_buffer_pos = 0;
		svcFlushProcessDataCache(CURRENT_PROCESS_HANDLE, (u32)g_buffers[g_index], file_toCommit_size);
		g_index = !g_index;
		LightEvent_Signal(&readyToCommit);
	}
	memcpy(g_buffers[g_index] + file_buffer_pos, ptr + tofill, bsz - tofill);
	file_buffer_pos += bsz - tofill;
	return bsz;
}

int downloadFile(const char* URL, char* filepath) {

	int retcode = 0;
	aptSetHomeAllowed(false);
	aptSetSleepAllowed(false);
	
	void *socubuf = memalign(0x1000, 0x100000);
	if (!socubuf) {
		sprintf(CURL_lastErrorCode, "Failed to allocate memory.");
		retcode = 1;
		goto exit;
	}
	
	downfile = fopen_mkdir(filepath, "wb");
	if (!downfile) {
		sprintf(CURL_lastErrorCode, "Failed to create file.");
		retcode = 4;
		goto exit;
	} 
	CURL_lastErrorCode[0] = 0;
	retryingFromError = 0;
	g_userHasCancelled = false;
	alreadyGotFile = 0;
	g_fileDontFit = false;
	getFreeSpace((u64*)&g_sdFreeSpace);
	updatingFile = getFileFromPath(filepath);
	clearTop();
	newAppTop(DEFAULT_COLOR, CENTER | BOLD | MEDIUM, updatingVer);
	newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "Downloading Files");
	newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%d / %d", fileDownCnt, totFileDownCnt);
	updateUI();

	while (true) {
		int res = socInit(socubuf, 0x100000);
		CURLcode cres;
		if (R_FAILED(res)) {
			sprintf(CURL_lastErrorCode, "socInit returned: 0x%08X", res);
			cres = 0xFF;
		}
		else {
			CURL* hnd = curl_easy_init();
			curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, FILE_ALLOC_SIZE);
			curl_easy_setopt(hnd, CURLOPT_URL, URL);
			curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(hnd, CURLOPT_USERAGENT, "Mozilla/5.0 (Nintendo 3DS; U; ; en) AppleWebKit/536.30 (KHTML, like Gecko) CTGP-7/1.0 CTGP-7/1.0");
			curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(hnd, CURLOPT_ACCEPT_ENCODING, "gzip");
			if (retryingFromError) {
				u32 gotSize = ftell(downfile);
				if (gotSize) {
					alreadyGotFile = gotSize;
					char tmpRange[0x30];
					sprintf(tmpRange, "%ld-", gotSize);
					curl_easy_setopt(hnd, CURLOPT_RANGE, tmpRange);
				}
			}
			curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
			curl_easy_setopt(hnd, CURLOPT_XFERINFOFUNCTION, file_progress_callback);
			curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
			curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, file_handle_data);
			curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, CURL_lastErrorCode);
			curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(hnd, CURLOPT_STDERR, stdout);

			cres = curl_easy_perform(hnd);
			curl_easy_cleanup(hnd);
		}		
		if (cres != CURLE_OK) {
			if (retryingFromError < 30 && aptMainLoop() && !g_userHasCancelled && !g_fileDontFit) {
				svcSleepThread(1000000000);
				clearTop();
				newAppTop(DEFAULT_COLOR, CENTER | BOLD | MEDIUM, updatingVer);
				newAppTop(COLOR_YELLOW, CENTER | MEDIUM, "Download failed with error code:");
				newAppTop(COLOR_YELLOW, CENTER | MEDIUM, "\n0x%08X", cres);
				newAppTopMultiline(COLOR_YELLOW, CENTER | SMALL, CURL_lastErrorCode);
				newAppTop(COLOR_YELLOW, CENTER | MEDIUM, "\nRetrying in 10 seconds...");
				newAppTop(COLOR_YELLOW, CENTER | MEDIUM, "Retry count: (%d / 30)", retryingFromError + 1);
				updateUI();
				svcSleepThread(10000000000);
				retryingFromError++;
			}
			else {
				retryingFromError = 0;
				retcode = cres;
				goto exit;
			}
		}
		else retryingFromError = 0;

		if (fsCommitThread) {
			LightEvent_Wait(&waitCommit);
			LightEvent_Clear(&waitCommit);
		}
		
		if (cres == CURLE_OK) {
			file_toCommit_size = file_buffer_pos;
			svcFlushProcessDataCache(CURRENT_PROCESS_HANDLE, (u32)g_buffers[g_index], file_toCommit_size);
			g_index = !g_index;
			if (!filecommit()) {
				sprintf(CURL_lastErrorCode, "Couldn't commit to file.");
				retcode = 2;
				goto exit;
			}
		}
		fflush(downfile);

	exit:
		if (fsCommitThread) {
			killThread = true;
			LightEvent_Signal(&readyToCommit);
			threadJoin(fsCommitThread, U64_MAX);
			killThread = false;
			fsCommitThread = NULL;
		}

		socExit();

		if (!retryingFromError) {

			if (socubuf) {
				free(socubuf);
			}
			if (downfile) {
				fclose(downfile);
				downfile = NULL;
			}
			updatingFile = NULL;
		}

		if (g_buffers[0]) {
			free(g_buffers[0]);
			g_buffers[0] = NULL;
		}
		if (g_buffers[1]) {
			free(g_buffers[1]);
			g_buffers[1] = NULL;
		}
		file_buffer_pos = 0;
		file_toCommit_size = 0;
		writeError = false;
		g_index = 0;
		showTimer = -1;

		if (!retryingFromError)
			return retcode;
		else svcSleepThread(1000000000);
	}
	aptSetHomeAllowed(true);
	aptSetSleepAllowed(true);
}


int downloadString(const char* URL, char** out) {

	DisableSleep();

	*out = NULL;
	int retcode = 0;

	void *socubuf = memalign(0x1000, 0x100000);
	if (!socubuf) {
		sprintf(CURL_lastErrorCode, "Failed to allocate memory.");
		retcode = 1;
		goto exit;
	}
	int res = socInit(socubuf, 0x100000);
	if (R_FAILED(res)) {
		sprintf(CURL_lastErrorCode, "socInit returned: 0x%08X", res);
		retcode = 2;
		goto exit;
	}

	CURL *hnd = curl_easy_init();
	curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
	curl_easy_setopt(hnd, CURLOPT_URL, URL);
	curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(hnd, CURLOPT_USERAGENT, "Mozilla/5.0 (Nintendo 3DS; U; ; en) AppleWebKit/536.30 (KHTML, like Gecko) CTGP-7/1.0 CTGP-7/1.0");
	curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(hnd, CURLOPT_ACCEPT_ENCODING, "gzip");
	curl_easy_setopt(hnd, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
	curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, handle_data);
	curl_easy_setopt(hnd, CURLOPT_ERRORBUFFER, CURL_lastErrorCode);
	curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(hnd, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(hnd, CURLOPT_STDERR, stdout);

	CURL_lastErrorCode[0] = 0;
	CURLcode cres = curl_easy_perform(hnd);
	curl_easy_cleanup(hnd);

	if (cres != CURLE_OK) {
		retcode = cres;
		goto exit;
	}

	str_result_buf[str_result_written] = '\0';
	*out = str_result_buf;

exit:
	socExit();

	if (socubuf) {
		free(socubuf);
	}

	str_result_buf = NULL;
	str_result_written = 0;
	str_result_sz = 0;

	EnableSleep();

	return retcode;
}

void fileNameTrim(char* filename) {
	int size = strlen(filename);
	for (int i = size - 1; i >= 0; i--) {
		char c = filename[i];
		if (c == '\n' || c == '\r' || c == ' ') filename[i] = '\0';
		else break;
	}
}

int zipCallBack(u32 curr, u32 total) {
	static u64 timer = 0;
	if (Timer_HasTimePassed(10, timer)) {
		timer = Timer_Restart();
		clearTop();
		newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
		newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Extracting files...");
		newAppTop(DEFAULT_COLOR, CENTER | MEDIUM, "%d / %d", curr, total);
		updateUI();
	}
	return 0;
}

static void restorepayloadsdir() {
	remove("/luma/payloads/SafeB9SInstaller.firm");
	rmdir("/luma/payloads");
	renameDir("/luma/payloadsold", "/luma/payloads");
}

u64 ezB9SPerform() {
	clearTop();
	newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
	newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Preparing files...");
	updateUI();

	deleteDirectory("/ezb9stemp");
	
	char* outJsonConfig = NULL;
	u32 ret = downloadString("https://ctgp7.page.link/EzB9SUpdaterConfig", &outJsonConfig);
	if (ret) {
		if (outJsonConfig) free(outJsonConfig);
		return (2ULL << 32) | ret;
	}

	if (createDir("/ezb9stemp")) {
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Failed to create /ezb9stemp");
		return (3ULL << 32) | ret;
	}

	JSON_Value *root_value = json_parse_string(outJsonConfig);
	if (json_value_get_type(root_value) != JSONObject) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Invalid JSON config file.");
		return (4ULL << 32) | ret;
	}
	const char* canRun = json_object_get_string(json_value_get_object(root_value), "canrun");
	if (strcmp(canRun, "1") != 0) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		return (5ULL << 32);
	}
	const char* sb9si = json_object_get_string(json_value_get_object(root_value), "sb9si");
	const char* sb9si_firm = json_object_get_string(json_value_get_object(root_value), "sb9si_firm");
	const char* b9sf = json_object_get_string(json_value_get_object(root_value), "b9sf");
	const char* b9sf_firm = json_object_get_string(json_value_get_object(root_value), "b9sf_firm");
	const char* b9sf_sha = json_object_get_string(json_value_get_object(root_value), "b9sf_sha");
	if (!sb9si || !sb9si_firm || !b9sf || !b9sf_firm || !b9sf_sha)
	{
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Invalid JSON config file.");
		return (6ULL << 32);
	}

	clearTop();
	newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
	newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Downloading files...");
	updateUI();

	fileDownCnt = 1;
	totFileDownCnt = 2;
	strcpy(updatingVer, "EzB9SUpdater");
	ret = downloadFile(sb9si, "/ezb9stemp/sb9si.zip");
	if (ret) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		return (7ULL << 32);
	}

	fileDownCnt = 2;
	totFileDownCnt = 2;
	ret = downloadFile(b9sf, "/ezb9stemp/b9sf.zip");
	if (ret) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		return (8ULL << 32);
	}

	clearTop();
	newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
	newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Extracting files...");
	updateUI();

	if (createDir("/ezb9stemp/sb9si") || createDir("/ezb9stemp/b9sf")) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Failed to create zip output folders");
		return (9ULL << 32) | ret;
	}

	Zip* sb9si_zip = ZipOpen("/ezb9stemp/sb9si.zip");
	if (!sb9si_zip) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Failed to open sb9si.zip");
		return (10ULL << 32) | ret;
	}
	chdir("/ezb9stemp/sb9si");
	ZipExtract(sb9si_zip, NULL, zipCallBack);
	ZipClose(sb9si_zip);
	chdir("/");

	Zip* b9sf_zip = ZipOpen("/ezb9stemp/b9sf.zip");
	if (!b9sf_zip) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Failed to open b9sf.zip");
		return (11ULL << 32) | ret;
	}
	chdir("/ezb9stemp/b9sf");
	ZipExtract(b9sf_zip, NULL, zipCallBack);
	ZipClose(b9sf_zip);
	chdir("/");

	char final_sb9si_firm[0x80];
	char final_b9sf_firm[0x80];
	char final_b9sf_sha[0x80];
	sprintf(final_sb9si_firm, "/ezb9stemp/sb9si/%s", sb9si_firm);
	sprintf(final_b9sf_firm, "/ezb9stemp/b9sf/%s", b9sf_firm);
	sprintf(final_b9sf_sha, "/ezb9stemp/b9sf/%s", b9sf_sha);
	if (!fileExists(final_sb9si_firm) || !fileExists(final_b9sf_firm) || !fileExists(final_b9sf_sha)) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Files not found in downloaded zip");
		return (12ULL << 32) | ret;
	}

	clearTop();
	newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
	newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Copying files...");
	updateUI();

	remove("/boot9strap/boot9strap.firm");
	remove("/boot9strap/boot9strap.firm.sha");
	renameDir("/luma/payloads", "/luma/payloadsold");
	ret = copy_file(final_sb9si_firm, "/luma/payloads/SafeB9SInstaller.firm");
	if (!ret) copy_file(final_b9sf_firm, "/boot9strap/boot9strap.firm");
	if (!ret) copy_file(final_b9sf_sha, "/boot9strap/boot9strap.firm.sha");
	if (ret) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		restorepayloadsdir();
		sprintf(CURL_lastErrorCode, "Failed to copy files to final location");
		return (13ULL << 32) | ret;
	}

	FILE* flagFile = fopen("/ezb9stemp/reboot.flag", "w");
	if (!flagFile) {
		json_value_free(root_value);
		if (outJsonConfig) free(outJsonConfig);
		sprintf(CURL_lastErrorCode, "Failed to create reboot flag");
		restorepayloadsdir();
		return (14ULL << 32) | ret;
	}
	fwrite("", 1, 1, flagFile);
	fclose(flagFile);

	json_value_free(root_value);
	if (outJsonConfig) free(outJsonConfig);
	return 0;
}

void ezB9SCleanup() {
	clearTop();
	newAppTop(DEFAULT_COLOR, BOLD | MEDIUM | CENTER, "EzB9SUpdater");
	newAppTop(DEFAULT_COLOR, MEDIUM | CENTER, "Cleaning up files...");
	updateUI();
	deleteDirectory("/ezb9stemp");
	restorepayloadsdir();
	// Keep boot9strap firm files
}