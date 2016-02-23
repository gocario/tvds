#pragma once
/**
 * @file fsls.h
 * @brief Filesystem Listing Module
 */

#include <3ds/services/fs.h>

#define FS_MAX_FPATH_LENGTH (0x106) // 0x106
#define FS_MAX_PATH_LENGTH (0x400) // 0x400

#define FS_OUT_OF_RESOURCE (0xD8604664)
#define FS_OUT_OF_RESOURCE_2 (0xC86044CD)

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
 * @brief Checks if a file exists.
 * @param path The path of the file.
 * @param archive The archive of the file.
 * @return Whether the file exists.
 */
bool fsFileExists(char* path, FS_Archive* archive);

/**
 * @brief Checks if a directory exists.
 * @param path The path of the directory.
 * @param archive The archive of the directory.
 * @return Whether the directory exists.
 */
bool fsDirExists(char* path, FS_Archive* archive);

/**
 * @brief Copies a file from an archive to another archive.
 * @param srcPath The path of the source file/directory.
 * @param srcArchive The archive of the source file/directory.
 * @param dstPath The path of the destination file/directory.
 * @param dstArchive The archive of the destination file/directory.
 * @param attributes The attributes of the file/directory.
 */
Result fsCopyFile(char* srcPath, FS_Archive* srcArchive, char* dstPath, FS_Archive* dstArchive, u32 attributes);

/**
 * @brief Scans a directory based on an archive.
 * @param dir The directory to scan.
 * @param archive The archive to scan.
 * @param rec Whether the scan is recursive.
 */
Result fsScanDir(fsEntry* dir, FS_Archive* archive, bool rec);

/**
 * @brief Frees the entries of a directory.
 * @param dir The directory to free.
 */
Result fsFreeDir(fsEntry* dir);

/**
 * @brief Adds a virtual entry, which is the parentdir of a directory.
 * @param dir The directory.
 */
Result fsAddParentDir(fsEntry* dir);

/**
 * @brief Goes to the parentdir of a directory.
 * @param dir The directory.
 */
Result fsGotoParentDir(fsEntry* dir);

/**
 * @brief Goes to a subdir of a directory.
 * @param dir The directory.
 * @param subDir The subdir path.
 */
Result fsGotoSubDir(fsEntry* dir, char* subDir);
