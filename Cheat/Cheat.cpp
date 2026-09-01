#include "Cheat.h"
#include "../Console/Console.h"
#include "../Memory/Memory.h"
#include "../Offsets/Offsets.h"

void Cheat::Run()
{
        std::int64_t dwBuildNumber{};
        Memory::Read(Memory::GetEngine2DLL() + Offsets::engine2_dll::dwBuildNumber, dwBuildNumber);
        Console::Print("dwBuildNumber: {}\n", dwBuildNumber);
}