#pragma once
/**
 * @file save.h
 * @brief Save
 */
#include <3ds/types.h>
#include <3ds/services/fs.h>

Result saveInit();

/**
 * @brief Retrieves some data of the current process.
 * @param[out] titleIdThe title id returned.
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
 * @param[out] res The result value.
 */
Result saveRemoveSecureValue(u64 titleId, FS_MediaType mediaType, u8* res);
