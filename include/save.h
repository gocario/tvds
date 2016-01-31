#pragma once
/**
 * @file save.h
 * @brief Save
 */
#ifndef SAVE_H
#define SAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds/types.h>
#include <3ds/services/fs.h>

Result saveInit();

/**
 * @brief Retrieves some data of the current process.
 * @param titleId @optional The title id returned;
 */
Result saveGetTitleId(u64* titleId);


Result saveBackup();

/**
 * @brief remove the secure value of the title in the NAND.
 */
Result saveRemoveSecureValue(u64 titleId, FS_MediaType mediaType, u8* res);

#ifdef __cplusplus
}
#endif

#endif // SAVE_H
