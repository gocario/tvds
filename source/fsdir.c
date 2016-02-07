#include "fsdir.h"
#include "fsls.h"
#include "fs.h"
#include "console.h"

#include <3ds/result.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define r(format, args...) consoleLog(format, ##args)
// #define r(format, args...)

fsDir saveDir;
fsDir sdmcDir;

fsDir* currentDir;
static fsDir* dickDir;

static u32 entryPrintCount = 20;

static void fsDirPrint(fsDir* dir, char* data);
static void fsDirRefreshDir(fsDir* _dir);
static Result fsDirCopy(fsEntry* srcEntry, fsDir* srcDir, fsDir* dstDir, bool overwrite);

Result fsDirInit(void)
{
	Result ret;

	memset(&saveDir, 0, sizeof(fsDir));
	memset(&sdmcDir, 0, sizeof(fsDir));

	strcpy(saveDir.entry.name, "/"); // TODO Replace by "/"
	strcpy(sdmcDir.entry.name, "/"); // TODO Replace by "/"

	// saveDir.archive = &sdmcArchive; // TODO Remove&Uncomment
	saveDir.archive = &saveArchive;
	saveDir.entryOffsetId = 0;
	saveDir.entrySelectedId = 0;
	ret = fsScanDir(&saveDir.entry, saveDir.archive, false);
	r(" > fsScanDir: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = fsAddParentDir(&saveDir.entry);
	r(" > fsAddParentDir: %lx\n", ret);

	sdmcDir.archive = &sdmcArchive;
	sdmcDir.entryOffsetId = 0;
	sdmcDir.entrySelectedId = 0;
	ret = fsScanDir(&sdmcDir.entry, sdmcDir.archive, false);
	r(" > fsScanDir: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = fsAddParentDir(&sdmcDir.entry);
	r(" > fsAddParentDir: %lx\n", ret);

	currentDir = &sdmcDir;
	dickDir = &saveDir;

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
	
	// Skip the first off-screen entries;
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
			// Tricky-hacky
			dir->entrySelected = next;

			consoleBackgroundColor(SILVER);
			if (next->isDirectory)
				consoleForegroundColor(TEAL);
			else
				consoleForegroundColor(BLACK);
			// Blank placeholder
			printf("\x1B[%u;0H \a                       ", row);
		}
		// Else if the entry is just a directory
		else if (next->isDirectory)
			consoleForegroundColor(CYAN);

		printf("\x1B[%u;3H%.22s", row++, next->name);
		consoleResetColor();
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

/**
 * @brief Refreshs the current dir (freeing and scanning it)
 */
static void fsDirRefreshDir(fsDir* _dir)
{
	fsDir* dir = (_dir ? _dir : currentDir);
	fsFreeDir(&dir->entry);
	fsScanDir(&dir->entry, dir->archive, false);
	fsAddParentDir(&dir->entry);

	dir->entrySelectedId = 0;
}

Result fsDirGotoParentDir(void)
{
	Result ret = 1;
	if (!currentDir->entry.isRootDirectory)
	{
		ret = fsGotoParentDir(&currentDir->entry);
		if (ret == 0) fsDirRefreshDir(currentDir);
	}
	return ret;
}

Result fsDirGotoSubDir(void)
{
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
			if (ret == 0) fsDirRefreshDir(currentDir);
		}
	}
	return ret;
}

/**
 * @todo comment static
 */
static Result fsDirCopy(fsEntry* srcEntry, fsDir* srcDir, fsDir* dstDir, bool overwrite)
{
	if (srcEntry->isDirectory)
	{
		if (!srcEntry->isRealDirectory) return 1;

		// TODO Implements rec copy for folders

		return 1;

		fsEntry srcPath;
		// Create another fsEntry for the scan only.
		srcPath.attributes = srcEntry->attributes;
		srcPath.isDirectory = true;
		srcPath.isRealDirectory = true;
		srcPath.isRootDirectory = !(strcmp("/", srcEntry->name));

		memset(srcPath.name, 0, sizeof(fsEntry));
		strcpy(srcPath.name, dstDir->entry.name);
		strcat(srcPath.name, srcEntry->name);
		consoleLog("\a%s\n", srcPath.name);

		FS_CreateDirectory(srcPath.name, dstDir->archive);

		fsScanDir(&srcPath, srcDir->archive, false);
		fsEntry* next = srcPath.firstEntry;

		while (next)
		{
			memset(srcPath.name, 0, sizeof(fsEntry));
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

		return fsCopyFile(srcPath, srcDir->archive, dstPath, dstDir->archive, srcEntry->attributes, overwrite);
	}

	return 1;
}

Result fsDirCopyCurrentFile(void)
{
	Result ret = fsDirCopy(currentDir->entrySelected, currentDir, dickDir, false);
	fsDirRefreshDir(dickDir);
	return ret;
}

Result fsDirCopyCurrentFileOverwrite(void)
{
	Result ret = fsDirCopy(currentDir->entrySelected, currentDir, dickDir, true);
	fsDirRefreshDir(dickDir);
	return ret;
}

Result fsDirCopyCurrentFolder(void)
{
	Result ret = fsDirCopy(&currentDir->entry, currentDir, dickDir, false);
	fsDirRefreshDir(dickDir);
	return ret;
}
