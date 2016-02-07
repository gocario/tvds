#include "fs.h"

#include <3ds/services/fs.h>
#include <3ds/result.h>
#include <3ds/ipc.h>
#include <3ds/srv.h>
#include <3ds/svc.h>

#include <string.h>

// #define FS_DEBUG

#ifdef FS_DEBUG
#include "console.h"
#define r(format, args ...) consoleLog(format, ##args)
#define debug_print(fmt, args ...) consoleLog(fmt, ##args)
#else
#define r(format, args ...)
#define debug_print(fmt, args ...)
#endif

static Handle fsHandle;
static bool sdmcInitialized = false;
static bool saveInitialized = false;
FS_Archive sdmcArchive;
FS_Archive saveArchive;

#ifdef FS_DEBUG_FIX_ARCHIVE
bool FSDEBUG_FixArchive(FS_Archive** archive)
{
	debug_print("FSDEBUG_FixArchive:\n");

	if (!saveInitialized && *archive == &saveArchive)
	{
		debug_print("   Save archive not initialized\n");
		*archive = &sdmcArchive;
	}

	if (!sdmcInitialized && *archive == &sdmcArchive)
	{
		debug_print("   Sdmc archive not initialized\n");
		*archive = NULL;
	}

	return (*archive != NULL);
}
#endif

Result FS_ReadFile(char* path, void* dst, FS_Archive* archive, u64 maxSize, u32* bytesRead)
{
	if (!path || !dst || !archive || !bytesRead) return -1;
	
#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;
	u64 size;
	Handle fileHandle;

	debug_print("FS_ReadFile:\n");

	ret = FSUSER_OpenFile(&fileHandle, *archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	r(" > FSUSER_OpenFile: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = FSFILE_GetSize(fileHandle, &size);
	r(" > FSFILE_GetSize: %lx\n", ret);
	if (R_FAILED(ret) || size > maxSize) ret = -2;

	if (R_SUCCEEDED(ret))
	{
		ret = FSFILE_Read(fileHandle, bytesRead, 0x0, dst, size);
		r(" > FSFILE_Read: %lx\n", ret);
		if (R_FAILED(ret) || *bytesRead < size) ret = -3;
	}

	ret = FSFILE_Close(fileHandle);
	r(" > FSFILE_Close: %lx\n", ret);

	return ret;
}

Result FS_WriteFile(char* path, void* src, u64 size, FS_Archive* archive, u32* bytesWritten)
{
	if (!path || !src || !archive || !bytesWritten) return -1;

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;
	Handle fileHandle;

	debug_print("FS_WriteFile:\n");

	ret = FSUSER_OpenFile(&fileHandle, *archive, fsMakePath(PATH_ASCII, path), FS_OPEN_WRITE | FS_OPEN_CREATE, FS_ATTRIBUTE_NONE);
	r(" > FSUSER_OpenFile: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	if (R_SUCCEEDED(ret))
	{
		ret = FSFILE_Write(fileHandle, bytesWritten, 0L, src, size, FS_WRITE_FLUSH);
		r(" > FSFILE_Write: %lx\n", ret);
		if (R_FAILED(ret) || *bytesWritten != size) ret = -2;
	}

	ret = FSFILE_Close(fileHandle);
	r(" > FSFILE_Close: %lx\n", ret);

	return ret;
}

Result FS_DeleteFile(char* path, FS_Archive* archive)
{
	if (!path || !archive) return -1;
	
#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;

	debug_print("FS_DeleteFile:\n");

	ret = FSUSER_DeleteFile(*archive, fsMakePath(PATH_ASCII, path));
	r(" > FSUSER_DeleteFile: %lx\n", ret);

	return ret;
}

Result FS_CreateDirectory(char* path, FS_Archive* archive)
{
	if (!path || !archive) return -1;
	
#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;

	debug_print("FS_CreateDirectory:\n");

	ret = FSUSER_CreateDirectory(*archive, fsMakePath(PATH_ASCII, path), FS_ATTRIBUTE_DIRECTORY);
	r(" > FSUSER_CreateDirectory: %lx\n", ret);

	return ret;
}

Result FS_CommitArchive(FS_Archive* archive)
{
	if (!archive) return -1;
	
	Result ret;

	debug_print("FS_CommitArchive:\n");

	ret = FSUSER_ControlArchive(*archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
	r(" > FSUSER_ControlArchive: %lx\n", ret);

	return ret;
}

Result FS_Init(void)
{
	Result ret = 0;

	debug_print("FS_Init:\n");

	ret = srvGetServiceHandleDirect(&fsHandle, "fs:USER");
	r(" > srvGetServiceHandleDirect: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = FSUSER_Initialize(fsHandle);
	r(" > FSUSER_Initialize: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	fsUseSession(fsHandle, false);
	debug_print(" > fsUseSession\n");

	if (!sdmcInitialized)
	{
		sdmcArchive = (FS_Archive) { ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, NULL), fsHandle };

		ret = FSUSER_OpenArchive(&sdmcArchive);
		r(" > FSUSER_OpenArchive: %lx\n", ret);

		sdmcInitialized = R_SUCCEEDED(ret);
	}

	if (!saveInitialized)
	{
		saveArchive = (FS_Archive) { ARCHIVE_SAVEDATA, fsMakePath(PATH_EMPTY, NULL), fsHandle };

		ret = FSUSER_OpenArchive(&saveArchive);
		r(" > FSUSER_OpenArchive: %lx\n", ret);

		saveInitialized = R_SUCCEEDED(ret);
	}

	return ret;
}

Result FS_Exit(void)
{
	Result ret = 0;
	
	debug_print("FS_Exit:\n");

	if (sdmcInitialized)
	{
		ret = FSUSER_CloseArchive(&sdmcArchive);
		r(" > FSUSER_CloseArchive: %lx\n", ret);

		sdmcInitialized = false;
	}

	if (saveInitialized)
	{
		ret = FS_CommitArchive(&saveArchive);
		r(" > FS_CommitArchive: %lx\n", ret);

		ret = FSUSER_CloseArchive(&saveArchive);
		r(" > FSUSER_CloseArchive: %lx\n", ret);

		saveInitialized = false;
	}

	fsEndUseSession();
	debug_print(" > fsEndUseSession\n");

	ret = svcCloseHandle(fsHandle);
	r(" > svcCloseHandle: %lx\n", ret);

	return ret;
}
