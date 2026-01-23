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
#define Hide_Function() try { int* __INVALID_ALLOC__ = new int[(std::size_t)std::numeric_limits<std::size_t>::max]; } catch (const std::bad_alloc&) {
#define Hide_Function_End() }


//#include "integ.h"    
#include <Psapi.h>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>

#include "oxorany/oxorany_include.h"
#include <string>
#include "lazy/lazy.h"
#include "skStr.h"
#include <conio.h>
#include <ctype.h>
#include <string.h>
#include "Imports.h"
volatile std::uint64_t set = 0;
bool tls_ran = false;
bool flag = false;
bool active = false;
bool authorized = false;
bool thread1_started = false;
bool thread_starter_called = false;
bool gettick = false;
bool var_is_valid = false;
bool main_called = false;
bool Protected_Thread_Started = false;
bool ban_system = false;
bool imgui = true;
int times;
int time1 = 0;
int start = 0;
int end = 0;
int called = 0;
int start2 = 0;
#define ThreadIsCritical  0x1
#define ThreadIsCritical2  0x11


HWND Imgui_Window = NULL;

DWORD Protected_ThreadID = 0;

HANDLE Thread1 = 0;

DWORD64 InitialRip = 0;
enum HIDE_COMMAND
{
    HidePid, //Hide a process
    UnhidePid, //Unhide a process
    UnhideAll //Unhide everything
};
typedef struct _HIDE_INFO
{
    HIDE_COMMAND Command;
    ULONG Type;
    HANDLE Pid;
}HIDE_INFO, * PHIDE_INFO;
void HandleAttack(const std::string& detection_reason, bool ban) {
    Imports.exit_application();
}


DWORD GetModuleSize(HMODULE hModule)
{
    if (hModule == NULL)
    {
        return Encrypt(0);
    }

    MODULEINFO moduleInfo;
    if (!GetModuleInformation(Imports.get_current_process(), hModule, &moduleInfo, sizeof(moduleInfo)))
    {
        Encrypt(0);
    }

    DWORD moduleSize = moduleInfo.SizeOfImage;
    return moduleSize;
}



void CheckDebugInfoIntegrity()
{
    Hide_Function();

    {
        auto addy = __readgsqword(Encrypt(0x60));
        auto beingDebugged = reinterpret_cast<std::uint8_t*>(addy + Encrypt(0x2));
        auto ntGlobalFlag = reinterpret_cast<std::uint32_t*>(addy + Encrypt(0xBC));
        auto processHeap = *reinterpret_cast<std::uint64_t*>(addy + Encrypt(0x30));
        auto heapFlags = reinterpret_cast<std::uint32_t*>(processHeap + Encrypt(0x70));

        if (*ntGlobalFlag != Encrypt(0x99))
        {
            flag = Encrypt(1); HandleAttack(Encrypt("Peb Modification #1"), Encrypt(1));
        }

        if (*beingDebugged != Encrypt(0x99))
        {
            flag = Encrypt(1); HandleAttack(Encrypt("Peb Modification #2"), Encrypt(1));
        }

        if (*heapFlags != Encrypt(0x99))
        {
            flag = Encrypt(1); HandleAttack(Encrypt("Peb Modification #3"), Encrypt(1));
        }
    }
    Hide_Function_End();
}
bool driverdetect()
{
    const TCHAR* devices[] =
    {
        (Encrypt(_T("\\\\.\\kdstinker"))),
        (Encrypt(_T("\\\\.\\NiGgEr"))),
        (Encrypt(_T("\\\\.\\KsDumper"))),
        (Encrypt(_T("\\\\.\\HyperHideDrv")))
    };

    WORD iLength = sizeof(devices) / sizeof(devices[0]);
    for (int i = 0; i < iLength; i++)
    {
        HANDLE hFile = Lazy(CreateFileA).get()((LPCSTR)devices[i], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            return Encrypt(1);
        }
    }

    return Encrypt(0);
}
__forceinline auto CheckDebugger(bool thread)
{
    if (thread)
    {
        if (flag == Encrypt(1))
        {
            __fastfail(Encrypt(0x99));
            *((volatile int*)nullptr) = Encrypt(0x99);
        }


        if (driverdetect())
        {
            flag = Encrypt(1);
            HandleAttack(Encrypt("Hyperhide #2"), Encrypt(0));
        }


        CheckDebugInfoIntegrity();
    }
}
void NTAPI __stdcall TempTls(PVOID pHandle, DWORD dwReason, PVOID Reserved)
{
    Hide_Function();
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        Encrypt(1); CheckDebugger(1);
    }

    if (dwReason == DLL_THREAD_ATTACH)
    {

    }

    if (dwReason == DLL_THREAD_DETACH)
    {

    }

    if (dwReason == DLL_PROCESS_DETACH)
    {

    }
    Hide_Function_End();
}


std::string Generate_Exe_Name(size_t length)
{
    const std::string charset = Encrypt("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> dist(Encrypt(0), charset.size() - Encrypt(1));

    std::string randomStr;
    for (size_t i = false; i < length; ++i)
    {
        randomStr += charset[dist(generator)];
    }
    return randomStr + Encrypt(".exe");
}
void xxd()
{
    WCHAR wcPath[MAX_PATH + 1];
    WCHAR wcCopyPath[MAX_PATH + 1];
    ZeroMemory(wcPath, sizeof(wcPath));
    ZeroMemory(wcCopyPath, sizeof(wcCopyPath));

    if (!Lazy(GetModuleFileNameW).forwarded_safe_cached()(NULL, wcPath, MAX_PATH))
    {
        return;
    }

    wcscpy_s(wcCopyPath, MAX_PATH, wcPath);
    WCHAR* lastSlash = wcsrchr(wcCopyPath, Encrypt(L'\\'));
    if (!lastSlash)
    {
        return;
    }

    std::string randomName = Generate_Exe_Name(Encrypt(16));
    int size = MultiByteToWideChar(CP_UTF8, 0, randomName.c_str(), -1, NULL, 0);
    if (size <= 0)
    {
        return;
    }

    wchar_t* wRandomName = new wchar_t[size];
    if (MultiByteToWideChar(CP_UTF8, 0, randomName.c_str(), -1, wRandomName, size) <= 0)
    {
        delete[] wRandomName;
        return;
    }

    wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash - wcCopyPath), wRandomName);
    delete[] wRandomName;

    if (!Lazy(CopyFileW).forwarded_safe_cached()(wcPath, wcCopyPath, FALSE))
    {
        return;
    }

    HANDLE hCurrent = Lazy(CreateFileW).forwarded_safe_cached()(wcPath, DELETE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hCurrent == INVALID_HANDLE_VALUE)
    {
        return;
    }


    FILE_RENAME_INFO* fRename;
    LPWSTR lpwStream = const_cast<LPWSTR>(Encrypt(L":layuuh"));
    DWORD bslpwStream = (DWORD)(wcslen(lpwStream)) * sizeof(WCHAR);
    DWORD bsfRename = sizeof(FILE_RENAME_INFO) + bslpwStream;
    fRename = (FILE_RENAME_INFO*)malloc(bsfRename);
    if (fRename == nullptr)
    {
        Lazy(CloseHandle).forwarded_safe_cached()(hCurrent);
        return;
    }

    ZeroMemory(fRename, bsfRename);
    fRename->FileNameLength = bslpwStream;
    import_memcpy(fRename->FileName, lpwStream, bslpwStream);
    if (!SetFileInformationByHandle(hCurrent, FileRenameInfo, fRename, bsfRename))
    {
        free(fRename);
        Lazy(CloseHandle).forwarded_safe_cached()(hCurrent);
        return;
    }

    free(fRename);
    Lazy(CloseHandle).forwarded_safe_cached()(hCurrent);
    hCurrent = Lazy(CreateFileW).forwarded_safe_cached()(wcPath, DELETE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hCurrent == INVALID_HANDLE_VALUE)
    {
        return;
    }

    FILE_DISPOSITION_INFO fDelete;
    ZeroMemory(&fDelete, sizeof(fDelete));
    fDelete.DeleteFile = TRUE;
    if (!SetFileInformationByHandle(hCurrent, FileDispositionInfo, &fDelete, sizeof(fDelete)))
    {
        Lazy(CloseHandle).forwarded_safe_cached()(hCurrent);
        return;
    }

    Lazy(CloseHandle).forwarded_safe_cached()(hCurrent);
}

void PreventUnload()
{
    Hide_Function();
    HMODULE thisModule = NULL; MEMORY_BASIC_INFORMATION mbi = { 0 }; DWORD oldProtect;
    Imports.get_function<BOOL(*)(DWORD, LPCSTR, HMODULE*)>(Encrypt("kernel32.dll"), Encrypt("GetModuleHandleExA"))(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCSTR>((&TempTls)), &thisModule);
    Imports.virtual_query(reinterpret_cast<void*>((&TempTls)), &mbi, sizeof(mbi)); Imports.virtual_protect(mbi.BaseAddress, mbi.RegionSize, PAGE_GUARD, &oldProtect);
    Hide_Function_End();
}


BOOL CALLBACK gxd_module(HWND hwnd, LPARAM lParam)
{
    //for (int i = Encrypt(0); i < Encrypt(200); ++i) { int3bs(); }
    std::vector<std::string> windowTitles = { Encrypt("hacker"), Encrypt("Resource Monitor"), Encrypt(") Properties"), Encrypt("dexz"), Encrypt("re-kit"), Encrypt("byte2mov"), Encrypt("Cheat Engine"), Encrypt("Command Prompt"), Encrypt("pssuspend"), Encrypt("sysinternals.com") };
    std::vector<std::string> windowClassNames = { Encrypt("ProcessHacker"), Encrypt("Qt5QWindowIcon"), Encrypt("WindowsForms10.Window.8.app.0.378734a"), Encrypt("MainWindowClassName"), Encrypt("BrocessRacker") };

    DWORD processId;
    char wndTitle[256];
    char wndClassName[256];
    Lazy(GetWindowThreadProcessId).get()(hwnd, &processId);
    DWORD id = Imports.get_current_process_id();
    Lazy(GetWindowTextA).get()(hwnd, wndTitle, sizeof(wndTitle));
    Lazy(GetClassNameA).get()(hwnd, wndClassName, sizeof(wndClassName));

    std::string windowTitle = wndTitle;
    std::string windowClassName = wndClassName;

    for (const auto& title : windowTitles) {
        if (windowTitle.find(title) != std::string::npos)
        {
            if (windowTitle == Encrypt("re-kit") || windowTitle == Encrypt("byte2mov"))
            {
                flag = Encrypt(1); HandleAttack(Encrypt("Api hooking"), Encrypt(1));
            }
            Lazy(SendMessageA).get()(hwnd, (WM_CLOSE), Encrypt(0), Encrypt(0)); // dont lazy will crash
            // minik buraya ss akma ekle
            return Encrypt(1);
        }
    }

    for (const auto& className : windowClassNames)
    {
        if (windowClassName.find(className) != std::string::npos)
        {
            if (processId != id)
            {
                Lazy(SendMessageA).get()(hwnd, (WM_CLOSE), Encrypt(0), Encrypt(0)); // dont lazy will crash
                return Encrypt(1);
            }
        }
    }

    return Encrypt(1);
}
BYTE multiByte[] = { 0xCD, 0x90, 0xCD, 0x03, 0xCD, 0x90 };
#define PAGE_SIZE2 0x1000

auto disable_titan_hide()
{
    DWORD written = NULL;
    HIDE_INFO titan_hide;
    HANDLE driver_handle = nullptr;
    driver_handle = Lazy(CreateFileA).get()(Encrypt("\\\\.\\TitanHide"), GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL, NULL);
    if (driver_handle == INVALID_HANDLE_VALUE) { return; }
    titan_hide.Command = UnhideAll;
    titan_hide.Pid = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(Imports.get_current_process_id()));
    titan_hide.Type = Encrypt(0x3FF);
    Lazy(WriteFile).get()(driver_handle, &titan_hide, sizeof(titan_hide), &written, nullptr);
    Lazy(CloseHandle).get()(driver_handle);
}
__forceinline void Wait(int time)
{
    std::mutex mtx;
    std::unique_lock<std::mutex> lock(mtx);
    std::condition_variable cv;
    cv.wait_for(lock, std::chrono::milliseconds(time));
}
DWORD WINAPI nova_切都将变得毫无乐趣和更多变得爱得毫无变得毫变得得(LPVOID lpParam)
{
    Hide_Function();
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    return Encrypt(0x99c);;
    Hide_Function_End();
}


DWORD WINAPI nova_这个该死的文本切都将变得毫无乐趣和更多的爱(LPVOID lpParam)
{
    Thread1 = Imports.get_current_thread();
    DWORD oldProtect;
    time1 = Imports.get_tick_count64();
    thread1_started = true;
    for (;;)
    {
        time1 = Imports.get_tick_count64();
        CheckDebugger(1);
        Wait(Encrypt(2000));
    }
    return Encrypt(0x99c);;
}

DWORD WINAPI nova_乐趣和更多的爱(LPVOID lpParam)
{
    Hide_Function();
    for (;;)
    {
        int time35 = Imports.get_tick_count64();
        if (time1 == Encrypt(0))
        {
            CheckDebugger(0);
            Wait(Encrypt(2000));
        }
        else
        {
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
            Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        }
    }
    return Encrypt(0x99c);
    Hide_Function_End();
}

DWORD WINAPI 乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱(LPVOID lpParam)
{
    Hide_Function();
    for (;;)
    {
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    }
    return Encrypt(0x99c);
    Hide_Function_End();
}
std::vector<DWORD> Protected_Threads;



void ChangeThreadStartAddress(HANDLE hThread, LPVOID newStartAddress)
{
    CONTEXT context;
    context.ContextFlags = CONTEXT_FULL;
    DWORD suspendCount = Lazy(SuspendThread).get()(hThread);
    Lazy(GetThreadContext).get()(hThread, &context);
    context.Rip = reinterpret_cast<ULONG_PTR>(newStartAddress);
    InitialRip = reinterpret_cast<ULONG_PTR>(newStartAddress);
    Lazy(SetThreadContext).get()(hThread, &context);
    Lazy(ResumeThread).get()(hThread);
}

auto 的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更()
{
    for (;;)
    {
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
        Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000)); Wait(Encrypt(200000000000000));
    }
}

__forceinline auto Secure_Create_Thread(void* function, HANDLE Base)
{
    Base = Imports.get_function<HANDLE(*)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)>(Encrypt("kernel32.dll"), Encrypt("CreateThread"))(nullptr, Encrypt(0), reinterpret_cast<LPTHREAD_START_ROUTINE>(function), nullptr, Encrypt(0), nullptr);
    Protected_Threads.push_back(Lazy(GetThreadId).get()(Base));

}

__forceinline auto Initialize_Threads()
{

    HANDLE e1 = 0;
    HANDLE e2 = 0;
    srand(static_cast<unsigned>(time(nullptr)));
    for (int i = Encrypt(0); i < Encrypt(3); ++i)
    {
        Secure_Create_Thread(&的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更, e1);
    }
    Secure_Create_Thread(&nova_这个该死的文本切都将变得毫无乐趣和更多的爱, Thread1);
    for (int i = Encrypt(0); i < Encrypt(3); ++i)
    {
        Secure_Create_Thread(&的爱乐趣和更多的爱乐趣和更多的爱乐趣和更多的爱乐趣和更, e2);
    }
    Wait(Encrypt(1000));


}

extern "C" __declspec(noinline) void HookFunction()
{
    Hide_Function();
    HANDLE Thread2 = CreateThread(NULL, Encrypt(0), Encrypt(&nova_乐趣和更多的爱), NULL, Encrypt(0), NULL);
    Hide_Function_End();
}
auto unloadntdll()
{
    HANDLE process = Imports.get_current_process();
    MODULEINFO mi = {};
    HMODULE ntdllModule = Imports.get_module_handleA(Encrypt("ntdll.dll"));

    GetModuleInformation(process, ntdllModule, &mi, sizeof(mi));
    LPVOID ntdllBase = (LPVOID)mi.lpBaseOfDll;
    HANDLE ntdllFile = Lazy(CreateFileA).get()(Encrypt("c:\\windows\\system32\\ntdll.dll"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, Encrypt(0), NULL);
    HANDLE ntdllMapping = CreateFileMapping(ntdllFile, NULL, PAGE_READONLY | SEC_IMAGE, Encrypt(0), Encrypt(0), NULL);
    LPVOID ntdllMappingAddress = Lazy(MapViewOfFile).get()(ntdllMapping, FILE_MAP_READ, Encrypt(0), Encrypt(0), Encrypt(0));

    PIMAGE_DOS_HEADER hookedDosHeader = (PIMAGE_DOS_HEADER)ntdllBase;
    PIMAGE_NT_HEADERS hookedNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)ntdllBase + hookedDosHeader->e_lfanew);

    for (WORD i = Encrypt(0); i < hookedNtHeader->FileHeader.NumberOfSections; i++) {
        PIMAGE_SECTION_HEADER hookedSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD_PTR)IMAGE_FIRST_SECTION(hookedNtHeader) + ((DWORD_PTR)IMAGE_SIZEOF_SECTION_HEADER * i));

        if (Imports.import_strcmp((char*)hookedSectionHeader->Name, (char*)Encrypt(".text")))
        {
            DWORD oldProtection = Encrypt(0);
            bool isProtected = Imports.virtual_protect((LPVOID)((DWORD_PTR)ntdllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, PAGE_EXECUTE_READWRITE, &oldProtection);
            Imports.import_memcpy((LPVOID)((DWORD_PTR)ntdllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), (LPVOID)((DWORD_PTR)ntdllMappingAddress + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize);
            isProtected = Imports.virtual_protect((LPVOID)((DWORD_PTR)ntdllBase + (DWORD_PTR)hookedSectionHeader->VirtualAddress), hookedSectionHeader->Misc.VirtualSize, oldProtection, &oldProtection);
        }
    }

    Lazy(CloseHandle).get()(process);
    Lazy(CloseHandle).get()(ntdllFile);
    Lazy(CloseHandle).get()(ntdllMapping);
    Lazy(FreeLibrary).get()(ntdllModule);
}
bool ChangeNumberOfSections(std::string module, DWORD newSectionsCount)
{
    PIMAGE_SECTION_HEADER sectionHeader = 0;
    HINSTANCE hInst = NULL;
    PIMAGE_DOS_HEADER pDoH = 0;
    PIMAGE_NT_HEADERS64 pNtH = 0;

    hInst = Imports.get_module_handleA(module.c_str());

    pDoH = (PIMAGE_DOS_HEADER)(hInst);

    if (pDoH == NULL || hInst == NULL)
    {
        return Encrypt(0);
    }

    pNtH = (PIMAGE_NT_HEADERS64)((PIMAGE_NT_HEADERS64)((PBYTE)hInst + (DWORD)pDoH->e_lfanew));
    sectionHeader = IMAGE_FIRST_SECTION(pNtH);

    DWORD dwOldProt = Encrypt(0);

    if (!Imports.virtual_protect((LPVOID)&pNtH->FileHeader.NumberOfSections, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &dwOldProt))
    {
        return Encrypt(0);
    }

    Imports.import_memcpy((void*)&pNtH->FileHeader.NumberOfSections, (void*)&newSectionsCount, sizeof(DWORD));

    if (!Imports.virtual_protect((LPVOID)&pNtH->FileHeader.NumberOfSections, sizeof(DWORD), dwOldProt, &dwOldProt))
    {
        return Encrypt(0);
    }

    return Encrypt(1);
}


void Execute_Bytes(PBYTE codeBytes)
{
    __try
    {
        ((void(*)())codeBytes)();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {

    }
}


auto int3bs()
{
    PBYTE Memory = (PBYTE)Imports.virtual_alloc(NULL, (PAGE_SIZE2 * Encrypt(2)), (MEM_COMMIT | MEM_RESERVE), PAGE_EXECUTE_READWRITE);
    if (Memory)
    {
        if (sizeof(multiByte) > PAGE_SIZE2)
        {
            Lazy(VirtualFree).get()(Memory, Encrypt(0), MEM_RELEASE);
        }
        else
        {
            PBYTE locationMultiByte = &Memory[PAGE_SIZE2 - sizeof(multiByte)];
            PBYTE locationPageTwo = &Memory[PAGE_SIZE2];
            Imports.import_memcpy(locationMultiByte, multiByte, sizeof(multiByte));
            PSAPI_WORKING_SET_EX_INFORMATION wsi;
            wsi.VirtualAddress = locationPageTwo;
            Imports.get_function<BOOL(*)(HANDLE, PVOID, DWORD)>(Encrypt("kernel32.dll"), Encrypt("K32QueryWorkingSetEx"))(((HANDLE)Encrypt(-1)), &wsi, sizeof(wsi));
            Obfuscate(Execute_Bytes)(locationMultiByte);
            Imports.get_function<BOOL(*)(HANDLE, PVOID, DWORD)>(Encrypt("kernel32.dll"), Encrypt("K32QueryWorkingSetEx"))(((HANDLE)Encrypt(-1)), &wsi, sizeof(wsi));

            if (Imports.import_memcmp(locationMultiByte, multiByte, sizeof(multiByte)) != Encrypt(0))
            {
                flag = Encrypt(1); HandleAttack(Encrypt("Triggering Trap Object #1"), Encrypt(1));
            }
            Lazy(VirtualFree).get()(Memory, Encrypt(0), MEM_RELEASE);
        }
    }
}



DWORD WINAPI threadxd() {
    Hide_Function();
    while (true) {
        Imports.get_function<BOOL(*)(WNDENUMPROC, LPARAM)>(Encrypt("user32.dll"), Encrypt("EnumWindows"))(gxd_module, Encrypt(0));

        Sleep(1000);
    }
    return Encrypt(0x99c);
    Hide_Function_End();
}
