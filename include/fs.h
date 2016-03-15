#pragma once
/**
 * @file fs.h
 * @brief Filesystem Services
 */
#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds/services/fs.h>

/// The data of a user savedata archive's lowpath.
typedef struct
{
	u32 mediatype;	///< The mediatype of the FS_Path.
	u32 lowid;		///< The lower word of the saveid.
	u32 highid;		///< The upper word of the saveid.
} FS_UserSaveData_LowPathData;

/// Attribute flags extended.
enum
{
	FS_ATTRIBUTE_NONE = 0,  ///< None.
};

extern FS_Archive sdmcArchive;
extern FS_Archive saveArchive;

#define FS_DEBUG_FIX_ARCHIVE
#ifdef FS_DEBUG_FIX_ARCHIVE
/**
 * @brief Redirects the saveArchive to the sdmcArchive for #Citra use
 * @return[in/out] Whether the archive fix is working.
 */
bool FSDEBUG_FixArchive(const FS_Archive** archive);
#endif

/**
 * @brief Reads a file (path) to dst.
 * @param[in] path The path of the file to read.
 * @param[out] dst The destination buffer.
 * @param maxSize The max size in bytes to read.
 * @param[in] archive The archive where the file is located.
 * @param[out] bytesRead The total of read bytes.
 */
Result FS_ReadFile(const char* path, void* dst, u64 maxSize, const FS_Archive* archive, u32* bytesRead);

/**
 * @brief Writes src to a file (path).
 * @param[in] path The path of the file to write.
 * @param[in] src The source buffer.
 * @param size The size in bytes to write.
 * @param[in] archive The archive where the file is located.
 * @param[out] bytesWritten The total of written bytes.
 */
Result FS_WriteFile(const char* path, const void* src, u64 size, const FS_Archive* archive, u32* bytesWritten);

/**
 * @brief Deletes a file (path).
 * @param[in] path The path of the file to delete.
 * @param[in] archive The archive where the file is located.
 */
Result FS_DeleteFile(const char* path, const FS_Archive* archive);

/**
 * @brief Creates a directory (path).
 * @param[in] path The path of the directory to create.
 * @param[in] archive The archive where the directory will be located.
 */
Result FS_CreateDirectory(const char* path, const FS_Archive* archive);

/**
 * @brief Deletes a directory (path).
 * @param[in] path The path of the directory to delete.
 * @param[in] archive The archive where the directory is located.
 */
Result FS_DeleteDirectory(const char* path, const FS_Archive* archive);

/**
 * @brief Deletes a directory recusively (path).
 * @param[in] path The path of the directory to delete.
 * @param[in] archive The archive where the directory is located.
 */
Result FS_DeleteDirectoryRecursively(const char* path, const FS_Archive* archive);

/**
 * @brief Commits an archive.
 * @param[in] The archive to commit.
 */
Result FS_CommitArchive(const FS_Archive* archive);

#ifdef __cia

// TODO: FS for cia build

#else

/**
 * @brief Initializes the filesystem service.
 */
Result FS_Init(void);

/**
 * @brief Exits the filesystem service.
 */
Result FS_Exit(void);

#endif // __cia

#ifdef __cplusplus
}
#endif

#endif // FS_H
