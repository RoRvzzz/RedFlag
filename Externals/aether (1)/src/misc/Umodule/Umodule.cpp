#include "Umodule.hpp"
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

uintptr_t virtualaddy;

INT32 Umodule::process_id;
HANDLE Umodule::hProcess;
int cool = 0;
int cool2 = 0;

namespace Umodule {

    std::string GetProcessName(DWORD processID) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return "Unknown";
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnapshot, &processEntry)) {
            do {
                if (processEntry.th32ProcessID == processID) {
                    CloseHandle(hSnapshot);
                    return std::string(processEntry.szExeFile);
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
        return "Unknown";
    }

    template <typename T>
    bool read_process_memory(uint64_t address, T* buffer, size_t size) {
        SIZE_T bytesRead;
        return ReadProcessMemory(Umodule::hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) != 0;
    }

    uintptr_t GetProcessBase() {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
        if (hProc == NULL) throw std::runtime_error("Unable to open process");
        HMODULE hModules[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProc, hModules, sizeof(hModules), &cbNeeded)) {
            uintptr_t baseAddress = reinterpret_cast<uintptr_t>(hModules[0]);
            CloseHandle(hProc);
            return baseAddress;
        }
        CloseHandle(hProc);
        throw std::runtime_error("Unable to enumerate process modules");
    }

    uintptr_t GetGuardedRegion() {
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;
        while (VirtualQueryEx(hProcess, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) != 0) {
            if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_GUARD)) {
                return reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            }
            address += mbi.RegionSize;
        }
        return 0;
    }

    INT32 find_process(LPCTSTR process_name) {
        PROCESSENTRY32 pt;
        HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        pt.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hsnap, &pt)) {
            do {
                if (!lstrcmpi(pt.szExeFile, process_name)) {
                    CloseHandle(hsnap);
                    process_id = pt.th32ProcessID;
                    return pt.th32ProcessID;
                }
            } while (Process32Next(hsnap, &pt));
        }
        CloseHandle(hsnap);
        return NULL;
    }

    uintptr_t GetModuleBase(const std::string& moduleName) {
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
                char szModName[MAX_PATH];
                if (GetModuleBaseNameA(hProcess, hMods[i], szModName, sizeof(szModName))) {
                    if (moduleName == szModName) {
                        return reinterpret_cast<uintptr_t>(hMods[i]);
                    }
                }
            }
        }
        return 0;
    }

    std::vector<HMODULE> ListModules() {
        std::vector<HMODULE> modules;
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            size_t count = cbNeeded / sizeof(HMODULE);
            modules.assign(hMods, hMods + count);
        }
        return modules;
    }

    std::string GetModuleName(HMODULE mod) {
        char szModName[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, mod, szModName, sizeof(szModName))) {
            return std::string(szModName);
        }
        return "";
    }

    bool WriteMemory(uintptr_t address, void* buffer, size_t size) {
        SIZE_T bytesWritten;
        return WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(address), buffer, size, &bytesWritten) != 0;
    }

    bool ReadMemory(uintptr_t address, void* buffer, size_t size) {
        SIZE_T bytesRead;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), buffer, size, &bytesRead) != 0;
    }

    uintptr_t PatternScan(uintptr_t base, size_t size, const char* pattern, const char* mask) {
        std::vector<char> buffer(size);
        SIZE_T bytesRead;
        if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(base), buffer.data(), size, &bytesRead)) {
            return 0;
        }

        for (size_t i = 0; i < size; ++i) {
            bool found = true;
            for (size_t j = 0; pattern[j] != '\0'; ++j) {
                if (mask[j] != '?' && pattern[j] != buffer[i + j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return base + i;
            }
        }
        return 0;
    }

    DWORD GetThreadCount(DWORD processID) {
        DWORD count = 0;
        HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hThreadSnap != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);
            if (Thread32First(hThreadSnap, &te32)) {
                do {
                    if (te32.th32OwnerProcessID == processID) count++;
                } while (Thread32Next(hThreadSnap, &te32));
            }
            CloseHandle(hThreadSnap);
        }
        return count;
    }

    bool IsProcessAlive(DWORD pid) {
        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
        DWORD code = 0;
        if (hProc != NULL) {
            if (GetExitCodeProcess(hProc, &code)) {
                CloseHandle(hProc);
                return code == STILL_ACTIVE;
            }
            CloseHandle(hProc);
        }
        return false;
    }

}
