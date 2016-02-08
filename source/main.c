#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "fsdir.h"

#include "key.h"
#include "save.h"
#include "console.h"

#define HELD_TICK (16000000)

typedef enum {
	STATE_EOF,			///< End of file
	STATE_ERROR,		///< Error
	STATE_BROWSE,		///< Browse the dir
	STATE_OW_CONFIRM,	///< Overwrite confirmation
} State;

static State state;

int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInitDefault();

	Result ret;
	u64 titleId;
	state = STATE_BROWSE;

	ret = FS_Init();
	if (R_FAILED(ret))
	{
		consoleLog("\nCouldn't initialize the FS module!\n");
		consoleLog("Have you selected a title?\n");
		printf("Error code: 0x%lx\n", ret);
		// state = STATE_ERROR;
	}

	ret = saveInit();
	if (R_FAILED(ret))
	{
		printf("\nCouldn't initialize the Save module!\n");
		printf("Error code: 0x%lx\n", ret);
		// state = BACKUP_ERROR;
	}

	ret = saveGetTitleId(&titleId);
	if (R_FAILED(ret))
	{
		printf("\nCouldn't get the title id of the game!\n");
		printf("Error code: 0x%lx\n", ret);
		// state = BACKUP_ERROR;
	}

	fsDirInit();
	fsDirPrintSave();
	fsDirPrintSdmc();

	u64 heldUp = 0;
	u64 heldDown = 0;
	u32 kDown, kHeld;
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		kDown = hidKeysDown();
		kHeld = hidKeysHeld();

		switch (state)
		{
			case STATE_OW_CONFIRM:
			{
				if (kDown)
				{
					if (kDown & KEY_SELECT)
					{
						consoleLog("Overwriting file!\n");
						ret = fsDirCopyCurrentFileOverwrite();
						consoleLog("   > fsDirCopyCurrentFileOverwrite: %lx\n", ret);
						fsDirPrintDick();

						state = STATE_BROWSE;
						break;
					}
					else
					{
						consoleLog("Overwrite cancelled!\n");

						state = STATE_BROWSE;
					}
				}
				else
				{
					break;
				}
			}
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
					fsDirMove(kHeld & KEY_R ? -5 : -1);
					fsDirPrintCurrent();
					heldUp = svcGetSystemTick() + HELD_TICK * 2;
				}
				else if (kHeld & KEY_UP)
				{
					if (heldUp + HELD_TICK < svcGetSystemTick())
					{
						fsDirMove(kHeld & KEY_R ? -5 : -1);
						fsDirPrintCurrent();
						heldUp = svcGetSystemTick();
					}
				}

				if (kDown & KEY_DOWN)
				{
					fsDirMove(kHeld & KEY_R ? +5 : +1);
					fsDirPrintCurrent();
					heldDown = svcGetSystemTick() + HELD_TICK * 2;
				}
				else if (kHeld & KEY_DOWN)
				{
					if (heldDown + HELD_TICK < svcGetSystemTick())
					{
						fsDirMove(kHeld & KEY_R ? +5 : +1);
						fsDirPrintCurrent();
						heldDown = svcGetSystemTick();
					}
				}

				if (kDown & KEY_A)
				{
					ret = fsDirGotoSubDir();
					consoleLog("   > fsfDirGotoSubDir: %lx\n", ret);
					fsDirPrintCurrent();						
				}

				if (kDown & KEY_B)
				{
					ret = fsDirGotoParentDir();
					consoleLog("   > fsDirGotoParentDir: %lx\n", ret);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_Y)
				{
					ret = fsDirCopyCurrentFile();
					consoleLog("   > fsDirCopyCurrentFile: %lx\n", ret);
					if (ret == FS_OVERWRITE)
					{
						consoleLog("File already exist!\n");
						consoleLog("Press SELECT to overwrite it!\n");
						state = STATE_OW_CONFIRM;
					}
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

	fsDirExit();
	FS_Exit();
	{
		hidScanInput();
		if (!(hidKeysHeld() & KEY_L) && !(hidKeysHeld() & KEY_R))
		{
			FS_MediaType mediaType = 3;
			FSUSER_GetMediaType(&mediaType);
			Result ret = saveRemoveSecureValue(titleId, mediaType, NULL);
			if (R_FAILED(ret))
			{
				printf("\nSecure value not removed.\n");
				printf("It might already be unitialized.\n");
				printf("Error code: 0x%lx\n", ret);
				printf("\n\nPress any key to exit.\n");
				waitKey(KEY_ANY);
			}
		}
	}
	gfxExit();
	return 0;
}
