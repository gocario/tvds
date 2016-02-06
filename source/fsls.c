#include "fsls.h"
#include "fs.h"
#include "utils.h"
#include "console.h"

#include <3ds/result.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define r(format, args...) consoleLog(format, ##args)
#define r(format, args...)

Result fsCopyFile(char* srcPath, FS_Archive* srcArchive, char* dstPath, FS_Archive* dstArchive, u32 attributes, bool overwrite)
{
	consoleSelect(&logConsole); consoleClear(); consoleSelectDefault(); // TODO REMOVE
	Result ret;
	Handle srcHandle, dstHandle;

	ret = FSUSER_OpenFile(&dstHandle, *dstArchive, fsMakePath(PATH_ASCII, dstPath), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	r(" > FSUSER_OpenFile: %lx\n", ret);

	if (R_SUCCEEDED(ret))
	{
		ret = FSFILE_Close(dstHandle);
		r(" > FSFILE_Close: %lx\n", ret);

		if (!overwrite)
			return FS_OVERWRITE;
	}

	ret = FSUSER_OpenFile(&dstHandle, *dstArchive, fsMakePath(PATH_ASCII, dstPath), FS_OPEN_WRITE | FS_OPEN_CREATE, attributes);
	r(" > FSUSER_OpenFile: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = FSUSER_OpenFile(&srcHandle, *srcArchive, fsMakePath(PATH_ASCII, srcPath), FS_OPEN_READ, attributes);
	r(" > FSUSER_OpenFile: %lx\n", ret);

	if (R_SUCCEEDED(ret))
	{
		u8* buffer;
		u32 bytes = 0;
		u64 size = 0;

		consoleLog("src: %lu\ndst: %lu\n\n", srcHandle, dstHandle);

		ret = FSFILE_GetSize(srcHandle, &size);
		r(" > FSFILE_GetSize: %lx\n", ret);

		buffer = (u8*) malloc(size);

		ret = FSFILE_Read(srcHandle, &bytes, 0, buffer, size);
		r(" > FSFILE_Read: %lx\n", ret);

		ret = FSFILE_Write(dstHandle, &bytes, 0, buffer, size, FS_WRITE_FLUSH);
		r(" > FSFILE_Read: %lx\n", ret);
		
		free(buffer);
	}

	ret = FSFILE_Close(srcHandle);
	r(" > FSFILE_Close: %lx\n", ret);
	ret = FSFILE_Close(dstHandle);
	r(" > FSFILE_Close: %lx\n", ret);

	return 0;
}

Result fsScanDir(fsEntry* dir, FS_Archive* archive, bool rec)
{
	if (!dir || !archive) return -1;
	consoleLog("scanDir(\"%s\", %li)\n", dir->name, archive->id);

	Result ret;
	Handle dirHandle;

	dir->firstEntry = NULL;
	dir->entryCount = 0;

	ret = FSUSER_OpenDirectory(&dirHandle, *archive, fsMakePath(PATH_ASCII, dir->name));
	r(" > FSUSER_OpenDirectory: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	u32 entriesRead;
	fsEntry* lastEntry = NULL;

	bool alphasort = true;

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

			unicodeToChar(entry->name, dirEntry.name, FS_MAX_FPATH_LENGTH);

			consoleLog("Entry: %s (%i)\n", entry->name, dir->entryCount+1);
			
			entry->attributes = dirEntry.attributes;
			entry->isDirectory = entry->attributes & FS_ATTRIBUTE_DIRECTORY;
			entry->isRealDirectory = true;
			entry->isRootDirectory = false;
			entry->nextEntry = NULL;
			entry->firstEntry = NULL;
			entry->entryCount = 0;

			if (rec && entry->attributes & FS_ATTRIBUTE_DIRECTORY)
			{
				sprintf(entry->name, "%s/%s/", dir->name, entry->name);
				fsScanDir(entry, archive, rec);
				unicodeToChar(entry->name, dirEntry.name, FS_MAX_FPATH_LENGTH);
			}

			/** Add the entry to the list ** START **/

			// Set the next entries
			if (dir->firstEntry)
			{
				
				if (alphasort)
				{
					fsEntry* tmpPrevEntry = dir->firstEntry;
					fsEntry* tmpNextEntry = dir->firstEntry;

					while (tmpNextEntry && tmpPrevEntry)
					{
						if (entry->isDirectory != tmpNextEntry->isDirectory)
						{
							if (entry->isDirectory)
							{
								entry->nextEntry = tmpPrevEntry->nextEntry;
								tmpPrevEntry->nextEntry = entry;

								tmpPrevEntry = NULL;
							}
							else
							{
								tmpPrevEntry = tmpNextEntry;
								tmpNextEntry = tmpPrevEntry->nextEntry;
							}
						}
						else if (strcmp(entry->name, tmpNextEntry->name) < 0)
						{
							entry->nextEntry = tmpPrevEntry->nextEntry;
							tmpPrevEntry->nextEntry = entry;

							tmpPrevEntry = NULL;
						}
						else
						{
							tmpPrevEntry = tmpNextEntry;
							tmpNextEntry = tmpPrevEntry->nextEntry;
						}
					}

					// Append the entry to the end if eof
					if (tmpPrevEntry)
					{
						tmpPrevEntry->nextEntry = entry;
						entry->nextEntry = tmpNextEntry;
					}
				}
				else
				{
					// Append the entry to the end LAST_ENTRY
					lastEntry->nextEntry = entry;
					lastEntry = entry;
				}
			}
			// Set the first entry
			else
			{
				dir->firstEntry = entry;
				lastEntry = entry;
			}

			dir->entryCount++;

			/** Add the next entry to the list ** END **/
		}
		else if (dir->entryCount == 0)
		{
			consoleLog("Empty folder!\n\n");
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
	char* dest = dir->name + strlen(dir->name);
	strncat(dest, subDir, (dest - path) + FS_MAX_PATH_LENGTH - 1);
	if (dest[strlen(dest+1)] != '/')
		dest = strcat(dest, "/");

	return 0;
}
