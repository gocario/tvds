#pragma once
/**
 * @file fsdir.h
 */

#include <3ds/services/fs.h>

#define FS_MAX_PATH_LENGTH (0x106)

/// An entry of a file or a directory.
typedef struct fsEntry
{
	char name[FS_MAX_PATH_LENGTH];	///< The name as char.
	u32 attributes : 29;			///< The attributes. (Is FS_DIRECTORY?)
	bool isDirectory : 1;			///< If FS_DIRECTORY.
	bool isRealDirectory : 1;		///< If FS_REAL_DIRECTORY.
	bool isRootDirectory : 1;		///< If FS_ROOT_DIRECTORY.
	struct fsEntry* nextEntry;		///< The next entry (linked list).
	struct fsEntry* firstEntry;		///< The first entry (child). FS_DIRECTORY
	u16 entryCount;					///< The count of the entries (childs). FS_DIRECTORY
} fsEntry;

/// The directory entry to readFrom, inherits from fsEntry.
typedef struct fsDir
{
	fsEntry entry;			///< The mother fsEntry.
	fsEntry* entrySelected;	///< The current entry selection.
	s16 entryOffsetId;		///< The current entry offset.
	s16 entrySelectedId;	///< The current entry selection.
	FS_Archive* archive;	///< The archive of the dir.
} fsDir;

extern fsDir saveDir;
extern fsDir sdmcDir;
extern fsDir* currentDir; // TODO: Remove

/**
 * @brief Initializes fsDir.
 */
Result fsDirInit(void);

/**
 * @brief Prints the save dir in its console.
 */
void fsDirPrintSave(void);

/**
 * @brief Prints the sdmc dir in its console.
 */
void fsDirPrintSdmc(void);

/**
 * @brief Prints the current dir in its console.
 */
void fsDirPrintCurrent(void);

/**
 * @brief Switchs the current dir.
 * @param dir The dir to switch to.
 */
void fsDirSwitch(fsDir* dir);

/**
 * @brief Moves the cursor dir.
 * @param count The count to move the cursor.
 */
void fsDirMove(s16 count);

/**
 * @brief Goes to the parent dir of the current dir.
 */
void fsDirGotoParentDir(void);

/**
 * @brief Goes to the sub dir of the current dir according to the selected entry name.
 */
void fsDirGotoSubDir(void);


/**
 * @param
 * @deprecated static only
 */
Result fsScanDir(fsEntry* dir, FS_Archive* archive, bool rec);

/**
 * @param
 * @deprecated static only
 */
Result fsFreeDir(fsEntry* dir);

/**
 * @param
 * @deprecated static only
 */
Result fsAddParentDir(fsEntry* dir);

/**
 * @param
 * @deprecated static only
 */
Result fsGotoParentDir(fsEntry* dir);

/**
 * @param
 * @deprecated static only
 */
Result fsGotoSubDir(fsEntry* dir, char* subDir);
