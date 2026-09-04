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

static std::atomic<bool> ThreadsShouldRun{ false };
static std::thread FrequentUpdateThread;
static std::thread SlowUpdateThread;

static Entity::Data Enemies[Entity::MAX_ENEMIES];
static size_t EnemiesCount = 0;

static void FrequentUpdate()
{
	Vector3 Origins[Entity::MAX_ENEMIES];
	Vector3 Heads[Entity::MAX_ENEMIES];

	ImVec2 ScreenOrigins[Entity::MAX_ENEMIES];
	ImVec2 ScreenHeads[Entity::MAX_ENEMIES];

	Render::Data LocalRenderData;

	while (ThreadsShouldRun.load(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(7));

		View::Update();

		for (int i = 0; i < EnemiesCount; i++)
		{
			Memory::Read(Enemies[i].AbsOriginAddress, Origins[i]);

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

		LocalRenderData.Count = EnemiesCount;
			
		{
			std::lock_guard<std::mutex> lock(Cache::Mutex);
			Render::BackBuffer = LocalRenderData;
		}
	}
}

static void SlowUpdate()
{
	std::uint8_t ClientTeam = 0;
	Entity::Data LocalEnemies[Entity::MAX_ENEMIES];

	while (ThreadsShouldRun.load(std::memory_order_relaxed))
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));

		uintptr_t ClientPawn{};
		Memory::Read(Offsets::client_dll::dwLocalPlayerPawn, ClientPawn);
		if (!ClientPawn)
		{
			std::lock_guard<std::mutex> lock(Cache::Mutex);
			EnemiesCount = 0;
			continue;
		}

		uintptr_t EntityList{};
		Memory::Read(Offsets::client_dll::dwEntityList, EntityList);
		if (!EntityList)
			continue;

		size_t LocalEnemiesCount = 0;

		for (int i = 1; i < Entity::MAX_ENTITIES; i++) // First index is skippable
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

			LocalEnemies[LocalEnemiesCount].AbsOriginAddress = GameSceneNode + Offsets::CGameSceneNode::m_vecOrigin;
			LocalEnemiesCount++;
		}

		std::lock_guard<std::mutex> lock(Cache::Mutex);

		for (size_t i = 0; i < LocalEnemiesCount; i++)
			Enemies[i] = LocalEnemies[i];

		EnemiesCount = LocalEnemiesCount;
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