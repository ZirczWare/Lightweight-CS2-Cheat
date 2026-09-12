#include "Console.h"
#include <cstdio>
#include <iostream>
#include <string_view>
#include <windows.h>

void Console::Show()
{
	// Syncing streams lowers performance, we don't need it
	std::ios::sync_with_stdio(false);

	// No punishment if Show() is called multiple times
	FreeConsole();

	if (AllocConsole())
	{
		// Redirect output stream to console
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

		// Clear error bits if printed before Show()
		std::cout.clear();

		Console::Print("[ ! ] Console is now available\n");
	}
}

void Console::Detail::PrintInternal(std::string_view message)
{
	std::cout << message;
}