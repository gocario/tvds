#pragma once
/**
 * @file fsls.h
 */

#include <3ds/services/fs.h>

#define FS_MAX_FPATH_LENGTH (0x106) // 0x106
#define FS_MAX_PATH_LENGTH (0x400) // 0x400

#define FS_OUT_OF_RESOURCE (0xD8604664)

/// An entry of a file or a directory.
typedef struct fsEntry
{
	char name[FS_MAX_PATH_LENGTH];	///< The name as char.
	u32 attributes;					///< The attributes. (Is FS_DIRECTORY?)
	bool isDirectory : 1;			///< If FS_DIRECTORY.
	bool isRealDirectory : 1;		///< If FS_REAL_DIRECTORY.
	bool isRootDirectory : 1;		///< If FS_ROOT_DIRECTORY.
	unsigned : 5;
	struct fsEntry* nextEntry;		///< The next entry (linked list).
	struct fsEntry* firstEntry;		///< The first entry (child). FS_DIRECTORY
	u16 entryCount;					///< The count of the entries (childs). FS_DIRECTORY
} fsEntry;

/**
 * @todo comment
 */
bool fsFileExists(char* path, FS_Archive* archive);

/**
 * @todo comment
 */
bool fsDirExists(char* path, FS_Archive* archive);

/**
 * @todo comment
 */
Result fsCopyFile(char* srcPath, FS_Archive* srcArchive, char* dstPath, FS_Archive* dstArchive, u32 attributes, bool overwrite);

/**
 * @todo comment
 */
Result fsScanDir(fsEntry* dir, FS_Archive* archive, bool rec);

/**
 * @todo comment
 */
Result fsFreeDir(fsEntry* dir);

/**
 * @todo comment
 */
Result fsAddParentDir(fsEntry* dir);

/**
 * @todo comment
 */
Result fsGotoParentDir(fsEntry* dir);

/**
 * @todo comment
 */
Result fsGotoSubDir(fsEntry* dir, char* subDir);
