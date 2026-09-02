#include "Cache.h"
#include <atomic>
#include <thread>
#include <mutex>
#include "../Render/Render.h"
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include "../Offsets/Offsets.h"
#include "../Console/Console.h"
#include "../Math/View.h"
#include "../Memory/Memory.h"
#include "../Math/Vector3.h"
#include "../Entity/Entity.h"

constexpr int MAX_ENTITIES = 64;

static std::atomic<bool> ThreadsShouldRun{ false };
static std::thread FrequentUpdateThread;
static std::thread SlowUpdateThread;

static Entity::Data Entities[MAX_ENTITIES];
static size_t EntitiesCount = 0;

static void FrequentUpdate()
{
	Vector3 Origins[MAX_ENTITIES];
	Vector3 Heads[MAX_ENTITIES];

	ImVec2 ScreenOrigins[MAX_ENTITIES];
	ImVec2 ScreenHeads[MAX_ENTITIES];

	Render::Data LocalRenderData;

	while (ThreadsShouldRun.load(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(7));

		View::Update();

		for (int i = 0; i < EntitiesCount; i++)
		{
			Memory::Read(Entities[i].AbsOriginAddress, Origins[i]);

			LocalRenderData.VisibleOnScreen[i] = View::WorldToScreen(Origins[i], ScreenOrigins[i]);
			if (!LocalRenderData.VisibleOnScreen[i])
				continue;

			Heads[i] = { Origins[i].x, Origins[i].y, Origins[i].z + 72.f };
			View::WorldToScreen(Heads[i], ScreenHeads[i]);

			float Width = ScreenOrigins[i].Distance(ScreenHeads[i]) * 0.45f * 0.5f;

			LocalRenderData.Boxes[i] =
			{
				static_cast<float>(static_cast<int>(ScreenHeads[i].x - Width)),
				static_cast<float>(static_cast<int>(ScreenHeads[i].y)),
				static_cast<float>(static_cast<int>(ScreenOrigins[i].x + Width)),
				static_cast<float>(static_cast<int>(ScreenOrigins[i].y))
			};
		}

		LocalRenderData.Count = EntitiesCount;
			
		{
			std::lock_guard<std::mutex> lock(Cache::Mutex);
			Render::BackBuffer = LocalRenderData;
		}
	}
}

static void SlowUpdate()
{
	std::uint8_t ClientTeam = 0;
	Entity::Data LocalEntities[MAX_ENTITIES];

	while (ThreadsShouldRun.load(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		uintptr_t EntityList{};
		Memory::Read(Offsets::client_dll::dwEntityList, EntityList);
		if (!EntityList)
			continue;

		uintptr_t ClientPawn{};
		Memory::Read(Offsets::client_dll::dwLocalPlayerPawn, ClientPawn);
		if (!ClientPawn)
			continue;

		size_t LocalEntitiesCount = 0;

		for (int i = 1; i < MAX_ENTITIES; i++) // First index is skippable
		{
			uintptr_t ListEntry{};
			Memory::Read(EntityList + (static_cast<unsigned long long>(8) * (i & 0x7FFF) >> 9) + 16, ListEntry);
			if (!ListEntry)
				continue;

			uintptr_t Controller{};
			Memory::Read(ListEntry + static_cast<unsigned long long>(112) * (i & 0x1FF), Controller);
			if (!Controller)
				continue;

			uint32_t PawnHandle{};
			Memory::Read(Controller + Offsets::CCSPlayerController::m_hPlayerPawn, PawnHandle);
			if (!PawnHandle)
				continue;

			uintptr_t PawnEntry{};
			Memory::Read(EntityList + static_cast<unsigned long long>(8) * ((PawnHandle & 0x7FFF) >> 9) + 16, PawnEntry);
			if (!PawnEntry)
				continue;

			uintptr_t Pawn{};
			Memory::Read(PawnEntry + static_cast<unsigned long long>(112) * (PawnHandle & 0x1FF), Pawn);
			if (!Pawn)
				continue;

			std::uint8_t Team{};
			Memory::Read(Controller + Offsets::C_BaseEntity::m_iTeamNum, Team);

			if (Pawn == ClientPawn)
			{
				ClientTeam = Team;
				continue;
			}

			if (ClientTeam == Team)
				continue;

			bool Alive{};
			Memory::Read(Controller + Offsets::CCSPlayerController::m_bPawnIsAlive, Alive);
			if (!Alive)
				continue;

			uintptr_t GameSceneNode{};
			Memory::Read(Pawn + Offsets::C_BaseEntity::m_pGameSceneNode, GameSceneNode);
			if (!GameSceneNode)
				break;

			LocalEntities[LocalEntitiesCount].AbsOriginAddress = GameSceneNode + Offsets::CGameSceneNode::m_vecOrigin;
			LocalEntitiesCount++;
		}

		{
			std::lock_guard<std::mutex> lock(Cache::Mutex);

			for (size_t i = 0; i < LocalEntitiesCount; i++)
				Entities[i] = LocalEntities[i];

			EntitiesCount = LocalEntitiesCount;
		}
	}
}

void Cache::Init()
{
	ThreadsShouldRun.store(true, std::memory_order_relaxed);
	FrequentUpdateThread = std::thread(FrequentUpdate);
	SlowUpdateThread = std::thread(SlowUpdate);
}

void Cache::Shutdown()
{
	ThreadsShouldRun.store(false, std::memory_order_relaxed);

	if (FrequentUpdateThread.joinable())
		FrequentUpdateThread.join();

	if (SlowUpdateThread.joinable())
		SlowUpdateThread.join();
}