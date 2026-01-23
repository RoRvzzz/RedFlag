#include <Windows.h>
#include <string>
#include <iostream>
#include "lazy/lazy.h"
#include "base64.h"
#include "oxorany/oxorany_include.h"
#include "spoof.h"
#include "obfusheader.h"

bool hashed = false;

//using namespace KeyAuth;
using namespace base64;

BYTE Kernel32[32] = { 0 };
BYTE Kernelbase[32] = { 0 };
BYTE Ntdll[32] = { 0 };
BYTE User32[32] = { 0 };
BYTE Msvcrt[32] = { 0 };
BYTE Vcruntime140[32] = { 0 };
BYTE Ucrtbase[32] = { 0 };

BYTE Kernel32_Check[32] = { 0 };
BYTE Kernelbase_Check[32] = { 0 };
BYTE Ntdll_Check[32] = { 0 };
BYTE Msvcrt_Check[32] = { 0 };
BYTE Vcruntime140_Check[32] = { 0 };
BYTE Ucrtbase_Check[32] = { 0 };
 
typedef struct MY_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} MY_UNICODE_STRING, * MY_PUNICODE_STRING;

typedef struct MY_LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    MY_UNICODE_STRING FullDllName;
    MY_UNICODE_STRING BaseDllName;
    ULONG Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    ULONG TimeDateStamp;
} MY_LDR_DATA_TABLE_ENTRY, * MY_PLDR_DATA_TABLE_ENTRY;

typedef struct MY_PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
} MY_PEB_LDR_DATA, * MY_PPEB_LDR_DATA;

typedef struct MY_PEB
{
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN ImageUsesLargePages : 1;
            BOOLEAN IsProtectedProcess : 1;
            BOOLEAN IsImageDynamicallyRelocated : 1;
            BOOLEAN SkipPatchingUser32Forwarders : 1;
            BOOLEAN IsPackagedProcess : 1;
            BOOLEAN IsAppContainer : 1;
            BOOLEAN IsProtectedProcessLight : 1;
            BOOLEAN IsLongPathAwareProcess : 1;
        } s1;
    } u1;

    HANDLE Mutant;

    PVOID ImageBaseAddress;
    MY_PPEB_LDR_DATA Ldr;
    PVOID SubSystemData;
    PVOID ProcessHeap;
    PRTL_CRITICAL_SECTION FastPebLock;
    PVOID AtlThunkSListPtr;
    PVOID IFEOKey;
    union
    {
        ULONG CrossProcessFlags;
        struct
        {
            ULONG ProcessInJob : 1;
            ULONG ProcessInitializing : 1;
            ULONG ProcessUsingVEH : 1;
            ULONG ProcessUsingVCH : 1;
            ULONG ProcessUsingFTH : 1;
            ULONG ProcessPreviouslyThrottled : 1;
            ULONG ProcessCurrentlyThrottled : 1;
            ULONG ReservedBits0 : 25;
        } s2;
    } u2;
    union
    {
        PVOID KernelCallbackTable;
        PVOID UserSharedInfoPtr;
    } u3;
    ULONG SystemReserved[1];
    ULONG AtlThunkSListPtr32;
    PVOID ApiSetMap;
    ULONG TlsExpansionCounter;
    PVOID TlsBitmap;
    ULONG TlsBitmapBits[2];

    PVOID ReadOnlySharedMemoryBase;
    PVOID SharedData; // HotpatchInformation
    PVOID* ReadOnlyStaticServerData;

    PVOID AnsiCodePageData; // PCPTABLEINFO
    PVOID OemCodePageData; // PCPTABLEINFO
    PVOID UnicodeCaseTableData; // PNLSTABLEINFO

    ULONG NumberOfProcessors;
    ULONG NtGlobalFlag;

    LARGE_INTEGER CriticalSectionTimeout;
    SIZE_T HeapSegmentReserve;
    SIZE_T HeapSegmentCommit;
    SIZE_T HeapDeCommitTotalFreeThreshold;
    SIZE_T HeapDeCommitFreeBlockThreshold;

    ULONG NumberOfHeaps;
    ULONG MaximumNumberOfHeaps;
    PVOID* ProcessHeaps; // PHEAP

    PVOID GdiSharedHandleTable;
    PVOID ProcessStarterHelper;
    ULONG GdiDCAttributeList;

    PRTL_CRITICAL_SECTION LoaderLock;

    ULONG OSMajorVersion;
    ULONG OSMinorVersion;
    USHORT OSBuildNumber;
    USHORT OSCSDVersion;
    ULONG OSPlatformId;
    ULONG ImageSubsystem;
    ULONG ImageSubsystemMajorVersion;
    ULONG ImageSubsystemMinorVersion;
    ULONG_PTR ActiveProcessAffinityMask;

    PVOID TlsExpansionBitmap;
    ULONG TlsExpansionBitmapBits[32];

    ULONG SessionId;

    ULARGE_INTEGER AppCompatFlags;
    ULARGE_INTEGER AppCompatFlagsUser;
    PVOID pShimData;
    PVOID AppCompatInfo; // APPCOMPAT_EXE_DATA

    MY_UNICODE_STRING CSDVersion;

    PVOID ActivationContextData; // ACTIVATION_CONTEXT_DATA
    PVOID ProcessAssemblyStorageMap; // ASSEMBLY_STORAGE_MAP
    PVOID SystemDefaultActivationContextData; // ACTIVATION_CONTEXT_DATA
    PVOID SystemAssemblyStorageMap; // ASSEMBLY_STORAGE_MAP

    SIZE_T MinimumStackCommit;

    PVOID* FlsCallback;
    LIST_ENTRY FlsListHead;
    PVOID FlsBitmap;
    ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
    ULONG FlsHighIndex;

    PVOID WerRegistrationData;
    PVOID WerShipAssertPtr;
    PVOID pUnused; // pContextData
    PVOID pImageHeaderHash;
    union
    {
        ULONG TracingFlags;
        struct
        {
            ULONG HeapTracingEnabled : 1;
            ULONG CritSecTracingEnabled : 1;
            ULONG LibLoaderTracingEnabled : 1;
            ULONG SpareTracingBits : 29;
        } s3;
    } u4;
    ULONGLONG CsrServerReadOnlySharedMemoryBase;
    PVOID TppWorkerpListLock;
    LIST_ENTRY TppWorkerpList;
    PVOID WaitOnAddressHashTable[128];
    PVOID TelemetryCoverageHeader; // REDSTONE3
    ULONG CloudFileFlags;
} MY_PEB, * MY_PPEB;


#define Hide_Ptr() try { int* __INVALID_ALLOC__ = new int[(std::size_t)std::numeric_limits<std::size_t>::max]; } catch ( const std::bad_alloc& except ) { 
#define Hide_Ptr_End() } 

static bool is_hash_valid(const BYTE* hash, size_t length)
{
    for (size_t i = false; i < length; ++i)
    {
        if (hash[i] != Encrypt(0))
            return Encrypt(1);
    }
    return Encrypt(0);
}

std::string ToLowercase(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}
 

class nova_Imports
{
public:

    void InjectExitCode()
    {
        LPVOID pMemory = virtual_alloc(nullptr, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        if (pMemory)
        {
            unsigned char exitCode[] =
            {
                Encrypt(0xC7), Encrypt(0x44), Encrypt(0x24), Encrypt(0x04), Encrypt(0x01), Encrypt(0x00), Encrypt(0x00), Encrypt(0x00),
                Encrypt(0xC3),
            };

            import_memcpy(pMemory, exitCode, sizeof(exitCode));
            ((void(*)())pMemory)();
        }
    }


    HMODULE get_module_handleA(const char* moduleName)
    {
        Hide_Ptr();
        MY_PEB* peb = (MY_PEB*)(Encrypt(0x60));

        LIST_ENTRY* moduleList = &peb->Ldr->InMemoryOrderModuleList;
        LIST_ENTRY* currentEntry = moduleList->Flink;

        while (currentEntry != moduleList)
        {
            MY_LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(currentEntry, MY_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

            if (moduleName == NULL || moduleName == nullptr)
            {
                return (HMODULE)entry->DllBase;
            }

            char baseName[MAX_PATH];
            int result = WideCharToMultiByte(CP_ACP, 0, entry->BaseDllName.Buffer, entry->BaseDllName.Length / sizeof(WCHAR), baseName, sizeof(baseName) - 1, NULL, NULL);

            if (result > Encrypt(0))
            {
                baseName[result] = Encrypt('\0');

                std::string lowerBaseName = ToLowercase(baseName);
                std::string lowerModuleName = ToLowercase(moduleName);

                if (lowerBaseName == lowerModuleName)
                {
                    return (HMODULE)entry->DllBase;
                }
            }

            currentEntry = currentEntry->Flink;
        }
        return NULL;
        Hide_Ptr_End();
    }

    FARPROC get_proc_address(HMODULE hModule, const char* procName)
    {
        Hide_Ptr();
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
        PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
        IMAGE_DATA_DIRECTORY exportDirectoryData = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        PIMAGE_EXPORT_DIRECTORY exportDirectory = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportDirectoryData.VirtualAddress);
        DWORD* namesRVA = (DWORD*)((BYTE*)hModule + exportDirectory->AddressOfNames);
        DWORD* functionsRVA = (DWORD*)((BYTE*)hModule + exportDirectory->AddressOfFunctions);
        WORD* ordinals = (WORD*)((BYTE*)hModule + exportDirectory->AddressOfNameOrdinals);

        for (DWORD i = false; i < exportDirectory->NumberOfNames; ++i)
        {
            const char* currentFuncName = (const char*)((BYTE*)hModule + namesRVA[i]);
            if (import_strcmp(currentFuncName, procName) == Encrypt(0))
            {
                WORD ordinal = ordinals[i];
                DWORD functionRVA = functionsRVA[ordinal];
                FARPROC functionAddress = (FARPROC)((BYTE*)hModule + functionRVA);
                return functionAddress;
            }
        }

        return nullptr;
        Hide_Ptr_End();
    }


    __forceinline static int import_strcmp(const char* s1, const char* s2)
    {
        while (*s1 == *s2++)
            if (*s1++ == Encrypt(0))
                return Encrypt(0);
        return (*(unsigned char*)s1 -
            *(unsigned char*) --s2);
    }

    __forceinline static int import_memcmp(const void* ptr1, const void* ptr2, size_t num)
    {
        const unsigned char* s1 = (const unsigned char*)ptr1;
        const unsigned char* s2 = (const unsigned char*)ptr2;

        while (num--)
        {
            if (*s1 != *s2)
                return (*s1 - *s2);
            s1++;
            s2++;
        }

        return Encrypt(0);
    }

    __forceinline static void* import_memcpy(void* dest, const void* src, size_t num)
    {
        unsigned char* d = (unsigned char*)dest;
        const unsigned char* s = (const unsigned char*)src;

        while (num--)
        {
            *d++ = *s++;
        }

        return dest;
    }

    _forceinline static void* import_memset(void* ptr, int value, size_t num)
    {
        unsigned char* dest = (unsigned char*)ptr;
        while (num--)
        {
            *dest++ = (unsigned char)value;
        }
        return ptr;
    }


    __forceinline void exit_application()
    {
        //std::string name = Encrypt("Q29kZVZ1YWx0");
        //std::string ownerid = Encrypt("QU5CdVU2TlRGRA");
        ////std::string secret = Encrypt("MzczYTM2ZTdjODBkZTMzNGQ3MGZkZjZmZDlkOGFiODcwMmNkNWIzM2FhMzU3Njk5YzEzZTdlMGU2ZDQxYjBlOA");
        //std::string ver = Encrypt("MS4w");
        //std::string authapi = Encrypt("aHR0cHM6Ly9rZXlhdXRoLndpbi9hcGkvMS4zLw");
        //api Auth(decode(name).c_str(), decode(ownerid).c_str(), /*decode(secret).c_str(),*/ decode(ver).c_str(), decode(authapi).c_str(), Encrypt(""));

        //if (hashed)
        //{
        //    Auth.init();
        //}



        InjectExitCode();
        get_function<void(*)(UINT)>(Encrypt("ntdll.dll"), Encrypt("RtlExitUserProcess"))(Encrypt(0x99c));  __debugbreak();
        get_function<void(*)(HANDLE, PVOID, SIZE_T, DWORD)>(Encrypt("ntdll.dll"), Encrypt("NtUnmapViewOfSection"))((HANDLE)-1, get_module_handleA(nullptr), 0, 0);
        __fastfail(Encrypt(0x90c)); terminate();
        for (long long int i = Encrypt(0x96c); ++i; (&i)[i] = i);
        *((char*)NULL) = Encrypt(0x96c);
        *(uintptr_t*)(Encrypt(0)) = Encrypt(0x96c);
        Lazy(exit).get()(Encrypt(0x96c));
    }

    HMODULE load_library(const std::string& library_name)
    {
        Hide_Ptr();
        HMODULE hModule = Lazy(LoadLibraryA).get()(library_name.c_str());
        return hModule;
        Hide_Ptr_End();
    }

    template <typename Func>
    Func get_function(const std::string& library_name, const std::string& function_name)
    {
        Hide_Ptr();
        HMODULE hModule = load_library(library_name);
        FARPROC procAddress = get_proc_address(hModule, function_name.c_str());
        return reinterpret_cast<Func>(procAddress);
        Hide_Ptr_End();
    }


    FARPROC GetOriginalFunctionAddress(const std::string& moduleName, const std::string& functionName)
    {
        Hide_Ptr();
        HMODULE hModule = load_library(moduleName.c_str());
        FARPROC funcAddr = get_proc_address(hModule, functionName.c_str());

        return funcAddr;
        Hide_Ptr_End();
    }

 

    PIMAGE_NT_HEADERS GetNtHeaders(BYTE* imageBase)
    {
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)imageBase;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;
        PIMAGE_NT_HEADERS nth = (PIMAGE_NT_HEADERS)(imageBase + dos->e_lfanew);
        if (nth->Signature != IMAGE_NT_SIGNATURE) return nullptr;
        return nth;
    }

    bool LoadFileToMemory(const std::string& path, std::vector<BYTE>& buffer)
    {
        HANDLE hFile = Lazy(CreateFileA).get()(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, Encrypt(0), NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;
        DWORD fileSize = Lazy(GetFileSize).get()(hFile, NULL);
        if (fileSize == INVALID_FILE_SIZE || fileSize == Encrypt(0))
        {
            Lazy(CloseHandle).get()(hFile);
            return false;
        }
        buffer.resize(fileSize);
        DWORD bytesRead = Encrypt(0);;
        if (!Lazy(ReadFile).get()(hFile, buffer.data(), fileSize, &bytesRead, NULL) || bytesRead != fileSize)
        {
            Lazy(CloseHandle).get()(hFile);
            return false;
        }
        Lazy(CloseHandle).get()(hFile);
        return true;
    }

 

    bool CheckFunctionMemoryProtection(void* funcAddress)
    {
        MEMORY_BASIC_INFORMATION mbi = { 0 };
        if (virtual_query(funcAddress, &mbi, sizeof(mbi)) == Encrypt(0))
            return false;

        if (mbi.Protect == PAGE_EXECUTE_READWRITE || mbi.Protect == PAGE_EXECUTE_WRITECOPY)
        {
            return false;
        }
        return true;
    }


    bool IsFunctionHooked(const char* moduleName, const char* functionName)
    {
        auto moduleHandle = get_module_handleA(moduleName);
        if (!moduleHandle) return false;
        auto functionAddress = reinterpret_cast<uint8_t*>(get_proc_address(moduleHandle, functionName));
        if (!functionAddress) return false;
        return (functionAddress[0] == Encrypt(0xE9) || functionAddress[0] == Encrypt(0xE8) || functionAddress[0] == Encrypt(0xCC) || functionAddress[0] == Encrypt(0x90));
    }
 

    __forceinline HANDLE get_current_process()
    {
        return (HANDLE)Encrypt(-1);
    }

    __forceinline HMODULE get_module_handleW(const std::wstring& module_name)
    {
        return Lazy(GetModuleHandleW).get()(module_name.c_str());
    }

    __forceinline DWORD virtual_protect(LPVOID address, SIZE_T size, DWORD newProtect, DWORD* oldProtect)
    {
        return get_function<BOOL(*)(LPVOID, SIZE_T, DWORD, PDWORD)>(Encrypt("kernel32.dll"), Encrypt("VirtualProtect"))(address, size, newProtect, oldProtect);
    }

    __forceinline DWORD get_current_thread_id()
    {
        return get_function<DWORD(*)()>(Encrypt("kernel32.dll"), Encrypt("GetCurrentThreadId"))();
    }

    __forceinline DWORD set_thread_execution_state(DWORD esFlags)
    {
        return get_function<DWORD(*)(DWORD)>(Encrypt("kernel32.dll"), Encrypt("SetThreadExecutionState"))(esFlags);
    }

    __forceinline HANDLE get_current_thread()
    {
        return (HANDLE)Encrypt(-2);
    }

    __forceinline ULONGLONG get_tick_count64()
    {
        return get_function<ULONGLONG(*)()>(Encrypt("kernel32.dll"), Encrypt("GetTickCount64"))();
    }

    __forceinline LPVOID virtual_alloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
    {
        return get_function<LPVOID(*)(LPVOID, SIZE_T, DWORD, DWORD)>(Encrypt("kernel32.dll"), Encrypt("VirtualAlloc"))(lpAddress, dwSize, flAllocationType, flProtect);
    }

    __forceinline SIZE_T virtual_query(LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)
    {
        return get_function<SIZE_T(*)(LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T)>(Encrypt("kernel32.dll"), Encrypt("VirtualQuery"))(lpAddress, lpBuffer, dwLength);
    }

    __forceinline DWORD get_current_process_id()
    {
        return get_function<DWORD(*)()>(Encrypt("kernel32.dll"), Encrypt("GetCurrentProcessId"))();
    }

    __forceinline HANDLE open_process(DWORD desired_access, BOOL inherit_handle, DWORD process_id)
    {
        return get_function<HANDLE(*)(DWORD, BOOL, DWORD)>(Encrypt("kernel32.dll"), Encrypt("OpenProcess"))(desired_access, inherit_handle, process_id);
    }

    __forceinline HANDLE get_std_handle(DWORD std_handle)
    {
        return get_function<HANDLE(*)(DWORD)>(Encrypt("kernel32.dll"), Encrypt("GetStdHandle"))(std_handle);
    }

    __forceinline bool get_console_mode(HANDLE console_handle, LPDWORD mode)
    {
        return get_function<BOOL(*)(HANDLE, LPDWORD)>(Encrypt("kernel32.dll"), Encrypt("GetConsoleMode"))(console_handle, mode) != Encrypt(0);
    }

    __forceinline bool set_console_mode(HANDLE console_handle, DWORD mode)
    {
        return get_function<BOOL(*)(HANDLE, DWORD)>(Encrypt("kernel32.dll"), Encrypt("SetConsoleMode"))(console_handle, mode) != Encrypt(0);
    }

    __forceinline bool read_console_a(HANDLE console_handle, LPVOID buffer, DWORD chars_to_read, LPDWORD chars_read)
    {
        return get_function<BOOL(*)(HANDLE, LPVOID, DWORD, LPDWORD, LPVOID)>(Encrypt("kernel32.dll"), Encrypt("ReadConsoleA"))(console_handle, buffer, chars_to_read, chars_read, nullptr) != Encrypt(0);
    }


}; nova_Imports Imports;
