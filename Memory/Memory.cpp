#include "Memory.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string>

static DWORD_PTR GetModuleBaseAddress(DWORD dwPid, const wchar_t* moduleName)
{
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid);
        if (hSnapshot == INVALID_HANDLE_VALUE)
                return 0;

        MODULEENTRY32W moduleEntry{ sizeof(MODULEENTRY32W) };
        if (not Module32FirstW(hSnapshot, &moduleEntry))
                return 0;

        DWORD_PTR baseAddr = 0;

        do
        {
                if (_wcsicmp(moduleEntry.szModule, moduleName) != 0)
                        continue;

                baseAddr = reinterpret_cast<DWORD_PTR>(moduleEntry.modBaseAddr);
                break;
        } while (Module32NextW(hSnapshot, &moduleEntry));

        CloseHandle(hSnapshot);
        return baseAddr;
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

	Detail::ClientDLL = GetModuleBaseAddress(ProcessID, L"client.dll");
        Detail::Engine2DLL = GetModuleBaseAddress(ProcessID, L"engine2.dll");

	return (Detail::ClientDLL != 0 && Detail::Engine2DLL != 0);
}

void Memory::Detail::InternalRead(std::uintptr_t from, void* to, std::size_t size)
{
        ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(from), reinterpret_cast<LPVOID>(to), size, nullptr);
}