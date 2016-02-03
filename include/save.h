#pragma once
/**
 * @file save.h
 * @brief Save
 */
#ifndef SAVE_H
#define SAVE_H

#include <3ds/types.h>
#include <3ds/services/fs.h>

Result saveInit();

/**
 * @brief Retrieves some data of the current process.
 * @param titleId @optional The title id returned.
 */
Result saveGetTitleId(u64* titleId);

/**
 * @brief Backups the save to a default directory.
 */
Result saveBackup();

/**
 * @brief Clears the secure value of a title in the NAND.
 * @param titleId The title id of the title.
 * @param mediaType The media type of the title.
 * @param res The result value (out).
 */
Result saveRemoveSecureValue(u64 titleId, FS_MediaType mediaType, u8* res);

#endif // SAVE_H
