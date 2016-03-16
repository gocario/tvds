#pragma once

#include <3ds/types.h>

/// Do NOT use that!
inline void unicodeToChar(char* dst, const u16* src, s16 max)
{
	if (!src || !dst) return;
	u16 ii;
	for (ii = 0; *src && ii < max-1; ii++)
		*(dst + ii) = (*(src + ii)) & 0xFF;
	*(dst + ii) = 0x00;
}

/// Do NOT use that!
inline void charToUnicode(u16* dst, const char* src, s16 max)
{
	if (!src || !dst) return;
	u16 ii;
	for (ii = 0; *src && ii < max-1; ii++)
		*(dst + ii) = *(src + ii);
	*(dst + ii) = 0x00;
}

