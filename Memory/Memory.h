#pragma once

#include <cstddef>
#include <cstdint>

namespace Memory
{
        namespace Detail
        {
                inline void* hProcess = nullptr;
                inline std::uint64_t ClientDLL = 0;
                inline std::uint64_t Engine2DLL = 0;

                void InternalRead(std::uintptr_t from, void* to, std::size_t size);
        }

        bool Attach();

        template <typename T>
        T Read(std::uintptr_t addr)
        {
                T out;
                Detail::InternalRead(addr, &out, sizeof(T));
                return out;
        }

        template <typename T>
        void Read(std::uintptr_t addr, T& out)
        {
                return Detail::InternalRead(addr, &out, sizeof(T));
        }

        inline std::uint64_t GetClientDLL() { return Detail::ClientDLL; }
        inline std::uint64_t GetEngine2DLL() { return Detail::Engine2DLL; }
}