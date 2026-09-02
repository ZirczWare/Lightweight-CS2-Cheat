#include <windows.h>
#include "Overlay/Overlay.h"
#include <sal.h>
#include "Console/Console.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) 
{
#ifdef _DEBUG
	Console::Show();
#endif

	Overlay::Run();

	return 0;
}