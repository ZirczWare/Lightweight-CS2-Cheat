#include "Memory.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <wchar.h>

struct DLL
{
        std::uint64_t* Address;
        const wchar_t* Name;
};

static bool GetModuleAddresses(DWORD dwPid, std::vector<DLL>& Modules)
{
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid);
        if (hSnapshot == INVALID_HANDLE_VALUE)
                return false;

        MODULEENTRY32W moduleEntry{ sizeof(MODULEENTRY32W) };
        if (not Module32FirstW(hSnapshot, &moduleEntry))
                return false;

        size_t FoundModules = 0;
        size_t TotalModules = Modules.size();

        do
        {
                auto it = std::find_if(Modules.begin(), Modules.end(), [&](const DLL& dll) {
                        return _wcsicmp(dll.Name, moduleEntry.szModule) == 0;
                });
                if (it != Modules.end())
                {
                        *it->Address = reinterpret_cast<std::uint64_t>(moduleEntry.modBaseAddr);

                        if (++FoundModules == TotalModules)
                                break;
                }

        } while (Module32NextW(hSnapshot, &moduleEntry));

        CloseHandle(hSnapshot);

        return FoundModules == TotalModules;
}

static std::uint32_t GetProcessID(const std::wstring& processName)
{
        PROCESSENTRY32 processInfo{};
        processInfo.dwSize = sizeof(processInfo);

        HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (processesSnapshot == INVALID_HANDLE_VALUE)
                return 0;

        Process32First(processesSnapshot, &processInfo);
        if (!processName.compare(processInfo.szExeFile))
        {
                CloseHandle(processesSnapshot);
                return processInfo.th32ProcessID;
        }

        while (Process32Next(processesSnapshot, &processInfo))
                if (!processName.compare(processInfo.szExeFile))
                {
                        CloseHandle(processesSnapshot);
                        return processInfo.th32ProcessID;
                }

        CloseHandle(processesSnapshot);

        return 0;
}

bool Memory::Attach()
{
        std::uint32_t ProcessID = GetProcessID(L"cs2.exe");
        if (ProcessID == NULL)
                return false;

        Detail::hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD, TRUE, ProcessID);
        if (Detail::hProcess == NULL)
                return false;

        std::vector<DLL> Modules;
        Modules.reserve(2);
        Modules.emplace_back(DLL{ &Detail::ClientDLL, L"client.dll" });
        Modules.emplace_back(DLL{ &Detail::Engine2DLL, L"engine2.dll" });

	return GetModuleAddresses(ProcessID, Modules);
}

void Memory::Detail::InternalRead(std::uintptr_t from, void* to, std::size_t size)
{
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(from), reinterpret_cast<LPVOID>(to), size, nullptr);
}