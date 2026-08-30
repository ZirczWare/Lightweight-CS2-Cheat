#include <windows.h>
#include "Overlay/Overlay.h"
#include <sal.h>
#include "Memory/Memory.h"
#include "Console/Console.h"
#include "Popup/Popup.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) 
{
	if (not Memory::Attach())
	{
		Popup::Error("Couldn't attach memory reader to CS2");
		return 1;
	}

	Overlay::Run();
}