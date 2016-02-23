#pragma once
/**
 * @file fsdir.h
 * @brief Filesystem Directory Module
 */

#include "fsls.h"

#include <3ds/services/fs.h>

#define FS_USER_INTERRUPT (0x8000DEAD)

/// A stack node for fsDir.
typedef struct fsStackNode
{
	s16 offsetId;
	s16 selectedId;
	struct fsStackNode* prev;
} fsStackNode;

/// A stack for fsDir.
typedef struct
{
	struct fsStackNode* last;
} fsStack;

/**
 * @brief Push a value to a stack.
 * @param stack The stack to push.
 * @param value The value to push.
 */
Result fsStackPush(fsStack* stack, s16 offsetId, s16 selectedId);

/**
 * @brief Pop a value from a stack.
 * @param stack The stack to pop.
 * @param value The value popped.
 */
Result fsStackPop(fsStack* stack, s16* offsetId, s16* selectedId);

/// The directory entry to read from, inherits from fsEntry.
typedef struct fsDir
{
	fsEntry entry;			///< The mother fsEntry.
	fsEntry* entrySelected;	///< The current entry selection.
	fsStack entryStack;		///< The stack of parent folders.
	s16 entryOffsetId;		///< The current entry offset.
	s16 entrySelectedId;	///< The current entry selection.
	FS_Archive* archive;	///< The archive of the dir.
} fsDir;

/// The save fsDir for fsDir.
extern fsDir saveDir;
/// The sdmc fsDir for fsDir.
extern fsDir sdmcDir;

/**
 * @brief Initializes fsDir.
 */
Result fsDirInit(void);

/**
 * @brief Exits fsDir.
 */
Result fsDirExit(void);

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
 * @brief Prints the dick dir in its console.
 */
void fsDirPrintDick(void);

/**
 * @brief Frees and scans a directory
 * @param dir The dir to refresh
 */
void fsDirRefreshDir(fsDir* dir, bool addParentDir);

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
Result fsDirGotoParentDir(void);

/**
 * @brief Goes to the sub dir of the current dir according to the selected entry name.
 */
Result fsDirGotoSubDir(void);

/**
 * @brief Copies the current entry to the other dir.
 */
Result fsDirCopyCurrentEntry(bool overwrite);

/**
 * @brief Copies the current directory to the other dir.
 */
Result fsDirCopyCurrentFolder(bool overwrite);

/**
 * @brief Deletes the current entry.
 */
Result fsDirDeleteCurrentEntry(void);

/// The sdmc fsDir for fsBack.
extern fsDir backDir;

/**
 * @brief Initializes fsBack.
 */
Result fsBackInit(u64 titleid);

/**
 * @brief Exits fsBack.
 */
Result fsBackExit(void);

/**
 * @brief Prints the save dir in its console.
 */
void fsBackPrintSave(void);

/**
 * @brief Prints the backup dir in its console.
 */
void fsBackPrintBackup(void);

/**
 * @brief Moves the cursor backup.
 * @param count The count to move the cursor.
 */
void fsBackMove(s16 count);

/**
 * @brief Exports a new backup. (save->sdmc)
 */
Result fsBackExport(void);

/**
 * @brief Imports the current backup. (sdmc->save)
 */
Result fsBackImport(void);

/**
 * @brief Deletes the current backup.
 */ 
Result fsBackDelete(void);
