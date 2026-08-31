#include <windows.h>
#include "Overlay/Overlay.h"
#include <sal.h>
#include "Memory/Memory.h"
#include "Console/Console.h"
#include "Popup/Popup.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) 
{
#ifdef _DEBUG
	Console::Show();
#endif

	if (not Memory::Attach())
	{
		Popup::Error("Couldn't attach memory reader to target");
		return 1;
	}

	Overlay::Run();

	return 0;
}