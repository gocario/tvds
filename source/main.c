#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "fsdir.h"

#include "key.h"
#include "save.h"
#include "console.h"

typedef enum {
	STATE_EOF,		///< End of file
	STATE_ERROR,	///< Error
	STATE_BROWSE,	///< Browse the dir
} State;

static State state;

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInitDefault();

	Result ret;
	u64 titleId;
	state = STATE_BROWSE;

	ret = FS_fsInit();
	if (R_FAILED(ret))
	{
		consoleLog("\nFS not fully initialized!\n");
		consoleLog("Have you selected a title?\n\n");
		// state = STATE_ERROR;
	}

	ret = saveInit();
	if (R_FAILED(ret))
	{
		printf("\nSave module not initialized!\n");
		// state = BACKUP_ERROR;
	}

	ret = saveGetTitleId(&titleId);
	if (R_FAILED(ret))
	{
		printf("\nCouldn't get the title id!\n");
		// state = BACKUP_ERROR;
	}

	// fsEntry dir;
	// memset(dir.name, 0, FS_MAX_PATH_LENGTH);
	// strcpy(dir.name, "/pk/data/");
	// fsScanDir(&dir, &sdmcArchive, false);
	// fsFreeDir(&dir);

	fsDirInit();
	fsDirPrintSave();
	fsDirPrintSdmc();

	u32 kDown;
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		kDown = hidKeysDown();

		switch (state)
		{
			case STATE_BROWSE:
			{
				if (kDown & (KEY_LEFT | KEY_RIGHT))
				{
					fsDirSwitch(NULL);
					fsDirPrintSave();
					fsDirPrintSdmc();
				}

				if (kDown & KEY_ZL)
				{
					fsDirSwitch(&saveDir);
					fsDirPrintSave();
					fsDirPrintSdmc();
				}

				if (kDown & KEY_ZR)
				{
					fsDirSwitch(&sdmcDir);
					fsDirPrintSave();
					fsDirPrintSdmc();
				}

				if (kDown & KEY_UP)
				{
					fsDirMove(-1);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_DOWN)
				{
					fsDirMove(+1);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_A)
				{
					consoleLog("Opening -> %s\n", currentDir->entrySelected->name);
					fsDirGotoSubDir();
					fsDirPrintCurrent();
				}

				if (kDown & KEY_B)
				{
					fsDirGotoParentDir();
					fsDirPrintCurrent();
				}

				if (kDown & KEY_Y)
				{
					fsDirCopyCurrentFile();
					fsDirPrintDick();
				}

				break;
			}
			case STATE_ERROR:
			{
				consoleLog("\nAn error has occured...\n");
				consoleLog("Please check previous logs!\n");
				consoleLog("\nPress any key to exit.\n");
				state = STATE_EOF;
				break;
			}
			default: break;
		}

		if (kDown & KEY_START)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	FS_fsExit();
	{
		hidScanInput();
		if (!(hidKeysHeld() & KEY_L) && !(hidKeysHeld() & KEY_R))
		// if (hidKeysHeld() & )
		{
			FS_MediaType mediaType = 3;
			FSUSER_GetMediaType(&mediaType);
			Result ret = saveRemoveSecureValue(titleId, mediaType, NULL);
			if (R_FAILED(ret))
			{
				printf("\nSecure value not removed.\n");
				printf("It might already be unitialized.\n");
				printf("\n\nPress any key to exit.\n");
				waitKey(KEY_ANY);
			}
		}
	}
	gfxExit();
	return 0;
}
