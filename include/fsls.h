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
	u16 name16[FS_MAX_PATH_LENGTH];	///< The name as UTF-16
	char name[FS_MAX_PATH_LENGTH];	///< The name as char
	u32 attributes;					///< The attributes (Is FS_DIRECTORY?)
	bool isDirectory : 1;			///< If FS_DIRECTORY
	bool isRealDirectory : 1;		///< If FS_REAL_DIRECTORY
	bool isRootDirectory : 1;		///< If FS_ROOT_DIRECTORY
	unsigned : 5;
	struct fsEntry* nextEntry;		///< The next entry (linked list)
	struct fsEntry* firstEntry;		///< The first entry (child) FS_DIRECTORY
	u16 entryCount;					///< The count of the entries (childs) FS_DIRECTORY
} fsEntry;

/**
 * @brief Checks if a file exists.
 * @param[in] path The path of the file.
 * @param[in] archive The archive of the file.
 * @return Whether the file exists.
 */
bool fsFileExists(const u16* path, const FS_Archive* archive);

/**
 * @brief Checks if a directory exists.
 * @param[in] path The path of the directory.
 * @param[in] archive The archive of the directory.
 * @return Whether the directory exists.
 */
bool fsDirExists(const u16* path, const FS_Archive* archive);

/**
 * @brief Copies a file from an archive to another archive.
 * @param[in] srcPath The path of the source file/directory.
 * @param[in] srcArchive The archive of the source file/directory.
 * @param[in] dstPath The path of the destination file/directory.
 * @param[in] dstArchive The archive of the destination file/directory.
 * @param attributes The attributes of the file/directory.
 */
Result fsCopyFile(const u16* srcPath, const FS_Archive* srcArchive, const u16* dstPath, const FS_Archive* dstArchive, u32 attributes);

/**
 * @brief Scans a directory based on an archive.
 * @param[in] dir The directory to scan.
 * @param[in] archive The archive to scan.
 * @param rec Whether the scan is recursive.
 */
Result fsScanDir(fsEntry* dir, const FS_Archive* archive, bool rec);

/**
 * @brief Frees the entries of a directory.
 * @param[in] dir The directory to free.
 */
Result fsFreeDir(fsEntry* dir);

/**
 * @brief Adds a virtual entry, which is the parentdir of a directory.
 * @param[in] dir The directory.
 */
Result fsAddParentDir(fsEntry* dir);

/**
 * @brief Goes to the parentdir of a directory.
 * @param[in] dir The directory.
 */
Result fsGotoParentDir(fsEntry* dir);

/**
 * @brief Goes to a subdir of a directory.
 * @param[in] dir The directory.
 * @param[in] subDir The subdir path.
 */
Result fsGotoSubDir(fsEntry* dir, const u16* subDir);
