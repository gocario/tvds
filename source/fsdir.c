#include "fsdir.h"
#include "fsls.h"
#include "fs.h"
#include "key.h"
#include "console.h"

#include <3ds/os.h>
#include <3ds/result.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define r(format, args...) consoleLog(format, ##args)
// #define r(format, args...)

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

Result fsDirInit(void)
{
	memset(&saveDir, 0, sizeof(fsDir));
	memset(&sdmcDir, 0, sizeof(fsDir));

	strcpy(saveDir.entry.name, "/"); // TODO Replace by "/"
	strcpy(sdmcDir.entry.name, "/"); // TODO Replace by "/"

	saveDir.archive = &saveArchive;
	saveDir.entryOffsetId = 0;
	fsDirRefreshDir(&saveDir);

	sdmcDir.archive = &sdmcArchive;
	sdmcDir.entryOffsetId = 0;
	fsDirRefreshDir(&sdmcDir);

	currentDir = &sdmcDir;
	dickDir = &saveDir;

	return 0;
}

Result fsDirExit(void)
{
	fsFreeDir(&saveDir.entry);
	fsFreeDir(&sdmcDir.entry);

	while (fsStackPop(&saveDir.entryStack, NULL, NULL) == 0);
	while (fsStackPop(&sdmcDir.entryStack, NULL, NULL) == 0);

	return 0;
}

/**
 * @brief Prints a directory to the current console.
 * @param dir The directory to print.
 * @param data An header string to print.
 */
static void fsDirPrint(fsDir* dir, char* data)
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
		currentDir->entryOffsetId = (currentDir->entry.entryCount > entryPrintCount ?
			currentDir->entry.entryCount - entryPrintCount : 0);
	}

	if (currentDir->entrySelectedId > currentDir->entry.entryCount-1)
	{
		currentDir->entrySelectedId = 0;
		currentDir->entryOffsetId = 0;
	}

	if (currentDir->entryOffsetId >= currentDir->entrySelectedId)
	{
		currentDir->entryOffsetId = (currentDir->entrySelectedId > 0 ?
			currentDir->entrySelectedId-1 : currentDir->entrySelectedId);
	}

	else if (currentDir->entrySelectedId > entryPrintCount-2 && count > 0 &&
		currentDir->entryOffsetId + entryPrintCount-2 < currentDir->entrySelectedId)
	{
		currentDir->entryOffsetId = (currentDir->entrySelectedId < currentDir->entry.entryCount-1 ?
			currentDir->entrySelectedId+1 : currentDir->entrySelectedId) - entryPrintCount+1;
	}
}

Result fsDirGotoParentDir(void)
{
	consoleLog("Opening -> %s/../\n", currentDir->entry.name);

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
			ret = fsDirGotoParentDir();
		}
		else if (currentDir->entrySelected->isDirectory)
		{
			Result ret = fsGotoSubDir(&currentDir->entry, currentDir->entrySelected->name);
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
 * @brief Displays the overwrite warning an wait for a key.
 * @return Whether the waited key was pressed.
 */
static bool fsWaitOverwrite()
{
	consoleLog("Overwrite detected!\n");
	consoleLog("Press [Select] to confirm overwrite.\n");

	return doKey(KEY_SELECT);
}

/**
 * @brief Displays the delete warning an wait for a key.
 * @return Whether the waited key was pressed.
 */
static bool fsWaitDelete()
{
	consoleLog("Delete asked!\n");
	consoleLog("Press [Select] to confirm delete.\n");

	return doKey(KEY_SELECT);
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
	if (srcEntry->isDirectory)
	{
		if (!srcEntry->isRealDirectory) return 1;

		fsEntry srcPath;
		// Create another fsEntry for the scan only.
		srcPath.attributes = srcEntry->attributes;
		srcPath.isDirectory = true;
		srcPath.isRealDirectory = true;
		srcPath.isRootDirectory = !(strcmp("/", srcEntry->name));

		memset(srcPath.name, 0, FS_MAX_PATH_LENGTH);
		strcpy(srcPath.name, dstDir->entry.name);
		strcat(srcPath.name, srcEntry->name);
		consoleLog("\a%s\n", srcPath.name);

		if (fsDirExists(srcPath.name, dstDir->archive) && !overwrite)
		{
			if (!fsWaitOverwrite()) return FS_USER_INTERRUPT;
			consoleLog("Overwrite validated!\n");
		}
		else
		{
			FS_CreateDirectory(srcPath.name, dstDir->archive);
		}

		memset(srcPath.name, 0, FS_MAX_PATH_LENGTH);
		strcpy(srcPath.name, srcDir->entry.name);
		strcat(srcPath.name, srcEntry->name);
		consoleLog("\a%s\n", srcPath.name);

		fsScanDir(&srcPath, srcDir->archive, false);
		fsEntry* next = srcPath.firstEntry;

		while (next)
		{
			srcPath.attributes = next->attributes;
			srcPath.isDirectory = next->isDirectory;
			srcPath.isRealDirectory = next->isRealDirectory;
			srcPath.isRootDirectory = next->isRootDirectory;

			memset(srcPath.name, 0, FS_MAX_PATH_LENGTH);
			strcat(srcPath.name, srcEntry->name);
			strcat(srcPath.name, "/");
			strcat(srcPath.name, next->name);
			consoleLog("\a%s\n", srcPath.name);

			fsDirCopy(&srcPath, srcDir, dstDir, overwrite);

			next = next->nextEntry;
		}

		fsFreeDir(&srcPath);

		return 1;
	}
	else
	{
		char srcPath[FS_MAX_PATH_LENGTH];
		char dstPath[FS_MAX_PATH_LENGTH];
	
		memset(srcPath, 0, FS_MAX_PATH_LENGTH);
		strcpy(srcPath, srcDir->entry.name);
		strcat(srcPath, srcEntry->name);
		// strncat(srcPath, srcEntry->name, FS_MAX_PATH_LENGTH - strlen(srcPath));
		consoleLog("\a%s\n", srcPath);

		memset(dstPath, 0, FS_MAX_PATH_LENGTH);
		strcpy(dstPath, dstDir->entry.name);
		strcat(dstPath, srcEntry->name);
		// strncat(dstPath, srcEntry->name, FS_MAX_PATH_LENGTH - strlen(dstPath));
		consoleLog("\a%s\n", dstPath);

		if (fsFileExists(dstPath, dstDir->archive) && !overwrite)
		{
			if (!fsWaitOverwrite()) return FS_USER_INTERRUPT;
			consoleLog("Overwrite validated!\n");
		}

		return fsCopyFile(srcPath, srcDir->archive, dstPath, dstDir->archive, srcEntry->attributes);
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
	Result ret = fsDirCopy(&currentDir->entry, currentDir, dickDir, overwrite);
	fsDirRefreshDir(dickDir, true);
	return ret;
}

Result fsDirDeleteCurrentEntry(void)
{
	if (!fsWaitDelete()) return FS_USER_INTERRUPT;
	consoleLog("Delete validated!\n");

	Result ret = -3;

	char path[FS_MAX_PATH_LENGTH];
	memset(path, 0, FS_MAX_PATH_LENGTH);
	strcpy(path, currentDir->entry.name);
	strcat(path, currentDir->entrySelected->name);
	consoleLog("\a%s\n", path);

	if (currentDir->entrySelected->isDirectory)
	{
		if (!currentDir->entrySelected->isRealDirectory)
		{
			ret = FS_DeleteDirectoryRecursively(path, currentDir->archive);
			fsDirRefreshDir(currentDir, true);
		}
	}
	else
	{
		ret = FS_DeleteFile(path, currentDir->archive);
		fsDirRefreshDir(currentDir, true);
	}

	return ret;
}

fsDir backDir;

Result fsBackInit(u64 titleid)
{
	memset(&backDir, 0, sizeof(fsDir));

	sprintf(backDir.entry.name, "/backup/%016llx/", titleid);
	FS_CreateDirectory("/backup/", &sdmcArchive);
	FS_CreateDirectory(backDir.entry.name, &sdmcArchive);

	backDir.archive = &sdmcArchive;
	backDir.entryOffsetId = 0;
	fsDirRefreshDir(&backDir, false);

	return 0;
}

Result fsBackExit(void)
{
	fsFreeDir(&backDir.entry);

	while (fsStackPop(&backDir.entryStack, NULL, NULL) == 0);

	return 0;
}

void fsBackPrint(void)
{
	s32 i = 0;
	u8 row = 3;
	fsEntry* next = backDir.entry.firstEntry;
	
	// Skip the first off-screen entries
	for (; next && i < backDir.entryOffsetId; i++)
	{
		next = next->nextEntry;
	}

	consoleSelectNew(&sdmcConsole);
	consoleClear();
	
	consoleResetColor();
	printf("\x1B[0;0HBackup data:");
	consoleForegroundColor(TEAL);
	printf("\x1B[1;0H%.25s", backDir.entry.name);
	consoleResetColor();

	for (; next && i < backDir.entry.entryCount && i < backDir.entryOffsetId + entryPrintCount; i++)
	{
		// If the entry is the current entry
		if (backDir.entrySelectedId == i)
		{
			consoleBackgroundColor(SILVER);
			if (next->isDirectory) consoleForegroundColor(TEAL);
			else consoleForegroundColor(BLACK);
			// Blank placeholder
			printf("\x1B[%u;0H \a                       ", row);

			// Tricky-hacky
			backDir.entrySelected = next;
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

	fsEntry entry;
	sprintf(entry.name, "%s/%llu/", backDir.entry.name, osGetTime()); // TODO: Date/Time

	fsDir saveDir;
	memset(&saveDir, 0, sizeof(fsDir));
	strcpy(saveDir.entry.name, "/");
	saveDir.archive = &saveArchive;

	FS_CreateDirectory(entry.name, &sdmcArchive);

	fsScanDir(&saveDir.entry, saveDir.archive, false);

	ret = fsDirCopy(&entry, &saveDir, &backDir, true);

	fsFreeDir(&saveDir.entry);

	return ret;
}

Result fsBackImport(void)
{
	// (sdmc->save)

	// TODO: Delete the whole save archive content.

	// TODO: Copy the selected entry content to the save archive.

	Result ret;

	fsEntry entry;
	sprintf(entry.name, "%s/%s/", backDir.entry.name, backDir.entrySelected->name);

	fsDir saveDir;
	memset(&saveDir, 0, sizeof(fsDir));
	strcpy(saveDir.entry.name, "/");
	saveDir.archive = &saveArchive;

	ret = fsDirCopy(&currentDir->entry, &backDir, &saveDir, true);
	fsDirRefreshDir(dickDir);
	
	return ret;
}

Result fsBackDelete(void)
{
	if (!fsWaitDelete()) return FS_USER_INTERRUPT;
	consoleLog("Delete validated!\n");

	char path[FS_MAX_PATH_LENGTH];
	memset(path, 0, FS_MAX_PATH_LENGTH);
	strcpy(path, backDir.entry.name);
	strcat(path, backDir.entrySelected->name);
	consoleLog("\a%s\n", path);

	Result ret = FS_DeleteFile(path, backDir.archive);
	fsDirRefreshDir(&backDir);
	return ret;
}
