#pragma once
/**
 * @file console.h
 * @brief Console Module
 */

#include <3ds/console.h>

typedef enum 
{
	BLACK, MAROON, GREEN, OLIVE, NAVY, PURPLE, TEAL, SILVER,	///< Normal (0-7)
	GRAY, RED, LIME, YELLOW, BLUE, FUCHSIA, CYAN, WHITE,		///< Bright (8-15)
} ConsoleColor;

extern PrintConsole statusConsole;	///< Header
extern PrintConsole saveConsole;	///< Save data
extern PrintConsole sdmcConsole;	///< Sdmc data
extern PrintConsole titleConsole;	///< Titleid
extern PrintConsole logConsole;		///< Log screen

/**
 * @brief Initializes the consoles.
 */
void consoleInitDefault(void);

/**
 * @brief Declares the use of a new console.
 * @param[in] console The new console to use.
 */
void consoleSelectNew(PrintConsole* console);

/**
 * @brief Selects the current console declared in consoleSelectNew.
 * @see consoleSelectNew(PrintConsole*)
 */
void consoleSelectDefault(void);

/**
 * @brief Selects the last console declared in consoleSelectNew.
 * @see consoleSelectNew(PrintConsole*)
 */
void consoleSelectLast(void);

/**
 * @brief Prints arguments to the log console, selects back the previous console.
 * @param[in] format The text to print.
 */
void consoleLog(const char* format, ...);

/**
 * @brief Reset the console colors.
 */
void consoleResetColor(void);

/**
 * @brief Changes the console foreground color.
 * @param color The color to use.
 */
void consoleForegroundColor(ConsoleColor color);

/**
 * @brief Changes the console background color.
 * @param color The color to use.
 */
void consoleBackgroundColor(ConsoleColor color);
