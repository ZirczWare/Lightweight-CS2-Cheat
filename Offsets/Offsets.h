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
                inline std::ptrdiff_t dwLocalPlayerPawn = 0x23C6268;
        }

        namespace engine2_dll
        {
                inline std::ptrdiff_t dwBuildNumber = 0x60F594;
        }

        namespace CCSPlayerController
        {
                inline std::ptrdiff_t m_hPlayerPawn = 0x914;
                inline std::ptrdiff_t m_bPawnIsAlive = 0x91C;
        }

        namespace C_BaseEntity
        {
                inline std::ptrdiff_t m_pGameSceneNode = 0x330;
                inline std::ptrdiff_t m_iTeamNum = 0x3E7;
        }

        namespace CGameSceneNode
        {
                inline std::ptrdiff_t m_vecOrigin = 0x80;
        }
}