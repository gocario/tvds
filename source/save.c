#include "save.h"
#include "fs.h"

#include <3ds/result.h>
#include <3ds/services/apt.h>

Result saveInit()
{
	// TODO Initialize save module.
	return 0;
}

Result saveGetTitleId(u64* titleId)
{
	if (!titleId) return -1;

	Result ret;

	aptOpenSession();

	ret = APT_GetProgramID(titleId);
	if (R_FAILED(ret)) *titleId = 0;

	aptCloseSession();

	return ret;
}

Result saveBackup()
{
	// TODO Backup the save to a default folder
	return 0;
}

Result saveRemoveSecureValue(u64 titleId, FS_MediaType mediaType, u8* res)
{
	if (mediaType == MEDIATYPE_GAME_CARD) return -1;

	Result ret;
	u64 in;
	u8 out;

	in = ((u64) SECUREVALUE_SLOT_SD << 32) | (titleId & 0xFFFFFF00);
	ret = FSUSER_ControlSecureSave(SECURESAVE_ACTION_DELETE, &in, 8, &out, 1);
	if (R_FAILED(ret)) return ret;

	if (res) *res = out;

	return ret;
}
