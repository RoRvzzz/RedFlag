#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <tchar.h>
#include <intrin.h>
#include <iostream>
#include <thread>
#include <winternl.h>

class Protection
{
public:

    static void Initialize()
    {
        if (IsDebuggerDetected())
        {
            std::cout << "[Protection] Failed: IsDebuggerDetected()" << std::endl;
            TriggerCrash();
        }

        if (IsApiHooked())
        {
            std::cout << "[Protection] Failed: IsApiHooked()" << std::endl;
            TriggerCrash();
        }

        IsInlineHooked();
     

        if (!IsVirtualMachine())
        {
            std::cout << "[Protection] Failed: IsVirtualMachine()" << std::endl;
            TriggerCrash();
        }

        if (!IsTimingLegit())
        {
            std::cout << "[Protection] Failed: IsTimingLegit() == false" << std::endl;
            TriggerCrash();
        }

        if (HasHiddenThreads())
        {
            std::cout << "[Protection] Failed: HasHiddenThreads()" << std::endl;
            TriggerCrash();
        }

        if (IsPebBeingDebugged())
        {
            std::cout << "[Protection] Failed: IsPebBeingDebugged()" << std::endl;
            TriggerCrash();
        }

        if (HasHardwareBreakpointsSetOnAllThreads())
        {
            std::cout << "[Protection] Failed: HasHardwareBreakpointsSetOnAllThreads()" << std::endl;
            TriggerCrash();
        }
     
    }

private:

   
    static DWORD ComputeTextSectionCrc()
    {
        HMODULE Module = GetModuleHandle(nullptr);
        if (!Module) return 0;

        PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)Module;
        PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((BYTE*)Module + DosHeader->e_lfanew);
        PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(NtHeader);

        for (int i = 0; i < NtHeader->FileHeader.NumberOfSections; ++i, ++Section)
        {
            if (std::string((char*)Section->Name).rfind(".text", 0) == 0)
            {
                return ComputeCrc32((BYTE*)Module + Section->VirtualAddress, Section->Misc.VirtualSize);
            }
        }
        return 0;
    }

    static DWORD ComputeCrc32(const BYTE* Data, size_t Size)
    {
        DWORD Crc = 0xFFFFFFFF;
        for (size_t i = 0; i < Size; ++i)
        {
            BYTE Byte = Data[i];
            Crc ^= Byte;
            for (int j = 0; j < 8; ++j)
            {
                DWORD Mask = -(Crc & 1);
                Crc = (Crc >> 1) ^ (0xEDB88320 & Mask);
            }
        }
        return ~Crc;
    }

    // --- Crash Function ---
    static void TriggerCrash()
    {
        // Force crash by writing to null pointer
        *((volatile int*)nullptr) = 0xBADCAFE;
    }

    // --- Debugger Detection ---

    static bool IsDebuggerDetected()
    {
        return IsDebuggerPresent()
            || CheckRemoteDebuggerPresent2()
            || IsHardwareBreakpointSet()
            || HasDebuggerUsingNtQuery()
            || IsPebBeingDebugged();
    }

    static bool CheckRemoteDebuggerPresent2()
    {
        BOOL DebuggerPresent = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &DebuggerPresent);
        return DebuggerPresent != FALSE;
    }

    static bool IsHardwareBreakpointSet()
    {
        CONTEXT Context = {};
        Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        HANDLE Thread = GetCurrentThread();
        if (GetThreadContext(Thread, &Context))
        {
            return Context.Dr0 || Context.Dr1 || Context.Dr2 || Context.Dr3;
        }
        return false;
    }

    // Checks hardware breakpoints on all threads to detect debugger presence
    static bool HasHardwareBreakpointsSetOnAllThreads()
    {
        DWORD CurrentPid = GetCurrentProcessId();
        HANDLE Snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (Snap == INVALID_HANDLE_VALUE) return false;

        THREADENTRY32 Entry = {};
        Entry.dwSize = sizeof(THREADENTRY32);

        for (BOOL Has = Thread32First(Snap, &Entry); Has; Has = Thread32Next(Snap, &Entry))
        {
            if (Entry.th32OwnerProcessID == CurrentPid)
            {
                HANDLE Thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, Entry.th32ThreadID);
                if (Thread)
                {
                    CONTEXT Context = {};
                    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (GetThreadContext(Thread, &Context))
                    {
                        if (Context.Dr0 || Context.Dr1 || Context.Dr2 || Context.Dr3)
                        {
                            CloseHandle(Thread);
                            CloseHandle(Snap);
                            return true;
                        }
                    }
                    CloseHandle(Thread);
                }
            }
        }
        CloseHandle(Snap);
        return false;
    }

    // Detect debugger using NtQueryInformationProcess for DebugPort
    static bool HasDebuggerUsingNtQuery()
    {
        typedef NTSTATUS(NTAPI* NtQueryInformationProcessFunc)(
            HANDLE ProcessHandle,
            ULONG ProcessInformationClass,
            PVOID ProcessInformation,
            ULONG ProcessInformationLength,
            PULONG ReturnLength);

        static NtQueryInformationProcessFunc NtQueryInformationProcess = nullptr;
        if (!NtQueryInformationProcess)
        {
            NtQueryInformationProcess = (NtQueryInformationProcessFunc)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
            if (!NtQueryInformationProcess)
                return false; // Cannot detect
        }

        ULONG_PTR DebugPort = 0;
        NTSTATUS Status = NtQueryInformationProcess(GetCurrentProcess(), 7 /* ProcessDebugPort */, &DebugPort, sizeof(DebugPort), nullptr);
        return (Status == 0 && DebugPort != 0);
    }

    // Check PEB BeingDebugged flag directly
    static bool IsPebBeingDebugged()
    {
#ifdef _M_X64
        PPEB Peb = (PPEB)__readgsqword(0x60);
#else
        PPEB Peb = (PPEB)__readfsdword(0x30);
#endif
        return Peb && Peb->BeingDebugged != 0;
    }

    // --- Inline Hook Detection ---
    // Checks if first bytes of kernel32.dll APIs are altered (jmp or int3)
    static bool IsInlineHooked()
    {
        std::vector<const char*> ApiNames = {
            "IsDebuggerPresent",
            "CheckRemoteDebuggerPresent",
            "NtQueryInformationProcess"
        };

        HMODULE Kernel32 = GetModuleHandleA("kernel32.dll");
        if (!Kernel32) return true;

        for (const auto& ApiName : ApiNames)
        {
            FARPROC Address = GetProcAddress(Kernel32, ApiName);
            if (!Address) return true;

            BYTE* Bytes = (BYTE*)Address;

            // Check for JMP instruction
            if (Bytes[0] == 0xE9) // Relative JMP
            {
                DWORD RelativeOffset = *(DWORD*)(Bytes + 1);
                void* Target = Bytes + 5 + RelativeOffset;

                if ((uintptr_t)Target < (uintptr_t)Kernel32 || (uintptr_t)Target >(uintptr_t)Kernel32 + 0x100000)
                {
                    std::cout << "[InlineHookCheck] Suspicious JMP in " << ApiName << " to " << Target << std::endl;
                    return true;
                }
            }
            else if (Bytes[0] == 0xCC)
            {
                std::cout << "[InlineHookCheck] INT3 detected in " << ApiName << std::endl;
                return true;
            }
            else if (Bytes[0] == 0xC3)
            {
                std::cout << "[InlineHookCheck] RET at start of " << ApiName << std::endl;
                return true;
            }
            else if (Bytes[0] == 0xFF && Bytes[1] == 0x25)
            {
                void* Indirect = *(void**)(Bytes + 2);
                if ((uintptr_t)Indirect < (uintptr_t)Kernel32 || (uintptr_t)Indirect >(uintptr_t)Kernel32 + 0x100000)
                {
                    std::cout << "[InlineHookCheck] Indirect JMP in " << ApiName << " to " << Indirect << std::endl;
                    return true;
                }
            }
        }

        return false;
    }


    // --- API Hook Detection ---
    static bool IsApiHooked()
    {
        FARPROC Address = GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsDebuggerPresent");
        if (!Address) return true;

        BYTE* Bytes = (BYTE*)Address;
        return (Bytes[0] == 0xE9 || Bytes[0] == 0xCC || Bytes[0] == 0xC3);
    }

    // --- Timing Detection ---
    static bool IsTimingLegit()
    {
        LARGE_INTEGER Frequency, Start, End;
        if (!QueryPerformanceFrequency(&Frequency))
            return false;

        QueryPerformanceCounter(&Start);
        Sleep(10);
        QueryPerformanceCounter(&End);

        LONGLONG Elapsed = End.QuadPart - Start.QuadPart;
        // Expect at least ~10ms (adjust threshold if needed)
        return Elapsed > Frequency.QuadPart / 100;
    }

    // --- Virtual Machine Detection ---
    static bool IsVirtualMachine()
    {
        return IsHvBitSet() || CheckVmRegistryKeys() || CheckVmCpuIdSignatures();
    }

    static bool IsHvBitSet()
    {
        int CpuInfo[4] = {};
        __cpuid(CpuInfo, 1);
        return (CpuInfo[2] & (1 << 31)) != 0;
    }

    static bool CheckVmRegistryKeys()
    {
        HKEY Key;
        const std::vector<std::wstring> Paths = {
            L"HARDWARE\\ACPI\\DSDT\\VBOX__",
            L"SOFTWARE\\VMware, Inc.\\VMware Tools",
            L"HARDWARE\\DESCRIPTION\\System\\SystemBiosVersion",
            L"SOFTWARE\\Microsoft\\Virtual Machine",
            L"SYSTEM\\ControlSet001\\Services\\Disk\\Enum"
        };

        for (const auto& Path : Paths)
        {
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, Path.c_str(), 0, KEY_READ, &Key) == ERROR_SUCCESS)
            {
                RegCloseKey(Key);
                return true;
            }
        }
        return false;
    }

    static bool CheckVmCpuIdSignatures()
    {
        char Vendor[13] = { 0 };
        int CpuInfo[4] = {};
        __cpuid(CpuInfo, 0);
        memcpy(Vendor, &CpuInfo[1], 4);
        memcpy(Vendor + 4, &CpuInfo[3], 4);
        memcpy(Vendor + 8, &CpuInfo[2], 4);

        std::string VendorStr(Vendor);
        return (VendorStr.find("VBox") != std::string::npos
            || VendorStr.find("VMware") != std::string::npos
            || VendorStr.find("Xen") != std::string::npos
            || VendorStr.find("QEMU") != std::string::npos);
    }

    // --- Hidden Thread Detection ---
    static bool HasHiddenThreads()
    {
        DWORD CurrentPid = GetCurrentProcessId();
        HANDLE Snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (Snap == INVALID_HANDLE_VALUE) return false;

        THREADENTRY32 Entry = {};
        Entry.dwSize = sizeof(THREADENTRY32);

        for (BOOL Has = Thread32First(Snap, &Entry); Has; Has = Thread32Next(Snap, &Entry))
        {
            if (Entry.th32OwnerProcessID == CurrentPid && Entry.th32ThreadID != GetCurrentThreadId())
            {
                HANDLE Thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, Entry.th32ThreadID);
                if (Thread)
                {
                    DWORD ExitCode = 0;
                    if (!GetExitCodeThread(Thread, &ExitCode))
                    {
                        CloseHandle(Thread);
                        CloseHandle(Snap);
                        return true;
                    }
                    CloseHandle(Thread);
                }
            }
        }

        CloseHandle(Snap);
        return false;
    }


};
