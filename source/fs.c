#include "fs.h"

#include <3ds/services/fs.h>
#include <3ds/result.h>
#include <3ds/ipc.h>
#include <3ds/srv.h>
#include <3ds/svc.h>

#include <string.h>

// #define DEBUG_FS
#define DEBUG_FIX_ARCHIVE_FS

#ifdef DEBUG_FS
#include <stdio.h>
#define r(format, args ...) printf(format, ##args)
#define debug_print(fmt, args ...) printf(fmt, ##args)
#else
#define r(format, args ...)
#define debug_print(fmt, args ...)
#endif

typedef enum {
	STATE_UNINITIALIZED,
	STATE_UNINITIALIZING,
	STATE_INITIALIZING,
	STATE_INITIALIZED,
} FS_State;

static Handle fsHandle;
static FS_State fsState = STATE_UNINITIALIZED;
static bool sdmcInitialized = false;
static bool saveInitialized = false;
FS_Archive sdmcArchive;
FS_Archive saveArchive;

#ifdef DEBUG_FIX_ARCHIVE_FS
/**
 * @brief Redirect the saveArchive to the sdmcArchive for #Citra use
 * @return Whether the archive fix is working.
 */
static bool FS_FixBasicArchive(FS_Archive** archive)
{
	debug_print("FS_FixBasicArchive:\n");

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


bool FS_IsInitialized(void)
{
	return (fsState == STATE_INITIALIZED);
	// return (sdmcInitialize && saveInitialized);
}


bool FS_IsArchiveInitialized(FS_Archive* archive)
{
	return (archive->id == ARCHIVE_SDMC && sdmcInitialized)
		|| (archive->id == ARCHIVE_SAVEDATA && saveInitialized);
}


Result FS_ReadFile(char* path, void* dst, FS_Archive* archive, u64 maxSize, u32* bytesRead)
{
	if (!path || !dst || !archive) return -1;
	
#ifdef DEBUG_FIX_ARCHIVE_FS
	if (!FS_FixBasicArchive(&archive)) return -1;
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
	if (!path || !src || !archive) return -1;

#ifdef DEBUG_FIX_ARCHIVE_FS
	if (!FS_FixBasicArchive(&archive)) return -1;
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
	
#ifdef DEBUG_FIX_ARCHIVE_FS
	if (!FS_FixBasicArchive(&archive)) return -1;
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
	
#ifdef DEBUG_FIX_ARCHIVE_FS
	if (!FS_FixBasicArchive(&archive)) return -1;
#endif

	Result ret;

	debug_print("FS_CreateDirectory:\n");

	ret = FSUSER_CreateDirectory(*archive, fsMakePath(PATH_ASCII, path), FS_ATTRIBUTE_DIRECTORY);
	r(" > FSUSER_CreateDirectory: %lx\n", ret);

	return ret;
}


Result FS_CommitArchive(FS_Archive* archive)
{
	Result ret = 0;

	debug_print("FS_CommitArchive:\n");

	if (saveInitialized)
	{
		ret = FSUSER_ControlArchive(*archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
		r(" > FSUSER_ControlArchive: %lx\n", ret);
	}

	return ret;
}


Result FS_fsInit(void)
{
	Result ret = 0;
	fsState = STATE_INITIALIZING;

	debug_print("FS_fsInit:\n");

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

	if (sdmcInitialized && saveInitialized)
	{
		fsState = STATE_INITIALIZED;
	}

	return ret;
}


Result FS_fsExit(void)
{
	Result ret = 0;
	fsState = STATE_UNINITIALIZING;
	
	debug_print("FS_fsExit:\n");

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

	if (!sdmcInitialized && !saveInitialized)
	{
		fsState = STATE_UNINITIALIZED;
	}

	return ret;
}
