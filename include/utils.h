#pragma once

#include <3ds/types.h>

/// Do NOT use that!
inline u16 unicodeToChar(char* dst, const u16* src, s16 max)
{
	if (!src || !dst) return 0;
	u16 ii;
	for (ii = 0; src[ii] && ii < max-1; ii++)
		dst[ii] = src[ii] & 0xFF;
	dst[ii] = '\0';
	return ii;
}

inline u16 str16len(const u16* str)
{
	if (!str) return 0;
	u16 ii;
	for (ii = 0; str[ii]; ii++);
	return ii;
}

inline u16 str16cpy(u16* dst, const u16* src)
{
	if (!src || !dst) return 0;
	u16 ii;
	for (ii = 0; src[ii]; ii++)
		dst[ii] = src[ii];
	dst[ii] = '\0';
	return ii;
}

inline u16 str16ncpy(u16* dst, const u16* src, s16 max)
{
	if (!src || !dst) return 0;
	u16 ii;
	for (ii = 0; src[ii] && ii < max-1; ii++)
		dst[ii] = src[ii];
	dst[ii] = '\0';
	return ii;
}

inline s32 str16cmp(const u16* str1, const u16* str2)
{
	if (!str1 || !str2) return 0;
	u16 ii;
	for (ii = 0; str1[ii] && str2[ii] && str1[ii] == str2[ii]; ii++);
	return str1[ii] - str2[ii];
}

inline s32 str16ncmp(const u16* str1, const u16* str2, s16 max)
{
	if (!str1 || !str2) return 0;
	for (u16 ii = 0; ii < max; ii++)
		if (!str1[ii] || !str2[ii] || str1[ii] != str2[ii])
			return str1[ii] - str2[ii];
	return 0;
}
