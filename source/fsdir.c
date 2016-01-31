#include "fsdir.h"
#include "fs.h"
#include "console.h"

#include <3ds/result.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define r(format, args ...) printf(format, ##args)
#define r(format, args ...)

static inline void unicodeToChar(char* dst, u16* src, s16 max)
{
	if (!src || !dst) return;
	s16 ii;
	for (ii = 0; *src && ii < max-1; ii++) *(dst + ii) = (*(src + ii)) & 0xFF;
	*(dst + ii) = 0x00;
}

static inline void charToUnicode(u16* dst, char* src, s16 max)
{
	if (!src || !dst) return;
	s16 ii;
	for (ii = 0; *src && ii < max-1; ii++) *(dst + ii) = *(src + ii);
	*(dst + ii) = 0x00;
}

fsDir saveDir;
fsDir sdmcDir;

fsDir* currentDir;

static u32 entryPrintCount = 20;

Result fsDirInit(void)
{
	Result ret;

	memset(&saveDir, 0, sizeof(fsDir));
	memset(&sdmcDir, 0, sizeof(fsDir));

	strcpy(saveDir.entry.name, "/pk/backup/");
	strcpy(sdmcDir.entry.name, "/");

	saveDir.archive = &sdmcArchive; // TODO Change to &saveArchive
	saveDir.entryOffsetId = 0;
	saveDir.entrySelectedId = 0;
	ret = fsScanDir((fsEntry*) &saveDir, saveDir.archive, false);
	r(" > fsScanDir: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = fsAddParentDir((fsEntry*) &saveDir);
	r(" > fsAddParentDir: %lx\n", ret);

	sdmcDir.archive = &sdmcArchive;
	sdmcDir.entryOffsetId = 0;
	sdmcDir.entrySelectedId = 0;
	ret = fsScanDir((fsEntry*) &sdmcDir, sdmcDir.archive, false);
	r(" > fsScanDir: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = fsAddParentDir((fsEntry*) &sdmcDir);
	r(" > fsAddParentDir: %lx\n", ret);

	currentDir = &sdmcDir;

	return 0;
}

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
	printf("\x1B[1;0H%s", dir->entry.name);

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
			printf("\x1B[%u;0H >                       ", row);
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

void fsDirSwitch(fsDir* dir)
{
	if (dir == NULL)
	{
		if (currentDir == &saveDir)
			currentDir = &sdmcDir;
		else if (currentDir == &sdmcDir)
			currentDir = &saveDir;
	}
	else if (dir == &saveDir || dir == &sdmcDir)
	{
		currentDir = dir;
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

	// consoleLog("entrySelectedId: %i\n", currentDir->entrySelectedId);
	// consoleLog("entryOffsetId: %i\n", currentDir->entryOffsetId);
}

static void fsDirRefreshDir(void)
{
	fsFreeDir((fsEntry*) currentDir);
	fsScanDir((fsEntry*) currentDir, currentDir->archive, false);
	fsAddParentDir((fsEntry*) currentDir);

	currentDir->entrySelectedId = 0;
}

void fsDirGotoParentDir(void)
{
	if (!currentDir->entry.isRootDirectory)
	{
		if (fsGotoParentDir((fsEntry*) currentDir) == 0)
			fsDirRefreshDir();
	}
}

void fsDirGotoSubDir(void)
{
	if (currentDir->entrySelected)
	{
		if (!currentDir->entrySelected->isRealDirectory)
		{
			fsDirGotoParentDir();
		}
		else if (currentDir->entrySelected->isDirectory)
		{
			if (fsGotoSubDir((fsEntry*) currentDir, currentDir->entrySelected->name) == 0)
				fsDirRefreshDir();
		}
	}
}


Result fsScanDir(fsEntry* dir, FS_Archive* archive, bool rec)
{
	if (!dir || !archive) return -1;
	// printf("scanDir(\"%s\", %li)\n", dir->name, archive->id);

	Result ret;
	Handle dirHandle;

	dir->firstEntry = NULL;
	dir->entryCount = 0;

	ret = FSUSER_OpenDirectory(&dirHandle, *archive, fsMakePath(PATH_ASCII, dir->name));
	r(" > FSUSER_OpenDirectory: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	u32 entriesRead;
	fsEntry* lastEntry = NULL;

	do
	{
		FS_DirectoryEntry dirEntry;
		memset(&dirEntry, 0, sizeof(FS_DirectoryEntry));
		entriesRead = 0;

		ret = FSDIR_Read(dirHandle, &entriesRead, 1, &dirEntry);
		r(" > FSDIR_Read: %lx\n", ret);

		if (entriesRead > 0)
		{
			fsEntry* entry = (fsEntry*) malloc(sizeof(fsEntry));
			memset(entry, 0, sizeof(fsEntry));

			unicodeToChar(entry->name, dirEntry.name, FS_MAX_PATH_LENGTH);

			// printf("Entry: %s\n", entry->name);
			
			entry->attributes = dirEntry.attributes;
			entry->isDirectory = (entry->attributes & FS_ATTRIBUTE_DIRECTORY) == 1;
			entry->isRealDirectory = true;
			entry->isRootDirectory = false;
			entry->nextEntry = NULL;
			entry->firstEntry = NULL;
			entry->entryCount = 0;

			if (rec && entry->attributes & FS_ATTRIBUTE_DIRECTORY)
			{
				sprintf(entry->name, "%s/%s/", dir->name, entry->name);
				fsScanDir(entry, archive, rec);
				unicodeToChar(entry->name, dirEntry.name, FS_MAX_PATH_LENGTH);
			}

			if (dir->firstEntry)
			{
				lastEntry->nextEntry = entry;
			}
			else
			{
				dir->firstEntry = entry;
			}

			lastEntry = entry;

			dir->entryCount++;
		}
		else if (dir->entryCount == 0)
		{
			// printf("Empty folder!\n\n");
		}
	} while (entriesRead > 0);

	ret = FSDIR_Close(dirHandle);

	return ret;
}

Result fsFreeDir(fsEntry* dir)
{
	if (!dir) return -1;

	fsEntry* line = dir->firstEntry;
	fsEntry* next = NULL;

	while (line)
	{
		next = line->nextEntry;

		if (line->firstEntry)
		{
			fsFreeDir(line);
		}

		free(line);
		line = next;
	}

	dir->entryCount = 0;
	dir->firstEntry = NULL;

	return 0;
}

Result fsAddParentDir(fsEntry* dir)
{
	// If the dir is the root directory. (no!)
	dir->isRootDirectory = !strcmp("/", dir->name) || !strcmp("", dir->name);
	if (!dir->isRootDirectory)
	{
		fsEntry* root = (fsEntry*) malloc(sizeof(fsEntry));
		memset(root, 0, sizeof(fsEntry));

		strcpy(root->name, "..");
		root->attributes = dir->attributes | FS_ATTRIBUTE_DIRECTORY;
		root->isDirectory = true;
		root->isRealDirectory = false;
		root->isRootDirectory = false;
		root->nextEntry = dir->firstEntry;
		root->firstEntry = NULL;
		root->entryCount = 0;

		dir->firstEntry = root;
		dir->entryCount++;

		return 0;
	}
	return 1;
}

Result fsGotoParentDir(fsEntry* dir)
{
	if (!dir) return -1;

	char* path = dir->name;
	char* p = path + strlen(path) - 2;
	while (p > path && *p != '/') *p-- = '\0';

	return 0;
}

Result fsGotoSubDir(fsEntry* dir, char* subDir)
{
	if (!dir || !subDir) return -1;

	char* path = dir->name;
	path = strcat(path, subDir);
	if (path[strlen(path+1)] != '/')
		path = strcat(path, "/");		

	return 0;
}
