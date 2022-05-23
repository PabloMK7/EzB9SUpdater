#include "main.h"
#include <errno.h>
#include <sys/stat.h>
#include <malloc.h>

extern char     *g_primary_error;

bool    fileExists(const char *path)
{
    int exists;

    if (!path) goto error;
    exists = access(path, F_OK);
    if (exists == 0)
        return (true);
error:
    return (false);
}

int     copy_file(char *old_filename, char  *new_filename)
{
    FILE        *old_file = NULL;
    FILE        *new_file = NULL;
	u8*          buffer = NULL;

    old_file = fopen(old_filename, "rb");
    new_file = fopen_mkdir(new_filename, "wb");
	u32 buffsize = 0x60000;
	buffer = memalign(0x1000, buffsize);
    if (!new_file || !old_file || !buffer)
    {
        if (old_file) fclose(old_file);
		if (new_file) fclose(new_file);
		if (buffer) free(buffer);
		return 1;
    }
	fseek(old_file, 0, SEEK_END);
	u32 totFile = ftell(old_file);
	fseek(old_file, 0, SEEK_SET);
	u32 currFile = 0;
    while (currFile < totFile) 
    {
		if (totFile - currFile < buffsize) buffsize = totFile - currFile;
		fread(buffer, 1, buffsize, old_file);
		fwrite(buffer, 1, buffsize, new_file);
		currFile += buffsize;
		fseek(old_file, currFile, SEEK_SET);
		fseek(new_file, currFile, SEEK_SET);
    }
    fclose(new_file);
    fclose(old_file);
	free(buffer);
    return  (0);
}

int     createDir(const char *path)
{
    u32     len = strlen(path);
    char    _path[0x100];
    char    *p;

    errno = 0;
    if (len > 0x100) goto error;
    strcpy(_path, path);
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
    if (mkdir(_path, 777) != 0)
        if (errno != EEXIST) goto error;
    return (0);
error:
    return (-1);
}


Result deleteDirectory(char* sdDir) {
	FS_Archive sd;
	FS_Path sd_path = { PATH_EMPTY, 0, NULL };
	Result ret = 0;
	ret = FSUSER_OpenArchive(&sd, ARCHIVE_SDMC, sd_path);
	if (ret < 0) return ret;
	ret = FSUSER_DeleteDirectoryRecursively(sd, fsMakePath(PATH_ASCII, sdDir));
	FSUSER_CloseArchive(sd);
	return ret;
}

Result existsDirectory(char *sdDir) {
	FS_Archive sd;
	FS_Path sd_path = { PATH_EMPTY, 0, NULL };
	Result ret = 0;
	Handle dir = 0;
	ret = FSUSER_OpenArchive(&sd, ARCHIVE_SDMC, sd_path);
	if (ret < 0) return false;
	ret = FSUSER_OpenDirectory(&dir, sd, fsMakePath(PATH_ASCII, sdDir));
	if (ret >= 0) {
		FSDIR_Close(dir);
		FSUSER_CloseArchive(sd);
		return true;
	}
	FSUSER_CloseArchive(sd);
	return false;
}
 
Result renameDir(char* src, char* dst) {
	FS_Archive sd;
	FS_Path sd_path = { PATH_EMPTY, 0, NULL };
	Result ret = 0;
	ret = FSUSER_OpenArchive(&sd, ARCHIVE_SDMC, sd_path);
	if (ret < 0) return ret;
	ret = FSUSER_RenameDirectory(sd, fsMakePath(PATH_ASCII, src), sd, fsMakePath(PATH_ASCII, dst));
	FSUSER_CloseArchive(sd);
	return ret;
}

Result getFreeSpace(u64* out) {
	FS_ArchiveResource resource = { 0 };
	int ret = FSUSER_GetArchiveResource(&resource, SYSTEM_MEDIATYPE_SD);
	if (ret < 0) return ret;
	*out = (u64)resource.freeClusters * (u64)resource.clusterSize;
	return ret;
}