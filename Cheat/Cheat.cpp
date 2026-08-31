#include "Cheat.h"
#include "../Console/Console.h"
#include "../Memory/Memory.h"
#include "../Offsets/Offsets.h"

void Cheat::Run()
{
        std::int64_t Buildnumber{};
        Memory::Read(Memory::GetEngine2DLL() + Offsets::engine2_dll::dwBuildNumber, Buildnumber);
        Console::Print("Buildnumber: {}\n", Buildnumber);
}