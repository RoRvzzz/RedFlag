
#include <windows.h> 
#include <TlHelp32.h> 
#include <string> 
#include <iostream> 
#include <Psapi.h>
#include <atomic>
#include <unordered_map>
#include <chrono>
#include <windows.h>
#include <string>
#include <iostream>
#include <wininet.h>
#include <iostream>
#include <conio.h>
#include "../misc/Umodule/Umodule.hpp"
#include "overlay/skStr.h"
#include "overlay/auth.hpp"
#include "main.h"
#include "../misc/output_system/output/output.hpp"
#include "engine/Classes/offsets/offsets.hpp"
#include "../../src/rbx/cheat/features/features.h"
#include "overlay/overlay.hpp"
#include "engine/data/data.h"
#include "../misc/output_system/log_system/log_system.h"
#include "cheat/features/rage/thread.h"

#undef Log

bool RBX::Initializer() {
    utils::output::info("Initialization started.");

    utils::output::info("Finding Roblox process...");
    Umodule::process_id = Umodule::find_process(TEXT("RobloxPlayerBeta.exe"));
    if (!Umodule::process_id || Umodule::process_id == 0 || Umodule::process_id == -1) {
        utils::output::error("Roblox process not found!");
        return false;
    }

    utils::output::printint("Found Process ID: ", Umodule::process_id);

    utils::output::info("Opening process handle...");
    Umodule::hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, Umodule::process_id);
    if (!Umodule::hProcess || Umodule::hProcess == INVALID_HANDLE_VALUE) {
        utils::output::error("Failed to open process handle.");
        return false;
    }

    utils::output::info("Fetching base address...");
    virtualaddy = Umodule::GetProcessBase();
    if (!virtualaddy) {
        utils::output::error("Failed to retrieve base address.");
        return false;
    }
    utils::output::printint("Base Address: ", virtualaddy);

    utils::output::info("Acquiring global variables...");
    if (!GetGlobalVariables()) {
        utils::output::error("GetGlobalVariables failed.");
        return false;
    }

    utils::output::info("Validating global variables...");

    CheckIfGlobalVariablesInvalid();
    utils::output::error("Exception in CheckIfGlobalVariablesInvalid.");

    utils::output::info("Launching feature threads...");
    std::thread(RBX::InitializeAimbot).detach();
    utils::output::error("Failed to launch Aimbot thread.");

    std::thread(RBX::initsilent).detach();
    utils::output::error("Failed to launch Silent Aim thread.");

    std::thread(RBX::TriggerBotLoop).detach();
    utils::output::error("Failed to launch TriggerBot thread.");

    //   rage::hook();
    std::thread(RBX::Instance::updatePlayers).detach();

    std::thread([] {
        utils::output::info("Starting render...");
        overlay::render(); }).detach();
    utils::output::info("Render thread launched.");

    std::cout << "\x1b[35mMade with love from koda & nova & stackz\x1b[0m" << std::endl;

    ShowWindow(GetConsoleWindow(), SW_SHOW);


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return true;
}


std::string tm_to_readable_time(tm ctx) {
    char buffer[80];

    strftime(buffer, sizeof(buffer), "%a %m/%d/%y %H:%M:%S %Z", &ctx);

    return std::string(buffer);
}

static std::time_t string_to_timet(std::string timestamp) {
    auto cv = strtol(timestamp.c_str(), NULL, 10); // long

    return (time_t)cv;
}

static std::tm timet_to_tm(time_t timestamp) {
    std::tm context;

    localtime_s(&context, &timestamp);

    return context;
}
