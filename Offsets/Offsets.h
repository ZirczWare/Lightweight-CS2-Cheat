#pragma once

#include <string>

namespace Offsets
{
	bool Init();
	std::string GetError();

        namespace engine2_dll {
                inline std::ptrdiff_t dwBuildNumber = 0x60F594;
                inline std::ptrdiff_t dwNetworkGameClient = 0x90D4B0;
                inline std::ptrdiff_t dwNetworkGameClient_isBackgroundMap = 0x2C141F;
                inline std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
        }
}