#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "fsdir.h"

#include "key.h"
#include "console.h"

typedef enum {
	STATE_ERROR,		///< Error
	STATE_BROWSE_SAVE,	///< Browse the save  
	STATE_BROWSE_SDMC,	///< Browse the sdmc
} State;

static State state;

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInitDefault();

	Result ret;
	state = STATE_BROWSE_SDMC;

	ret = FS_fsInit();
	if (R_FAILED(ret))
	{
		consoleLog("\nFS not fully initialized!\n");
		consoleLog("Have you selected a title?\n\n");
		// state = STATE_ERROR;
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
			case STATE_BROWSE_SAVE:
			{
				if (kDown & (KEY_LEFT | KEY_RIGHT))
				{
					fsDirSwitch(&sdmcDir);
					fsDirPrintSave();
					fsDirPrintSdmc();
					state = STATE_BROWSE_SDMC;
				}
				break;
			}
			case STATE_BROWSE_SDMC:
			{
				if (kDown & (KEY_LEFT | KEY_RIGHT))
				{
					fsDirSwitch(&saveDir);
					fsDirPrintSdmc();
					fsDirPrintSave();
					state = STATE_BROWSE_SAVE;
				}
				break;
			}
			default: break;
		}

		switch (state)
		{
			case STATE_BROWSE_SAVE:
			case STATE_BROWSE_SDMC:
			{
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
					consoleLog("%s\n", currentDir->entrySelected->name);
					fsDirGotoSubDir();
					fsDirPrintCurrent();
				}

				if (kDown & KEY_B)
				{
					fsDirGotoParentDir();
					fsDirPrintCurrent();
				}
			}
			default: break;
		}

		if (kDown & KEY_START)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	FS_fsExit();
	gfxExit();
	return 0;
}
