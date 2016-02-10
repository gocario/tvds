#pragma once
/**
 * @file key.h
 */
#ifndef KEY_H
#define KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds/services/apt.h>
#include <3ds/services/hid.h>
#include <3ds/services/gspgpu.h>

/**
 * @brief Key value.
 */
enum
{
	KEY_ANY = 0b11111111000100001100111111111111, ///< Any key
};

/**
 * @brief Blocks the running process until some keys are pressed.
 * @param key The key(s) to press.
 */
inline void waitKey(u32 key)
{
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown() & key) break;
	}
}

/**
 * @brief Bloacks the running process until any key is pressed.
 * @param key The key(s) to check for a single press.
 * @return Whether the key was pressed once.
 */
inline bool doKey(u32 key)
{
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown())
		{
			if (hidKeysDown() & key) return true;
			else return false;
		}
	}
	return false;
}

#ifdef __cplusplus
}
#endif

#endif // KEY_H
