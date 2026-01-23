
#include <windows.h> 
#include <TlHelp32.h> 
#include <string> 
#include <iostream> 
#include "plr.h"
#include <Psapi.h>
#include "debugger.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <thread>
#include <tlhelp32.h>
#include <psapi.h>
#include "../../main.h"
#include "../../../misc/output_system/output/output.hpp"
#include "../../../misc/Umodule/Umodule.hpp"
#include "../../../globals/globals.hpp"
#include "../../../misc/output_system/log_system/log_system.h"


bool RBX::GetGlobalVariables()
{
    utils::output::info("Initializing...");

    RBX::TaskScheduler Instance;
    RBX::Instance Instance2;

    Umodule::process_id = Umodule::find_process(TEXT("RobloxPlayerBeta.exe"));
    utils::output::printint("Found Process ID:", Umodule::process_id);

    virtualaddy = Umodule::GetProcessBase();
    utils::output::printint("Base Address:", virtualaddy);

    auto fake_datamodel = Umodule::read<std::uint64_t>(virtualaddy + Offsets::FakeDataModelPointer);
    globals::game = RBX::Instance(Umodule::read<std::uint64_t>(fake_datamodel + 0x1c0));
    globals::visualengine = RBX::Instance(Umodule::read<std::uint64_t>(virtualaddy + Offsets::VisualEnginePointer));
    globals::players = globals::game.FindFirstChild("Players");


    globals::lighting = Instance2.GetLighting();
    utils::output::printint("Lighting:", globals::lighting.address);

    globals::game_id = globals::game.GetGameId();
    utils::output::printint("Game ID:", globals::game_id);

    globals::localplayername = globals::players.GetLocalPlayer().GetName();
    utils::output::print("Local Player Name: " + globals::localplayername);

    globals::walkspeed_amount = globals::localplayer.humanoid.GetWalkSpeed();
    utils::output::printint("WalkSpeed:", globals::walkspeed_amount);

    globals::JumpPower = globals::localplayer.humanoid.GetJumpPower();
    utils::output::printint("JumpPower:", globals::JumpPower);

    globals::HipHeight = globals::localplayer.humanoid.GetHipHeight();
    utils::output::printint("HipHeight:", globals::HipHeight);

    test = globals::walkspeed_amount;
    test2 = globals::JumpPower;
    test3 = globals::HipHeight;

    utils::output::info("Tests Assigned. Calling plrdata()...");
    plrdata();
    utils::output::end("Completed.");

    return true;
}
bool RBX::CheckIfGlobalVariablesInvalid()
{
    utils::output::info("Validating all global variables...");

    if (!globals::game.address) {
        utils::output::warning("Game Invalid");
    }

    if (!globals::DataModelPTR.address) {
        utils::output::warning("DataModelPTR Invalid");
    }

    if (!globals::mouse_service) {
        utils::output::warning("Mouse Service Invalid");
    }

    if (!globals::players.address) {
        utils::output::warning("Players Invalid");
    }

    if (!globals::game_id) {
        utils::output::warning("Game ID Invalid");
    }

    if (!globals::lighting.address) {
        utils::output::warning("Lighting Invalid");
    }

    if (!globals::visualengine.address) {
        utils::output::warning("Visual Engine Invalid");
    }

    if (!globals::players.GetLocalPlayer().address) {
        utils::output::warning("LocalPlayer / Character Invalid");
    }

    int PartCount = 0;
    for (const auto& Part : globals::localplayer.character.GetChildren()) {
        if (!Part.address) {
            utils::output::warning("LocalPlayer Part Invalid");
            utils::output::info("Dumping Parts To Identify Invalid One...");
            utils::output::print("[Part Name]: " + Part.GetName());
        }
        PartCount++;
    }

    utils::output::printint("Total Valid Parts:", PartCount);
    if (PartCount < 16) {
        utils::output::error("Expected >= 16 Parts, Found Less. Check Character Integrity or Contact Support.");
    }

    utils::output::end("Check Complete.");
    return true;
}

bool RBX::checkUmodule()
{
    utils::output::info("[CheckUmodule] Verifying Umodule...");

    try {
        Umodule::process_id = Umodule::find_process(TEXT("RobloxPlayerBeta.exe"));

        if (!Umodule::process_id) {
            RBX::Log_System::Error("Umodule Proc ID Not Found, Restart PC");
            utils::output::error("Umodule Proc ID Not Found, Restart PC");
            return false;
        }

        utils::output::printint("[CheckUmodule] Process ID:", Umodule::process_id);
        RBX::Log_System::Info("Umodule Successfully Mapped.");
        return true;
    }
    catch (const std::exception& Ex) {
        std::string Msg = "Exception occurred in CheckUmodule: " + std::string(Ex.what());
        RBX::Log_System::Error(Msg);
        utils::output::error(Msg);
        return false;
    }
    catch (...) {
        RBX::Log_System::Error("Unknown exception occurred in CheckUmodule.");
        utils::output::error("Unknown exception occurred in CheckUmodule.");
        return false;
    }
}

void RBX::getrobloxId()
{
    utils::output::info("Getting Roblox Base Address...");

    try {
        virtualaddy = Umodule::GetProcessBase();

        if (virtualaddy == 0) {
            RBX::Log_System::Error("Failed to find image base address.");
            utils::output::error("Failed to find image base address.");
            return;
        }

        std::string LogMsg = "Base Address: " + std::to_string(virtualaddy);
        RBX::Log_System::Info(LogMsg);
        utils::output::print(LogMsg);
    }
    catch (const std::exception& Ex) {
        std::string Msg = "Exception occurred in GetRobloxId: " + std::string(Ex.what());
        RBX::Log_System::Error(Msg);
        utils::output::error(Msg);
    }
    catch (...) {
        RBX::Log_System::Error("Unknown exception occurred in GetRobloxId.");
        utils::output::error("Unknown exception occurred in GetRobloxId.");
    }
}

