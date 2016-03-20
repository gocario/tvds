#include "fsdir.h"
#include "fsls.h"
#include "fs.h"
#include "key.h"
#include "utils.h"
#include "console.h"

#include <3ds/os.h>
#include <3ds/result.h>
#include <3ds/util/utf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Result fsStackPush(fsStack* stack, s16 offsetId, s16 selectedId)
{
	if (!stack) return -1;

	fsStackNode* last = (fsStackNode*) malloc(sizeof(fsStackNode));
	last->offsetId = offsetId;
	last->selectedId = selectedId;
	last->prev = stack->last;
	stack->last = last;

	return last->prev != NULL;
}

Result fsStackPop(fsStack* stack, s16* offsetId, s16* selectedId)
{
	if (!stack) return -1;
	if (!stack->last) return 2;

	if (offsetId) *offsetId = stack->last->offsetId;
	if (selectedId) *selectedId = stack->last->selectedId;
	fsStackNode* prev = stack->last->prev;
	free(stack->last);
	stack->last = prev;

	return stack->last != NULL;
}

fsDir saveDir;
fsDir sdmcDir;

static fsDir* currentDir;
static fsDir* dickDir;
static u32 entryPrintCount = 20;

void fsDirInit(void)
{
	memset(&saveDir, 0, sizeof(fsDir));
	memset(&sdmcDir, 0, sizeof(fsDir));

	saveDir.entry.name16[0] = sdmcDir.entry.name16[0] = '/';
	saveDir.entry.name16[1] = sdmcDir.entry.name16[1] = '\0';

	// TODO: Remove when native UTF-16 font.
	strcpy(saveDir.entry.name, "/");
	strcpy(sdmcDir.entry.name, "/");

	saveDir.archive = &saveArchive;
	sdmcDir.archive = &sdmcArchive;

	fsDirRefreshDir(&saveDir, true);
	fsDirRefreshDir(&sdmcDir, true);

	currentDir = &sdmcDir;
	dickDir = &saveDir;
}

void fsDirExit(void)
{
	fsFreeDir(&saveDir.entry);
	fsFreeDir(&sdmcDir.entry);

	while (fsStackPop(&saveDir.entryStack, NULL, NULL) == 0);
	while (fsStackPop(&sdmcDir.entryStack, NULL, NULL) == 0);
}

/**
 * @brief Prints a directory to the current console.
 * @param dir The directory to print.
 * @param data An header string to print.
 */
static void fsDirPrint(fsDir* dir, const char* data)
{
	s32 i = 0;
	u8 row = 3;
	fsEntry* next = dir->entry.firstEntry;

	// Skip the first off-screen entries
	for (; next && i < dir->entryOffsetId; i++)
	{
		next = next->nextEntry;
	}

	consoleClear();

	consoleResetColor();
	printf("\x1B[0;0H%s data:", data);
	consoleForegroundColor(TEAL);
	printf("\x1B[1;0H%.25s", dir->entry.name);
	consoleResetColor();

	for (; next && i < dir->entry.entryCount && i < dir->entryOffsetId + entryPrintCount; i++)
	{
		// If the entry is the current entry
		if (dir == currentDir && dir->entrySelectedId == i)
		{
			consoleBackgroundColor(SILVER);
			if (next->isDirectory) consoleForegroundColor(TEAL);
			else consoleForegroundColor(BLACK);

			// Blank placeholder
			printf("\x1B[%u;0H \a                       ", row);

			// Tricky-hacky
			dir->entrySelected = next;
		}
		// Else if the entry is just a simple entry
		else if (next->isDirectory) consoleForegroundColor(CYAN);
		else consoleForegroundColor(WHITE);

		// Display entry's name
		printf("\x1B[%u;3H%.22s", row++, next->name);
		consoleResetColor();

		// Iterate through linked list
		next = next->nextEntry;
	}
}

void fsDirPrintSave(void)
{
	consoleSelectNew(&saveConsole);
	fsDirPrint(&saveDir, "Save");
	consoleSelectLast();
}

void fsDirPrintSdmc(void)
{
	consoleSelectNew(&sdmcConsole);
	fsDirPrint(&sdmcDir, "Sdmc");
	consoleSelectLast();
}

void fsDirPrintCurrent(void)
{
	if (currentDir == &saveDir)
		fsDirPrintSave();
	else if (currentDir == &sdmcDir)
		fsDirPrintSdmc();
}

void fsDirPrintDick(void)
{
	if (dickDir == &saveDir)
		fsDirPrintSave();
	else if (dickDir == &sdmcDir)
		fsDirPrintSdmc();
}

void fsDirRefreshDir(fsDir* _dir, bool addParentDir)
{
	fsDir* dir = (_dir ? _dir : currentDir);
	fsFreeDir(&dir->entry);
	fsScanDir(&dir->entry, dir->archive, false);
	if (addParentDir) fsAddParentDir(&dir->entry);

	dir->entryOffsetId = 0;
	dir->entrySelectedId = 0;
}

void fsDirSwitch(fsDir* dir)
{
	if ((dir == NULL && currentDir == &sdmcDir) || dir == &saveDir)
	{
		consoleLog("Switched to save div\n");
		currentDir = &saveDir;
		dickDir = &sdmcDir;
	}
	else if ((dir == NULL && currentDir == &saveDir) || dir == &sdmcDir)
	{
		consoleLog("Switched to sdmc div\n");
		currentDir = &sdmcDir;
		dickDir = &saveDir;
	}
}

void fsDirMove(s16 count)
{
	// That bitchy was pretty hard... :/

	currentDir->entrySelectedId += count;

	if (currentDir->entrySelectedId < 0)
	{
		currentDir->entrySelectedId = currentDir->entry.entryCount-1;
		currentDir->entryOffsetId = (currentDir->entry.entryCount > entryPrintCount ? currentDir->entry.entryCount - entryPrintCount : 0);
	}

	if (currentDir->entrySelectedId > currentDir->entry.entryCount-1)
	{
		currentDir->entrySelectedId = 0;
		currentDir->entryOffsetId = 0;
	}

	if (currentDir->entryOffsetId >= currentDir->entrySelectedId)
	{
		currentDir->entryOffsetId = currentDir->entrySelectedId + (currentDir->entrySelectedId > 0 ? -1 : 0);
	}

	else if (currentDir->entrySelectedId > entryPrintCount-2 && count > 0 && currentDir->entryOffsetId + entryPrintCount-2 < currentDir->entrySelectedId)
	{
		currentDir->entryOffsetId = currentDir->entrySelectedId - entryPrintCount + (currentDir->entrySelectedId < currentDir->entry.entryCount-1 ? 2 : 1);
	}
}

Result fsDirGotoParentDir(void)
{
	consoleLog("Opening -> %s ../\n", currentDir->entry.name);

	Result ret = 1;

	if (!currentDir->entry.isRootDirectory)
	{
		ret = fsGotoParentDir(&currentDir->entry);
		if (ret == 0)
		{
			fsDirRefreshDir(currentDir, true);
			fsStackPop(&currentDir->entryStack, &currentDir->entryOffsetId, &currentDir->entrySelectedId);
		}
	}

	return ret;
}

Result fsDirGotoSubDir(void)
{
	consoleLog("Opening -> %s/\n", currentDir->entrySelected->name);

	Result ret = 1;

	if (currentDir->entrySelected)
	{
		if (!currentDir->entrySelected->isRealDirectory)
		{
			if (!currentDir->entrySelected->isRootDirectory)
			{
				ret = fsDirGotoParentDir();
			}
		}
		else if (currentDir->entrySelected->isDirectory)
		{
			Result ret = fsGotoSubDir(&currentDir->entry, currentDir->entrySelected->name16);
			if (ret == 0)
			{
				fsStackPush(&currentDir->entryStack, currentDir->entryOffsetId, currentDir->entrySelectedId);
				fsDirRefreshDir(currentDir, true);
			}
		}
	}
	
	return ret;
}

/**
 * @brief Displays the overwrite warning and wait for any key.
 * @param path The path which causes the warning.
 * @return Whether the waited key was pressed.
 */
static bool fsWaitOverwrite(const u16* path)
{
	consoleSelect(&logConsole);
	printf("[path=");
	for (u16 i = 0; path[i] && i < FS_MAX_PATH_LENGTH; i++)
		printf("%c", (u8) path[i]);
	printf("]\n");
	printf("Overwrite detected!\n");
	printf("Press [Select] to confirm the overwrite.\n");
	consoleSelectDefault();

	return doKey(KEY_SELECT);
}

/**
 * @brief Displays the delete warning and wait for any key.
 * @param path The path which causes the warning.
 * @return Whether the waited key was pressed.
 */
static bool fsWaitDelete(const u16* path)
{
	consoleSelect(&logConsole);
	printf("[path=");
	for (u16 i = 0; path[i] && i < FS_MAX_PATH_LENGTH; i++)
		printf("%c", (u8) path[i]);
	printf("]\n");
	printf("Delete asked!\n");
	printf("Press [Select] to confirm the delete.\n");
	consoleSelectDefault();

	return doKey(KEY_SELECT);
}

/**
 * @brief Displays the out of resource warning and wait for any key.
 * @param path The path which causes the warning.
 * @return Whether the waited key was pressed.
 */
static bool fsWaitOutOfResource(const u16* path)
{
	consoleSelect(&logConsole);
	printf("[path=");
	for (u16 i = 0; path[i] && i < FS_MAX_PATH_LENGTH; i++)
		printf("%c", (u8) path[i]);
	printf("]\n");
	printf("The file was too big for the archive!\n");
	printf("Press any key to continue.\n");
	consoleSelectDefault();

	return doKey(KEY_ANY);
}

/**
 * @brief Copy an entry from a dir to another dir with overwrite option.
 * @param srcEntry The source entry to copy.
 * @param srcDir The source dir.
 * @param dstDir The destination dir.
 * @param overwrite Whether it shall overwrite the data.
 */
static Result fsDirCopy(fsEntry* srcEntry, fsDir* srcDir, fsDir* dstDir, bool overwrite)
{
	// TODO: UTF-16
	u16 len;

	if (srcEntry->isDirectory)
	{
		if (!srcEntry->isRealDirectory) return 1;

		fsEntry srcPath;
		// Create another fsEntry for the scan only.
		srcPath.attributes = srcEntry->attributes;
		srcPath.isDirectory = srcEntry->isDirectory;
		srcPath.isRealDirectory = srcEntry->isRealDirectory;
		srcPath.isRootDirectory = srcEntry->isRootDirectory;

		memset(srcPath.name16, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
		len = str16cpy(srcPath.name16, dstDir->entry.name16);
		if (!srcPath.isRootDirectory) str16cpy(srcPath.name16 + len, srcEntry->name16);

		if (fsDirExists(srcPath.name16, dstDir->archive) && !overwrite)
		{
			if (!fsWaitOverwrite(srcPath.name16)) return FS_USER_INTERRUPT;
			consoleLog("Overwrite validated!\n");
		}
		else if (!srcPath.isRootDirectory)
		{
			FSUSER_CreateDirectory(*dstDir->archive, fsMakePath(PATH_UTF16, srcPath.name16), FS_ATTRIBUTE_DIRECTORY);
		}

		memset(srcPath.name16, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
		if (!srcPath.isRootDirectory) len = str16cpy(srcPath.name16, srcDir->entry.name16); else len = 0;
		str16cpy(srcPath.name16 + len, srcEntry->name16);

		fsScanDir(&srcPath, srcDir->archive, false);
		fsEntry* next = srcPath.firstEntry;

		while (next)
		{
			srcPath.attributes = next->attributes;
			srcPath.isDirectory = next->isDirectory;
			srcPath.isRealDirectory = next->isRealDirectory;
			srcPath.isRootDirectory = next->isRootDirectory;

			memset(srcPath.name16, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
			len = str16cpy(srcPath.name16, srcEntry->name16);
			srcPath.name16[len++] = '/'; srcPath.name16[len] = '\0';
			str16cpy(srcPath.name16 + len, next->name16);

			fsDirCopy(&srcPath, srcDir, dstDir, overwrite);

			next = next->nextEntry;
		}

		fsFreeDir(&srcPath);

		return 1;
	}
	else
	{
		u16 srcPath[FS_MAX_PATH_LENGTH];
		u16 dstPath[FS_MAX_PATH_LENGTH];

		memset(srcPath, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
		len = str16cpy(srcPath, srcDir->entry.name16);
		str16cpy(srcPath + len, srcEntry->name16);

		memset(dstPath, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
		len = str16cpy(dstPath, dstDir->entry.name16);
		str16cpy(dstPath + len, srcEntry->name16);

		if (fsFileExists(dstPath, dstDir->archive) && !overwrite)
		{
			if (!fsWaitOverwrite(dstPath)) return FS_USER_INTERRUPT;
			consoleLog("Overwrite validated!\n");
		}

		Result ret = fsCopyFile(srcPath, srcDir->archive, dstPath, dstDir->archive, srcEntry->attributes);

		if (ret == FS_OUT_OF_RESOURCE || ret == FS_OUT_OF_RESOURCE_2)
		{
			fsWaitOutOfResource(dstPath);
			FSUSER_DeleteFile(*dstDir->archive, fsMakePath(PATH_UTF16, dstPath));
		}

		return ret;
	}

	return 1;
}

Result fsDirCopyCurrentEntry(bool overwrite)
{
	Result ret = fsDirCopy(currentDir->entrySelected, currentDir, dickDir, overwrite);
	fsDirRefreshDir(dickDir, true);
	return ret;
}

Result fsDirCopyCurrentFolder(bool overwrite)
{
	fsEntry entry;
	memset(entry.name, 0, FS_MAX_PATH_LENGTH);
	memset(entry.name16, 0, FS_MAX_PATH_LENGTH);
	entry.attributes = currentDir->entry.attributes;
	entry.isDirectory = true;
	entry.isRealDirectory = true;
	entry.isRootDirectory = false;

	Result ret = fsDirCopy(&entry, currentDir, dickDir, overwrite);
	fsDirRefreshDir(dickDir, true);
	return ret;
}

Result fsDirDeleteCurrentEntry(void)
{
	Result ret = -3;

	u16 len;
	u16 path[FS_MAX_PATH_LENGTH];
	memset(path, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
	len = str16cpy(path, currentDir->entry.name16);
	str16cpy(path + len, currentDir->entrySelected->name16);

	if (currentDir->entrySelected->isDirectory)
	{
		if (currentDir->entrySelected->isRealDirectory)
		{
			if (!fsWaitDelete(path)) return FS_USER_INTERRUPT;
			consoleLog("Delete validated!\n");

			ret = FSUSER_DeleteDirectoryRecursively(*currentDir->archive, fsMakePath(PATH_UTF16, path));

			fsDirRefreshDir(currentDir, true);
		}
	}
	else
	{
		if (!fsWaitDelete(path)) return FS_USER_INTERRUPT;
		consoleLog("Delete validated!\n");

		ret = FSUSER_DeleteFile(*currentDir->archive, fsMakePath(PATH_UTF16, path));

		fsDirRefreshDir(currentDir, true);
	}

	return ret;
}

fsDir backDir;

void fsBackInit(u64 titleid)
{
	memset(&backDir, 0, sizeof(fsDir));
	
	sprintf(backDir.entry.name, "/backup/%016llx/", titleid);

	// TODO: UTF-16
	utf8_to_utf16(backDir.entry.name16, backDir.entry.name, strlen(backDir.entry.name));

	backDir.archive = &sdmcArchive;

	FS_CreateDirectory("/backup/", backDir.archive);
	FSUSER_CreateDirectory(*backDir.archive, fsMakePath(PATH_UTF16, backDir.entry.name16), FS_ATTRIBUTE_DIRECTORY);

	fsDirRefreshDir(&backDir, false);
}

void fsBackExit(void)
{
	fsFreeDir(&backDir.entry);

	while (fsStackPop(&backDir.entryStack, NULL, NULL) == 0);
}

/**
 * @brief Prints a directory to the current console.
 * @param dir The directory to print.
 * @param data An header string to print.
 */
static void fsBackPrint(fsDir* dir, const char* data)
{
	s32 i = 0;
	u8 row = 3;
	fsEntry* next = dir->entry.firstEntry;

	// Skip the first off-screen entries
	for (; next && i < dir->entryOffsetId; i++)
	{
		next = next->nextEntry;
	}

	consoleClear();

	consoleResetColor();
	printf("\x1B[0;0H%s data:", data);
	consoleForegroundColor(TEAL);
	printf("\x1B[1;0H%.25s", dir->entry.name);
	consoleResetColor();

	for (; next && i < dir->entry.entryCount && i < dir->entryOffsetId + entryPrintCount; i++)
	{
		// If the entry is the current entry
		if (dir->entrySelectedId == i)
		{
			consoleBackgroundColor(SILVER);
			if (next->isDirectory) consoleForegroundColor(TEAL);
			else consoleForegroundColor(BLACK);

			// Blank placeholder
			printf("\x1B[%u;0H \a                       ", row);

			// Tricky-hacky
			dir->entrySelected = next;
		}
		// Else if the entry is just a directory or a simple file
		else if (next->isDirectory) consoleForegroundColor(CYAN);
		else consoleForegroundColor(WHITE);

		// Display entry's name
		printf("\x1B[%u;3H%.22s", row++, next->name);
		consoleResetColor();

		// Iterate though linked list
		next = next->nextEntry;
	}
}

void fsBackPrintSave(void)
{
	fsDir saveDir;
	memset(&saveDir, 0, sizeof(fsDir));

	saveDir.entry.name16[0] = '/';
	saveDir.entry.name16[1] = '\0';
	
	// TODO: Remove when native UTF-16 font.
	strcpy(saveDir.entry.name, "/");

	saveDir.archive = &saveArchive;
	saveDir.entryOffsetId = 0;
	saveDir.entrySelectedId = -1;
	saveDir.entry.isDirectory = true;
	saveDir.entry.isRealDirectory = true;
	saveDir.entry.isRootDirectory = true;

	fsScanDir(&saveDir.entry, saveDir.archive, false);

	consoleSelectNew(&saveConsole);
	fsBackPrint(&saveDir, "Save");
	consoleSelectLast();

	fsFreeDir(&saveDir.entry);
}

void fsBackPrintBackup(void)
{
	consoleSelectNew(&sdmcConsole);
	fsBackPrint(&backDir, "Backup");
	consoleSelectLast();
}

void fsBackMove(s16 count)
{
	// That bitchy was pretty hard... :/

	backDir.entrySelectedId += count;

	if (backDir.entrySelectedId < 0)
	{
		backDir.entrySelectedId = backDir.entry.entryCount-1;
		backDir.entryOffsetId = (backDir.entry.entryCount > entryPrintCount ?
			backDir.entry.entryCount - entryPrintCount : 0);
	}

	if (backDir.entrySelectedId > backDir.entry.entryCount-1)
	{
		backDir.entrySelectedId = 0;
		backDir.entryOffsetId = 0;
	}

	if (backDir.entryOffsetId >= backDir.entrySelectedId)
	{
		backDir.entryOffsetId = (backDir.entrySelectedId > 0 ?
			backDir.entrySelectedId-1 : backDir.entrySelectedId);
	}

	else if (backDir.entrySelectedId > entryPrintCount-2 && count > 0 &&
		backDir.entryOffsetId + entryPrintCount-2 < backDir.entrySelectedId)
	{
		backDir.entryOffsetId = (backDir.entrySelectedId < backDir.entry.entryCount-1 ?
			backDir.entrySelectedId+1 : backDir.entrySelectedId) - entryPrintCount+1;
	}
}

Result fsBackExport(void)
{
	// (save->sdmc)
	// TODO: Create the new entry backup.
	// TODO: Copy the whole save archive content to the new entry.

	Result ret;

	// The current time for the backup name.
	time_t t_time = time(NULL);
	struct tm* tm_time = gmtime(&t_time);

	// The backup folder name.
	char path8[FS_MAX_PATH_LENGTH];
	memset(path8, 0, FS_MAX_PATH_LENGTH);
	sprintf(path8, "%04u-%02u-%02u--%02u-%02u-%02u/",
		tm_time->tm_year+1900,
		tm_time->tm_mon+1,
		tm_time->tm_yday,
		tm_time->tm_hour,
		tm_time->tm_min+tm_time->tm_sec/60,
		tm_time->tm_sec%60
	);

	// TODO: UTF-16
	u16 path[FS_MAX_PATH_LENGTH];
	memset(path, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
	utf8_to_utf16(path, (u8*) path8, strlen(path8));

	// The root dir of the save archive.
	fsDir saveDir;
	memset(&saveDir, 0, sizeof(fsDir));
	saveDir.archive = &saveArchive;
	saveDir.entry.isDirectory = true;
	saveDir.entry.isRealDirectory = true;
	saveDir.entry.isRootDirectory = true;

	saveDir.entry.name16[0] = '/';
	saveDir.entry.name16[1] = '\0';

	// TODO: Remove when native UTF-16 font.
	strcpy(saveDir.entry.name, "/");

	// Create the backup directory.
	ret = FSUSER_CreateDirectory(sdmcArchive, fsMakePath(PATH_UTF16, backDir.entry.name16), FS_ATTRIBUTE_DIRECTORY);

	// Go to the backup directory.
	fsFreeDir(&backDir.entry);
	fsGotoSubDir(&backDir.entry, path);

	// Copy the current save directory to the sdmc archive
	ret = fsDirCopy(&saveDir.entry, &saveDir, &backDir, true);

	// Reset the current directory to default.
	fsGotoParentDir(&backDir.entry);
	fsScanDir(&backDir.entry, backDir.archive, false);

	return ret;
}

Result fsBackImport(void)
{
	// (sdmc->save)
	// TODO: Delete the whole save archive content.
	// TODO: Copy the selected entry content to the save archive.

	Result ret;

	// The root dir of the save archive.
	fsDir saveDir;
	memset(&saveDir, 0, sizeof(fsDir));
	saveDir.archive = &saveArchive;
	saveDir.entry.isDirectory = true;
	saveDir.entry.isRealDirectory = true;
	saveDir.entry.isRootDirectory = false;

	saveDir.entry.name16[0] = '/';
	saveDir.entry.name16[1] = '\0';

	// TODO: Remove when native UTF-16 font.
	strcpy(saveDir.entry.name, "/");

	// Delete the save archive content.
	ret = FSUSER_DeleteDirectoryRecursively(*saveDir.archive, fsMakePath(PATH_UTF16, saveDir.entry.name16));

	// Go to the backup directory.
	fsFreeDir(&backDir.entry);
	fsGotoSubDir(&backDir.entry, backDir.entrySelected->name16);

	// The fake entry to copy the current directory to.
	fsEntry entry;
	memset(entry.name, 0, FS_MAX_PATH_LENGTH);
	entry.attributes = backDir.entry.attributes;
	entry.isDirectory = true; // backDir.entry.isDirectory
	entry.isRealDirectory = true; // backDir.entry.isRealDirectory
	entry.isRootDirectory = false; // backDir.entry.isRootDirectory

	// Copy the current directory content to the save archive.
	ret = fsDirCopy(&entry, &backDir, &saveDir, true);

	// Reset the current directory to default.
	fsGotoParentDir(&backDir.entry);
	fsScanDir(&backDir.entry, backDir.archive, false);

	return ret;
}

Result fsBackDelete(void)
{
	Result ret = -3;

	u16 len;
	u16 path[FS_MAX_PATH_LENGTH];
	memset(path, 0, FS_MAX_PATH_LENGTH*sizeof(u16));
	len = str16cpy(path, backDir.entry.name16);
	str16cpy(path + len, backDir.entrySelected->name16);

	if (!fsWaitDelete(path)) return FS_USER_INTERRUPT;
	consoleLog("Delete validated!\n");

	ret = FSUSER_DeleteDirectory(*backDir.archive, fsMakePath(PATH_UTF16, path));

	fsDirRefreshDir(&backDir, false);

	return ret;
}
