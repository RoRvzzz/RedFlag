#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <type_traits>

extern uintptr_t virtualaddy;

#define code_rw CTL_CODE(FILE_DEVICE_UNKNOWN, 0x71, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_ba CTL_CODE(FILE_DEVICE_UNKNOWN, 0x72, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_get_guarded_region CTL_CODE(FILE_DEVICE_UNKNOWN, 0x73, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define code_security 0x85b3e12

typedef struct _rw {
    INT32 security;
    INT32 process_id;
    ULONGLONG address;
    ULONGLONG buffer;
    ULONGLONG size;
    BOOLEAN write;
} rw, * prw;

typedef struct _ba {
    INT32 security;
    INT32 process_id;
    ULONGLONG* address;
} ba, * pba;

typedef struct _ga {
    INT32 security;
    ULONGLONG* address;
} ga, * pga;

namespace Umodule {

    extern INT32 process_id;
    extern HANDLE hProcess;

    uintptr_t GetProcessBase();

    template <typename T>
    bool read_process_memory(uint64_t address, T* buffer, size_t size);

    uintptr_t GetGuardedRegion();

    INT32 find_process(LPCTSTR process_name);

    std::string GetProcessName(DWORD processID);

    uintptr_t GetModuleBase(const std::string& moduleName);

    std::vector<HMODULE> ListModules();

    std::string GetModuleName(HMODULE mod);

    bool WriteMemory(uintptr_t address, void* buffer, size_t size);

    bool ReadMemory(uintptr_t address, void* buffer, size_t size);

    uintptr_t PatternScan(uintptr_t base, size_t size, const char* pattern, const char* mask);

    DWORD GetThreadCount(DWORD processID);

    bool IsProcessAlive(DWORD pid);

    template <typename T>
    T read(uint64_t address) {
        T buffer{};
        SIZE_T bytesRead = 0;
        ReadProcessMemory(Umodule::hProcess, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), &bytesRead);
        return buffer;
    }

    template <typename T>
    bool write(uint64_t address, const T& buffer) {
        SIZE_T bytesWritten = 0;
        WriteProcessMemory(Umodule::hProcess, reinterpret_cast<LPVOID>(address), &buffer, sizeof(T), &bytesWritten);
        return true;
    }
}

namespace Umodule {
    template<typename T>
    bool safe_read(std::uint64_t Address, T* Out, SIZE_T Size = sizeof(T)) {
        if (!hProcess || !Out || !Address || Size == 0 || Size > sizeof(T) * 8 || reinterpret_cast<void*>(Address) == nullptr) return false;
        __try {
            SIZE_T BytesRead = 0;
            if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(Address), Out, Size, &BytesRead)) return false;
            return BytesRead == Size;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
}
