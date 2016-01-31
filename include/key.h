#pragma once
/**
 * @file key.h
 */
#ifndef KEY_H
#define KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds/services/hid.h>

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
		hidScanInput();
		if (hidKeysDown() & key) break;
	}
}

#ifdef __cplusplus
}
#endif

#endif // KEY_H
