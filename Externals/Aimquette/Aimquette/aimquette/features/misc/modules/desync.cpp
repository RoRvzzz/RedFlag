#include "../misc.h"
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <iostream>
#include <Psapi.h>
#include <thread>
#include <chrono>
#pragma comment(lib, "Psapi.lib")

using namespace roblox;

std::string findRobloxPath() {
    HANDLE hProcessSnap;
    PROCESSENTRY32W pe32;
    std::string exePath = "";

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
        return "";

    pe32.dwSize = sizeof(PROCESSENTRY32W);
    if (!Process32FirstW(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return "";
    }

    do {
        std::wstring wExe = pe32.szExeFile;
        std::string exeName;
        exeName.assign(wExe.begin(), wExe.end());

        if (_stricmp(exeName.c_str(), "RobloxPlayerBeta.exe") == 0) {
            HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProc) {
                char buffer[MAX_PATH];
                if (GetModuleFileNameExA(hProc, NULL, buffer, MAX_PATH)) {
                    exePath = buffer;
                    CloseHandle(hProc);
                    break;
                }
                CloseHandle(hProc);
            }
        }
    } while (Process32NextW(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return exePath;
}

void blockRoblox() {
    if (!globals::misc::roblox_path.empty()) {
        std::string cmdOut = "netsh advfirewall firewall add rule name=\"" + globals::misc::firewall_rule_name + "_OUT\" dir=out program=\"" + globals::misc::roblox_path + "\" action=block enable=yes";
        system(cmdOut.c_str());
        std::cout << "[+] Started The Goon\n";
    }
}

void unblockRoblox() {
    std::string cmdDel = "netsh advfirewall firewall delete rule name=\"" + globals::misc::firewall_rule_name + "_OUT\"";
    system(cmdDel.c_str());
    std::cout << "[-] Stopped Gooning\n";
}

void hooks::desync() {
    // Initialize Roblox path on first run
    if (globals::misc::roblox_path.empty()) {
        globals::misc::roblox_path = findRobloxPath();
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Update keybind
        globals::misc::desynckeybind.update();
        
        // Check if desync is enabled and keybind is pressed
        if (globals::misc::desync && globals::misc::desynckeybind.enabled) {
            if (!globals::misc::desync_active) {
                // Start desync
                blockRoblox();
                globals::misc::desync_active = true;
                globals::misc::desync_timer = 0.0f;
                
                // Capture activation position for visualizer
                if (globals::misc::desync_visualizer && is_valid_address(globals::instances::lp.hrp.address)) {
                    globals::misc::desync_activation_pos = globals::instances::lp.hrp.get_pos();
                }
            }
        } else {
            if (globals::misc::desync_active) {
                // Stop desync
                unblockRoblox();
                globals::misc::desync_active = false;
                globals::misc::desync_timer = 0.0f;
            }
        }
        
        // Update timer if desync is active
        if (globals::misc::desync_active) {
            globals::misc::desync_timer += 0.05f; // 50ms increments
            
            // Auto-stop after 15 seconds - disable desync completely
            if (globals::misc::desync_timer >= globals::misc::desync_max_time) {
                unblockRoblox();
                globals::misc::desync_active = false;
                globals::misc::desync_timer = 0.0f;
                globals::misc::desync = false; // Disable the desync feature completely
            }
        }
    }
}
