#include "Popup.h"
#include <Windows.h>
#include <string_view>

void Popup::Error(std::string_view message)
{
	MessageBoxA(nullptr, message.data(), "Error", MB_OK | MB_ICONERROR | MB_TOPMOST);
}