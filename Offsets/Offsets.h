#pragma once

#include <string>

namespace Offsets
{
	bool Init();
	std::string GetError();

        namespace client_dll
        {
                inline std::ptrdiff_t dwEntityList = 0x2571220;
                inline std::ptrdiff_t dwViewMatrix = 0x23CB830;
        }

        namespace engine2_dll
        {
                inline std::ptrdiff_t dwBuildNumber = 0x60F594;
                inline std::ptrdiff_t dwNetworkGameClient = 0x90D4B0;
                inline std::ptrdiff_t dwNetworkGameClient_isBackgroundMap = 0x2C141F;
                inline std::ptrdiff_t dwNetworkGameClient_signOnState = 0x230;
        }

        namespace CCSPlayerController
        {
                inline std::ptrdiff_t m_hPlayerPawn = 0x914;
        }

        namespace C_BaseEntity
        {
                inline std::ptrdiff_t m_pGameSceneNode = 0x330;
        }

        namespace CGameSceneNode
        {
                inline std::ptrdiff_t m_vecOrigin = 0x80;
        }
}