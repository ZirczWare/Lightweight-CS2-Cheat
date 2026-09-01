#include "Cheat.h"
#include "../Console/Console.h"
#include "../Memory/Memory.h"
#include "../Offsets/Offsets.h"
#include "../Math/Vector3.h"
#include <vector>
#include "../Math/View.h"
#include "../ImGui/imgui.h"

static Vector3 WorldPositions[64];
static ImVec2 ScreenPositions[64];
static bool PositionsVisibility[64];

void Cheat::Run()
{
	/*
	Read entity positions in cache thread
	Mass transform in worker thread, render here
	Filter out dead entities / enemies
	Also cache entities for faster reads
	*/

	uintptr_t TempEntityListAddress{};
	Memory::Read(Offsets::client_dll::dwEntityList, TempEntityListAddress);
	if (!TempEntityListAddress)
	{
		Console::Print("TempEntityListAddress nullptr_t\n");
		return;
	}

	size_t Count = 0;

	for (int i = 1; i < 64; i++)
	{
		uintptr_t ListEntry{};
		Memory::Read(TempEntityListAddress + (static_cast<unsigned long long>(8) * (i & 0x7FFF) >> 9) + 16, ListEntry);
		if (!ListEntry)
			continue;

		uintptr_t ControllerReadAddress = ListEntry + static_cast<unsigned long long>(112) * (i & 0x1FF);
		uintptr_t Controller{};
		Memory::Read(ControllerReadAddress, Controller);
		if (!Controller)
			continue;

		uint32_t PlayerPawn{};
		Memory::Read(Controller + Offsets::CCSPlayerController::m_hPlayerPawn, PlayerPawn);
		uintptr_t ListEntry_2{};
		Memory::Read(TempEntityListAddress + static_cast<unsigned long long>(0x8) * ((PlayerPawn & 0x7FFF) >> 9) + 16, ListEntry_2);
		if (!ListEntry_2)
			continue;

		uintptr_t PawnReadAddress = ListEntry_2 + static_cast<unsigned long long>(112) * (PlayerPawn & 0x1FF);
		uintptr_t pCSPlayerPawn{};
		Memory::Read(PawnReadAddress, pCSPlayerPawn);
		if (!pCSPlayerPawn)
			continue;

		uintptr_t GameSceneNode{};
		Memory::Read(pCSPlayerPawn + Offsets::C_BaseEntity::m_pGameSceneNode, GameSceneNode);
		if (!GameSceneNode)
			break;

		Memory::Read(GameSceneNode + Offsets::CGameSceneNode::m_vecOrigin, WorldPositions[Count]);
		Count++;
	}

	View::Update();
	View::WorldToScreenBulk(WorldPositions, ScreenPositions, PositionsVisibility, Count);

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	for (size_t i = 0; i < Count; i++)
		if (PositionsVisibility[i])
			drawList->AddText(ScreenPositions[i], IM_COL32(255, 0, 255, 255), "Enemy");
}