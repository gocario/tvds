#pragma once
/**
 * @file fsdir.h
 */

#include "fsls.h"

#include <3ds/services/fs.h>

/// The directory entry to read from, inherits from fsEntry.
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
 * @brief Prints the dick dir in its console.
 */
void fsDirPrintDick(void);

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
 * @brief Copies the current file to the other dir.
 */
Result fsDirCopyCurrentFile(void);

/**
 * @brief Copies the current directory to the other dir.
 */
Result fsDirCopyCurrentFolder(void);
