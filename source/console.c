#include "console.h"

#include <stdio.h>
#include <stdarg.h>

PrintConsole statusConsole;
PrintConsole saveConsole;
PrintConsole sdmcConsole;
PrintConsole titleConsole;
PrintConsole logConsole;

static PrintConsole* currentConsole;
static PrintConsole* lastConsole;

void consoleInitDefault(void)
{
	consoleInit(GFX_TOP, &statusConsole);
	consoleInit(GFX_TOP, &saveConsole);
	consoleInit(GFX_TOP, &sdmcConsole);
	consoleInit(GFX_TOP, &titleConsole);
	consoleInit(GFX_BOTTOM, &logConsole);

	consoleSetWindow(&statusConsole, 0, 0, 50,  4);
	consoleSetWindow(&saveConsole,   0, 4, 25, 25);
	consoleSetWindow(&sdmcConsole,  25, 4, 25, 25);
	consoleSetWindow(&titleConsole, 0, 29, 50,  1);

	currentConsole = &logConsole;
	lastConsole = &logConsole;

	consoleSelectDefault();
}

void consoleSelectNew(PrintConsole* console)
{
	if (!console) return;

	lastConsole = currentConsole;
	currentConsole = console;

	consoleSelect(console);
}

void consoleSelectDefault(void)
{
    consoleSelect(currentConsole);
}

void consoleSelectLast(void)
{
	consoleSelect(lastConsole);
}

void consoleLog(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    consoleSelect(&logConsole);
    vprintf(format, args);
    consoleSelectDefault();
    va_end(args);
}

void consoleResetColor(void)
{
	printf("\x1B[0m");
}

void consoleForegroundColor(ConsoleColor color)
{
	// If bright color
	if (color > SILVER)
	{
		printf("\x1B[3%d;1m", color % 8);
	}
	// Else normal color
	else
	{
		printf("\x1B[3%dm", color % 8);
	}
}

void consoleBackgroundColor(ConsoleColor color)
{
	// If bright color
	if (color > SILVER)
	{
		printf("\x1B[4%d;1m", color % 8);
	}
	// Else normal color
	else
	{
		printf("\x1B[4%dm", color % 8);
	}
}
