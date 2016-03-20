#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"
#include "fsdir.h"

#include "key.h"
#include "save.h"
#include "console.h"

#define HELD_TICK (16000000)
#define NO_HELD_TICK

typedef enum {
	STATE_START,		///< Start
	STATE_EOF,			///< End of file
	STATE_ERROR,		///< Error
	STATE_BROWSE,		///< Browse
	STATE_BACKUP,		///< Backup
} State;

static State state;
static u64 titleid;

void drawHelp(void)
{
	consoleSelect(&logConsole);
	consoleClear();

	printf(" Welcome in tvds, a tdvs clone.\n");
	printf("\n");

	switch (state)
	{
		case STATE_BROWSE:
		{
			printf("> [Up/Down] Select file/folder\n");
			printf("> [L/R] Swap between Save/Sdmc folder\n");
			printf("> [A] Navigate inside a folder\n");
			printf("> [B] Return to the parent folder\n");
			printf("> [X] Delete the current file/folder\n");
			printf("> [Y] Copy the current file/folder\n");
			break;
		}
		case STATE_BACKUP:
		{
			printf("> [Up/Down] Select backup\n");
			printf("> [A] Inject the selected backup back\n");
			printf("> [X] Delete the selected backup\n");
			printf("> [Y] Create a new backup\n");
			break;
		}
		default: break;
	}

	printf("> [Select] Print these instructions\n");
	printf("> [Start] Exit tvds\n");
	printf("\n");

	consoleSelectDefault();

	consoleSelect(&titleConsole);
	consoleClear();

	printf("Title id: 0x%016llx", titleid);

	consoleSelectDefault();
}

void drawBrowse(void)
{
	fsDirPrintSave();
	fsDirPrintSdmc();
	drawHelp();
}

void drawBackup(void)
{
	fsBackPrintSave();
	fsBackPrintBackup();
	drawHelp();
}

void switchState(State* state)
{
	switch (*state)
	{
		case STATE_START:
		case STATE_BACKUP: *state = STATE_BROWSE; drawBrowse(); break;
		case STATE_BROWSE: *state = STATE_BACKUP; drawBackup(); break;
		default: break;
	}
}

int main(void)
{
	gfxInitDefault();
	consoleInitDefault();

	Result ret;
	state = STATE_START;

	ret = FS_Init();
	if (R_FAILED(ret))
	{
		consoleLog("\nCouldn't initialize the FS module!\n");
		consoleLog("Have you selected a title?\n");
		consoleLog("Error code: 0x%lx\n", ret);
		// state = STATE_ERROR;
	}

	ret = saveInit();
	if (R_FAILED(ret))
	{
		consoleLog("\nCouldn't initialize the Save module!\n");
		consoleLog("Error code: 0x%lx\n", ret);
		// state = BACKUP_ERROR;
	}

	ret = saveGetTitleId(&titleid);
	if (R_FAILED(ret))
	{
		consoleLog("\nCouldn't get the title id of the game!\n");
		consoleLog("Error code: 0x%lx\n", ret);
		// state = BACKUP_ERROR;
	}

	fsDirInit();
	// fsBackInit(titleid);
	switchState(&state);

	drawHelp();

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
					heldUp = svcGetSystemTick() + HELD_TICK * 2;
				}
#ifndef NO_HELD_TICK
				else if (kHeld & KEY_UP)
				{
					if (heldUp + HELD_TICK < svcGetSystemTick())
					{
						fsDirMove(-1);
						fsDirPrintCurrent();
						heldUp = svcGetSystemTick();
					}
				}
#endif

				if (kDown & KEY_DOWN)
				{
					fsDirMove(+1);
					fsDirPrintCurrent();
					heldDown = svcGetSystemTick() + HELD_TICK * 2;
				}
#ifndef NO_HELD_TICK
				else if (kHeld & KEY_DOWN)
				{
					if (heldDown + HELD_TICK < svcGetSystemTick())
					{
						fsDirMove(+1);
						fsDirPrintCurrent();
						heldDown = svcGetSystemTick();
					}
				}
#endif

				if (kDown & KEY_A)
				{
					ret = fsDirGotoSubDir();
					consoleLog("   > fsDirGotoSubDir: %lx\n", ret);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_B)
				{
					ret = fsDirGotoParentDir();
					consoleLog("   > fsDirGotoParentDir: %lx\n", ret);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_X)
				{
					ret = fsDirDeleteCurrentEntry();
					consoleLog("   > fsDirDeleteCurrentEntry: %lx\n", ret);
					fsDirPrintCurrent();
				}

				if (kDown & KEY_Y)
				{
					ret = fsDirCopyCurrentEntry(false);
					consoleLog("   > fsDirCopyCurrentEntry: %lx\n", ret);
					fsDirPrintDick();
				}

				break;
			}
			case STATE_BACKUP:
			{
				if (kDown & KEY_A)
				{
					ret = fsBackImport();
					consoleLog("  > fsBackImport: %lx\n", ret);
					fsBackPrintSave();
				}

				if (kDown & KEY_X)
				{
					ret = fsBackDelete();
					consoleLog("  > fsBackDelete: %lx\n", ret);
					fsBackPrintBackup();
				}

				if (kDown & KEY_Y)
				{
					ret = fsBackExport();
					consoleLog("  > fsBackExport: %lx\n", ret);
					fsBackPrintBackup();
				}

				if (kDown & KEY_UP)
				{
					fsBackMove(-1);
					fsBackPrintBackup();
					heldUp = svcGetSystemTick() + HELD_TICK * 2;
				}
#ifndef NO_HELD_TICK
				else if (kHeld & KEY_UP)
				{
					if (heldUp + HELD_TICK < svcGetSystemTick())
					{
						fsBackMove(-1);
						fsBackPrintBackup();
						heldUp = svcGetSystemTick();
					}
				}
#endif

				if (kDown & KEY_DOWN)
				{
					fsBackMove(+1);
					fsBackPrintBackup();
					heldDown = svcGetSystemTick() + HELD_TICK * 2;
				}
#ifndef NO_HELD_TICK
				else if (kHeld & KEY_DOWN)
				{
					if (heldDown + HELD_TICK < svcGetSystemTick())
					{
						fsBackMove(+1);
						fsBackPrintBackup();
						heldDown = svcGetSystemTick();
					}
				}
#endif

				break;
			}
			case STATE_ERROR:
			{
				consoleLog("\nAn error has occured...\n");
				consoleLog("Please check previous logs!\n");
				consoleLog("\nPress start to exit.\n");
				state = STATE_EOF;
				break;
			}
			default: break;
		}

		{
			if (kDown & KEY_L)
			{
				// TODO: Prev
				// switchState(&state); // TODO: Uncomment
			}

			if (kDown & KEY_R)
			{
				// TODO: Next
				// switchState(&state); // TODO: Uncomment
			}

			if (kDown & KEY_SELECT)
			{
				drawHelp();
			}
		}

		if (kDown & KEY_START)
			break;

		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	fsDirExit();
	// fsBackExit();
	FS_Exit();
	{
		hidScanInput();
		if (!(hidKeysHeld() & KEY_L) && !(hidKeysHeld() & KEY_R))
		{
			u8 out = 0;
			FS_MediaType mediaType = 3;
			FSUSER_GetMediaType(&mediaType);
			Result ret = saveRemoveSecureValue(titleid, mediaType, &out);
			if (R_FAILED(ret))
			{
				consoleSelect(&consoleLog);
				printf("\nSecure value not removed.\n");
				printf("It might already be unitialized.\n");
				printf("Error code: 0x%lx (%i)\n", ret, out);
				printf("\n\nPress any key to exit.\n");
				waitKey(KEY_ANY);
			}
		}
	}
	gfxExit();
	return 0;
}
