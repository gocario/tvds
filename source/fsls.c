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

bool fsFileExists(const u16* path, const FS_Archive* archive)
{
	if (!path || !archive) return -1;

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;
	Handle fileHandle;

	ret = FSUSER_OpenFile(&fileHandle, *archive, fsMakePath(PATH_UTF16, path), FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	r(" > FSUSER_OpenDirectory: %lx\n", ret);
	if (R_SUCCEEDED(ret))
	{
		FSFILE_Close(fileHandle);
		r(" > FSFILE_Close\n");
	}

	return R_SUCCEEDED(ret);
}

bool fsDirExists(const u16* path, const FS_Archive* archive)
{
	if (!path || !archive) return -1;

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;
	Handle dirHandle;

	ret = FSUSER_OpenDirectory(&dirHandle, *archive, fsMakePath(PATH_UTF16, path));

	r(" > FSUSER_OpenDirectory: %lx\n", ret);
	if (R_SUCCEEDED(ret))
	{
		FSDIR_Close(dirHandle);
		r(" > FSDIR_Close\n");
	}

	return R_SUCCEEDED(ret);
}

Result fsCopyFile(const u16* srcPath, const FS_Archive* srcArchive, const u16* dstPath, const FS_Archive* dstArchive, u32 attributes)
{
	if (!srcPath || !srcArchive || !dstPath || !dstArchive) return -1;

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&srcArchive)) return -1;
#endif

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&dstArchive)) return -1;
#endif

	Result ret;
	Handle srcHandle, dstHandle;

	ret = FSUSER_OpenFile(&dstHandle, *dstArchive, fsMakePath(PATH_UTF16, dstPath), FS_OPEN_WRITE | FS_OPEN_CREATE, attributes);
	r(" > FSUSER_OpenFile: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	ret = FSUSER_OpenFile(&srcHandle, *srcArchive, fsMakePath(PATH_UTF16, srcPath), FS_OPEN_READ, attributes);
	r(" > FSUSER_OpenFile: %lx\n", ret);

	if (R_SUCCEEDED(ret))
	{
		u8* buffer;
		u32 bytes = 0;
		u64 size = 0;

		ret = FSFILE_GetSize(srcHandle, &size);
		r(" > FSFILE_GetSize: %lx\n", ret);

		buffer = (u8*) malloc(size);

		ret = FSFILE_Read(srcHandle, &bytes, 0, buffer, size);
		r(" > FSFILE_Read: %lx\n", ret);

		ret = FSFILE_Write(dstHandle, &bytes, 0, buffer, size, FS_WRITE_FLUSH);
		r(" > FSFILE_Read: %lx\n", ret);

		free(buffer);
	}

	FSFILE_Close(srcHandle);
	r(" > FSFILE_Close\n");
	FSFILE_Close(dstHandle);
	r(" > FSFILE_Close\n");

	return ret;
}

Result fsScanDir(fsEntry* dir, const FS_Archive* archive, bool rec)
{
	if (!dir || !archive) return -1;

#ifdef FS_DEBUG_FIX_ARCHIVE
	if (!FSDEBUG_FixArchive(&archive)) return -1;
#endif

	Result ret;
	Handle dirHandle;

	consoleLog("fsScanDir(\"%s\", %li)\n", dir->name, archive->id);

	dir->firstEntry = NULL;
	dir->entryCount = 0;

	ret = FSUSER_OpenDirectory(&dirHandle, *archive, fsMakePath(PATH_UTF16, dir->name16));
	r(" > FSUSER_OpenDirectory: %lx\n", ret);
	if (R_FAILED(ret)) return ret;

	u32 entriesRead;
	fsEntry* lastEntry = NULL;

	const bool alphasort = true;

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

			str16ncpy(entry->name16, dirEntry.name, FS_MAX_FPATH_LENGTH);

			// TODO: Remove when native UTF-16 font.
			unicodeToChar(entry->name, entry->name16, FS_MAX_FPATH_LENGTH);

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

				str16ncpy(entry->name16, dirEntry.name, FS_MAX_FPATH_LENGTH);
				
				// TODO: Remove when native UTF-16 font.
				unicodeToChar(entry->name, entry->name16, FS_MAX_FPATH_LENGTH);
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
						// If it's a file and a directory
						if (entry->isDirectory != tmpNextEntry->isDirectory)
						{
							if (entry->isDirectory)
							{
								if (tmpNextEntry == dir->firstEntry)
								{
									// Place first (entry-first)
									entry->nextEntry = dir->firstEntry;
									dir->firstEntry = entry;
								}
								else
								{
									// Place before (prev-entry-next)
									entry->nextEntry = tmpPrevEntry->nextEntry;
									tmpPrevEntry->nextEntry = entry;
								}

								tmpPrevEntry = NULL;
							}
							else
							{
								// Next entry
								tmpPrevEntry = tmpNextEntry;
								tmpNextEntry = tmpNextEntry->nextEntry;
							}
						}
						// Else if it is a bigger alpha string.
						else if (str16cmp(entry->name16, tmpNextEntry->name16) < 0)
						
						{
							if (tmpNextEntry == dir->firstEntry)
							{
								// Place first (entry-first)
								entry->nextEntry = dir->firstEntry;
								dir->firstEntry = entry;
							}
							else
							{
								// Place before (prev-entry-next)
								entry->nextEntry = tmpPrevEntry->nextEntry;
								tmpPrevEntry->nextEntry = entry;
							}

							tmpPrevEntry = NULL;
						}
						else
						{
							// Next entry
							tmpPrevEntry = tmpNextEntry;
							tmpNextEntry = tmpNextEntry->nextEntry;
						}
					}

					// Append the entry to the end if eof
					if (tmpPrevEntry)
					{
						// Place last (last-entry)
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
				// Place first and last
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

	FSDIR_Close(dirHandle);
	r(" > FSDIR_Close\n");

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
	if (!dir) return -1;

	// If the dir is the root directory. (no!)
	// dir->isRootDirectory = strcmp("/", dir->name) == 0 || strcmp("", dir->name) == 0;

	// TODO: UTF-16
	dir->isRootDirectory = (dir->name16[0] == '/' && dir->name16[1] == '\0') || dir->name16[0] == '\0';

	if (dir->isRootDirectory)
	{
		if (dir->firstEntry && !dir->firstEntry->isRealDirectory)
			return 2;

		fsEntry* root = (fsEntry*) malloc(sizeof(fsEntry));
		memset(root, 0, sizeof(fsEntry));
		root->attributes = dir->attributes | FS_ATTRIBUTE_DIRECTORY;
		root->isDirectory = true;
		root->isRealDirectory = false;
		root->isRootDirectory = true;
		root->nextEntry = dir->firstEntry;
		root->firstEntry = NULL;
		root->entryCount = 0;

		root->name16[0] = '/';
		root->name16[1] = '\0';

		// TODO: Remove when native UTF-16 font.
		unicodeToChar(root->name, root->name16, FS_MAX_PATH_LENGTH);

		dir->firstEntry = root;
		dir->entryCount++;

		return 0;
	}
	else
	{
		if (dir->firstEntry && !dir->firstEntry->isRealDirectory)
			return 2;

		fsEntry* root = (fsEntry*) malloc(sizeof(fsEntry));
		memset(root, 0, sizeof(fsEntry));
		root->attributes = dir->attributes | FS_ATTRIBUTE_DIRECTORY;
		root->isDirectory = true;
		root->isRealDirectory = false;
		root->isRootDirectory = false;
		root->nextEntry = dir->firstEntry;
		root->firstEntry = NULL;
		root->entryCount = 0;

		root->name16[0] = '.';
		root->name16[1] = '.';
		root->name16[2] = '\0';

		// TODO: Remove when native UTF-16 font.
		unicodeToChar(root->name, root->name16, FS_MAX_PATH_LENGTH);

		dir->firstEntry = root;
		dir->entryCount++;

		return 0;
	}

	return 1;
}

Result fsGotoParentDir(fsEntry* dir)
{
	consoleLog("fsGotoParentDir\n");
	if (!dir) return -1;

	u16* path = dir->name16;
	u16* p = path + str16len(path) - 1;
	if (*p == '/') *p-- = '\0';
	while (p > path && *p != '/') *p-- = '\0';

	// TODO: Remove when native UTF-16 font.
	unicodeToChar(dir->name, dir->name16, FS_MAX_FPATH_LENGTH);

	return 0;
}

Result fsGotoSubDir(fsEntry* dir, const u16* subDir)
{
	consoleLog("fsGotoSubDir\n");
	if (!dir || !subDir) return -1;

	u16* path = dir->name16;
	u16* dest = path + str16len(path);
	u16 len = str16ncpy(dest, subDir, (path - dest)/sizeof(u16) + FS_MAX_PATH_LENGTH - 1);
	if (dest[len-1] != '/') dest[len] = '/';

	// TODO: Remove when native UTF-16 font.
	unicodeToChar(dir->name, dir->name16, FS_MAX_FPATH_LENGTH);

	return 0;
}
