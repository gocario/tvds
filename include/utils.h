#pragma once

#include <3ds/types.h>

inline void unicodeToChar(char* dst, u16* src, s16 max)
{
	if (!src || !dst) return;
	u16 ii;
	for (ii = 0; *src && ii < max-1; ii++)
		*(dst + ii) = (*(src + ii)) & 0xFF;
	*(dst + ii) = 0x00;
}

inline void charToUnicode(u16* dst, char* src, s16 max)
{
	if (!src || !dst) return;
	u16 ii;
	for (ii = 0; *src && ii < max-1; ii++)
		*(dst + ii) = *(src + ii);
	*(dst + ii) = 0x00;
}