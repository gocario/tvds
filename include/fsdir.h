#pragma once
/**
 * @file fsdir.h
 */

#include "fsls.h"

#include <3ds/services/fs.h>

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
	fsStackNode* last;
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

extern fsDir saveDir;
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
 * @brief Copies the current file to the other dir.
 * @see fsDirCopyCurrentFileOverwrite(void)
 */
Result fsDirCopyCurrentFile(void);

/**
 * @brief Overwrites the current file to the other dir.
 * @see fsDirCopyCurrentFile(void)
 */
Result fsDirCopyCurrentFileOverwrite(void);

/**
 * @brief Copies the current directory to the other dir.
 */
Result fsDirCopyCurrentFolder(void);
