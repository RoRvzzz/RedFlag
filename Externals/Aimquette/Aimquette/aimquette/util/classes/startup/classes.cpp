#include "../classes.h"
#include <Windows.h>
#include <TlHelp32.h>
#include "../../../util/driver/driver.h"
#include <vector>
#include <sstream>
#include <mutex>
#include "../../../util/offsets.h"
#include "../../protection/hider.h"
#include "C:\Users\ADMIN\Downloads\Aimquette MAIN\aimquette\protect/skStr.h"
#include <thread>
#include "../../../drawing/overlay/overlay.h"
#include <chrono>
#include "../../globals.h"
#include "../../../features/hook.h"

inline class Logger {
public:
    static void GetTime(char(&timeBuffer)[9])
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        struct tm localTime;
        localtime_s(&localTime, &now_time);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &localTime);
    }

    static void Log(const std::string& message) { (void)message; }

    static void CustomLog(int color, const std::string& start, const std::string& message) { (void)color; (void)start; (void)message; }

    static void Logf(const char* format, ...) { (void)format; }

    static void LogfCustom(int color, const char* start, const char* format, ...) { (void)color; (void)start; (void)format; }

    static void Banner() { }

    static void Separator() { }

    static void Section(const std::string& title) { (void)title; }

    static void Success(const std::string& message) { (void)message; }

}logger;

roblox::instance ReadVisualEngine() {
    return read<roblox::instance>(base_address + offsets::VisualEnginePointer);
}

roblox::instance ReadDatamodel() {
    uintptr_t padding = read<uintptr_t>(ReadVisualEngine().address + offsets::VisualEngineToDataModel1);
    return read<roblox::instance>(padding + offsets::VisualEngineToDataModel2);
}

roblox::instance ReadDatamodel(uintptr_t baseAddress) {
    uintptr_t padding = read<uintptr_t>(ReadVisualEngine().address + offsets::VisualEngineToDataModel1);
    return read<roblox::instance>(padding + offsets::VisualEngineToDataModel2);
}

std::vector<roblox::player> GetBots(uintptr_t baseAddress) {
    std::vector<roblox::player> bots;

    if (!globals::misc::npc_check) {
        return bots;
    }

    try {
        if (!is_valid_address(globals::instances::workspace.address)) {
            return bots;
        }

        roblox::instance bots_folder = globals::instances::workspace.findfirstchild("Bots");
        if (!bots_folder.is_valid()) {
            return bots;
        }

        auto children = bots_folder.get_children();

        for (const auto& child : children) {
            if (!child.is_valid()) continue;

            try {
                auto model_children = child.get_children();
                bool has_body_parts = false;

                for (const auto& part : model_children) {
                    if (part.is_valid()) {
                        std::string part_name = part.get_name();
                        if (part_name == "Head" || part_name == "HumanoidRootPart" || part_name == "Humanoid" ||
                            part_name == "Torso" || part_name == "RootPart" || part_name == "hrp") {
                            has_body_parts = true;
                            break;
                        }
                    }
                }

                if (!has_body_parts) continue;

                roblox::player bot_entity;
                bot_entity.name = child.get_name();
                bot_entity.displayname = child.get_name(); 
                bot_entity.main = child;
                bot_entity.address = child.address;
                bot_entity.userid = child;
                bot_entity.instance = child;

                for (const auto& model_child : model_children) {
                    if (!model_child.is_valid()) continue;
                    try {
                        std::string part_name = model_child.get_name();

                        if (part_name == "Head" || part_name == "head") {
                            bot_entity.head = model_child;
                        }
                        else if (part_name == "HumanoidRootPart" || part_name == "RootPart" ||
                                 part_name == "hrp" || part_name == "Torso") {
                            bot_entity.hrp = model_child;
                        }
                        else if (part_name == "Humanoid" || part_name == "humanoid") {
                            bot_entity.humanoid = model_child;
                        }
                        else if (part_name == "UpperTorso" || part_name == "uppertorso") {
                            bot_entity.uppertorso = model_child;
                        }
                        else if (part_name == "LowerTorso" || part_name == "lowertorso") {
                            bot_entity.lowertorso = model_child;
                        }
                        else if (part_name == "LeftUpperArm" || part_name == "leftupperarm") {
                            bot_entity.leftupperarm = model_child;
                        }
                        else if (part_name == "RightUpperArm" || part_name == "rightupperarm") {
                            bot_entity.rightupperarm = model_child;
                        }
                        else if (part_name == "LeftLowerArm" || part_name == "leftlowerarm") {
                            bot_entity.leftlowerarm = model_child;
                        }
                        else if (part_name == "RightLowerArm" || part_name == "rightlowerarm") {
                            bot_entity.rightlowerarm = model_child;
                        }
                        else if (part_name == "LeftHand" || part_name == "lefthand") {
                            bot_entity.lefthand = model_child;
                        }
                        else if (part_name == "RightHand" || part_name == "righthand") {
                            bot_entity.righthand = model_child;
                        }
                        else if (part_name == "LeftUpperLeg" || part_name == "leftupperleg") {
                            bot_entity.leftupperleg = model_child;
                        }
                        else if (part_name == "RightUpperLeg" || part_name == "rightupperleg") {
                            bot_entity.rightupperleg = model_child;
                        }
                        else if (part_name == "LeftLowerLeg" || part_name == "leftlowerleg") {
                            bot_entity.leftlowerleg = model_child;
                        }
                        else if (part_name == "RightLowerLeg" || part_name == "rightlowerleg") {
                            bot_entity.rightlowerleg = model_child;
                        }
                        else if (part_name == "LeftFoot" || part_name == "leftfoot") {
                            bot_entity.leftfoot = model_child;
                        }
                        else if (part_name == "RightFoot" || part_name == "rightfoot") {
                            bot_entity.rightfoot = model_child;
                        }
                        else if (part_name == "BodyEffects") {
                            bot_entity.bodyeffects = model_child;
                        }
                    }
                    catch (...) {
                    }
                }

                if (bot_entity.humanoid.is_valid()) {
                    try {
                        bot_entity.health = static_cast<int>(bot_entity.humanoid.read_health());
                        bot_entity.maxhealth = static_cast<int>(bot_entity.humanoid.read_maxhealth());
                    }
                    catch (...) {
                        bot_entity.health = 100;
                        bot_entity.maxhealth = 100;
                    }
                } else {
                    bot_entity.health = 100;
                    bot_entity.maxhealth = 100;
                }

                // Try to get BodyEffects if not already set
                if (!bot_entity.bodyeffects.is_valid()) {
                    bot_entity.bodyeffects = child.findfirstchild("BodyEffects");
                }

                bots.push_back(bot_entity);
            }
            catch (...) {
            }
        }
    }
    catch (...) {
    }

    return bots;
}

roblox::instance ResolveRobloxPath(const std::string& path) {
    if (path.empty()) return roblox::instance{};

    try {
        roblox::instance datamodel = ReadDatamodel(base_address);
        if (!datamodel.is_valid()) return roblox::instance{};

        roblox::instance current_instance = datamodel;

        std::stringstream ss(path);
        std::string segment;
        bool first_segment = true;

        while (std::getline(ss, segment, '.')) {
            if (segment.empty()) continue;

            if (first_segment) {
                first_segment = false;

                if (segment == "Workspace") {
                    current_instance = datamodel.read_service("Workspace");
                } else if (segment == "Players") {
                    current_instance = datamodel.read_service("Players");
                } else if (segment == "Lighting") {
                    current_instance = datamodel.read_service("Lighting");
                } else if (segment == "ReplicatedStorage") {
                    current_instance = datamodel.read_service("ReplicatedStorage");
                } else if (segment == "StarterGui") {
                    current_instance = datamodel.read_service("StarterGui");
                } else {
                    current_instance = datamodel.findfirstchild(segment);
                }
            } else {
                current_instance = current_instance.findfirstchild(segment);
            }

            if (!current_instance.is_valid()) {
                return roblox::instance{};
            }
        }

        return current_instance;
    } catch (...) {
        return roblox::instance{};
    }
}

bool engine::startup() {
    system("cls");
    logger.Banner();

    logger.Section("CORE INITIALIZATION");
    logger.CustomLog(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "ENGINE", "Starting Core Systems...");

    // Ensure overlay can start immediately and not exit early
    globals::firstreceived = true;
    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "OVERLAY", "Launching overlay interface...");
    std::thread overlay_thread_early(overlay::load_interface);
    overlay_thread_early.detach();
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "Overlay thread launched");

    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "DRIVER", "Initializing memory driver interface...");
    mem::initialize();

    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "SCANNER", "Scanning for Roblox process...");
    
    // Verify Roblox is actually running before proceeding
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    bool robloxRunning = false;
    
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        
        if (Process32First(snapshot, &pe32)) {
            do {
                if (strcmp(pe32.szExeFile, "RobloxPlayerBeta.exe") == 0) {
                    robloxRunning = true;
                    break;
                }
            } while (Process32Next(snapshot, &pe32));
        }
        CloseHandle(snapshot);
    }
    
    if (!robloxRunning) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "RobloxPlayerBeta.exe not found - exiting");
        Sleep(2000);
        exit(1);
    }
    
    mem::grabroblox_h();
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "Memory interface established");

    logger.Section("INSTANCE RESOLUTION");
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "RESOLVER", "Reading core Roblox instances...");

    roblox::instance visualengine = ReadVisualEngine();
    roblox::instance datamodel = ReadDatamodel();
    roblox::instance workspace = datamodel.read_workspace();
    roblox::instance players = datamodel.read_service("Players");
    roblox::instance localplayer = players.local_player();
    roblox::instance lighting = datamodel.read_service("Lighting");
    roblox::instance camera = workspace.read_service("Camera");

    logger.Section("ADDRESS VALIDATION");

    if (is_valid_address(visualengine.address)) {
        globals::instances::visualengine = visualengine;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "VisualEngine    -> 0x%p", visualengine.address);
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "Base Address   -> 0x%p", base_address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "VisualEngine Address -> 0x0");
    }

    if (is_valid_address(datamodel.address)) {
        globals::instances::datamodel = datamodel;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "DataModel      -> 0x%p", datamodel.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "DataModel Address -> 0x0");
    }

    if (is_valid_address(workspace.address)) {
        globals::instances::workspace = workspace;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "Workspace      -> 0x%p", workspace.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Workspace Address -> 0x0");
    }

    if (is_valid_address(players.address)) {
        globals::instances::players = players;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "Players        -> 0x%p", players.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Players Address -> 0x0");
    }

    if (is_valid_address(localplayer.address)) {
        globals::instances::localplayer = localplayer;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "LocalPlayer    -> 0x%p", localplayer.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "LocalPlayer Address -> 0x0");
    }

    if (is_valid_address(lighting.address)) {
        globals::instances::lighting = lighting;
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "Lighting       -> 0x%p", lighting.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Lighting Address -> 0x0");
    }

    if (is_valid_address(camera.address)) {
        globals::instances::camera = static_cast<roblox::camera>(camera.address);
        logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "VALID", "Camera         -> 0x%p", camera.address);
    }
    else {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Camera Address -> 0x0");
    }

    // Game detection via PlaceId -> set gamename (add Phantom Forces support)
    if (is_valid_address(datamodel.address)) {
        uint64_t placeId = 0;
        try {
            placeId = read<uint64_t>(datamodel.address + offsets::PlaceId);
        }
        catch (...) {
            placeId = 0;
        }

        // Known mappings (extend as needed)
        // Phantom Forces main place
        if (placeId == 292439477ULL) {
            globals::instances::gamename = "Phantom Forces";
        }
        else if (placeId == 286090429ULL) {
            globals::instances::gamename = "Arsenal";
        }
        else if (placeId == 3233893879ULL) {
            globals::instances::gamename = "Bad Business";
        }
        else {
            globals::instances::gamename = "Universal";
        }
        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "GAME", std::string("Detected placeId: ") + std::to_string(placeId) + ", name: " + globals::instances::gamename);
    }

    logger.Section("THREAD INITIALIZATION");

    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "CACHE", "Initializing player cache system...");
    player_cache playercache;
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "Player cache initialized");

    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "MONITOR", "Starting teleport detection thread...");
    std::thread PlayerCacheThread([&playercache]() {
        static auto lastTeleportCheck = std::chrono::steady_clock::now();
        static const int TELEPORT_CHECK_INTERVAL_MS = 5000; // Check every 5 seconds instead of 2
        
        while (true)
        {
            if (globals::unattach) break;

            try {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTeleportCheck);
                
                // Performance optimization: Reduce teleport check frequency
                if (elapsed.count() >= TELEPORT_CHECK_INTERVAL_MS) {
                    roblox::instance datamodel = ReadDatamodel();
                    if (globals::instances::datamodel.address != datamodel.address) {
                        system("cls");
                        logger.Banner();
                        logger.Section("TELEPORT RECOVERY");
                        logger.CustomLog(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY, "TELEPORT", "Game teleport detected! Emergency rescan initiated...");
                        globals::handlingtp = true;

                        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "RECOVERY", "Reinitializing driver connection...");
                      //  mem::initialize();
                      //  mem::grabroblox_h();

                        std::this_thread::sleep_for(std::chrono::seconds(4));
                        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "RECOVERY", "Resolving new instance addresses...");

                        roblox::instance visualengine = ReadVisualEngine();
                        roblox::instance datamodel = ReadDatamodel();
                        roblox::instance workspace = datamodel.read_workspace();
                        roblox::instance players = datamodel.read_service("Players");
                        roblox::instance localplayer = players.local_player();
                        roblox::instance lighting = datamodel.read_service("Lighting");
                        roblox::instance camera = workspace.read_service("Camera");
                        playercache.Refresh();

                        logger.Section("RECOVERY VALIDATION");

                        if (is_valid_address(datamodel.address)) {
                            globals::instances::datamodel = datamodel;
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "DataModel      -> 0x%p", datamodel.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "DataModel recovery failed");
                        }

                        if (is_valid_address(workspace.address)) {
                            globals::instances::workspace = workspace;
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "Workspace      -> 0x%p", workspace.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "Workspace recovery failed");
                        }

                        if (is_valid_address(players.address)) {
                            globals::instances::players = players;
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "Players        -> 0x%p", players.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "Players recovery failed");
                        }

                        if (is_valid_address(localplayer.address)) {
                            globals::instances::localplayer = localplayer;
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "LocalPlayer    -> 0x%p", localplayer.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "LocalPlayer recovery failed");
                        }

                        if (is_valid_address(lighting.address)) {
                            globals::instances::lighting = lighting;
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "Lighting       -> 0x%p", lighting.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "Lighting recovery failed");
                        }

                        if (is_valid_address(camera.address)) {
                            globals::instances::camera = static_cast<roblox::camera>(camera.address);
                            logger.LogfCustom(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "RECOVERED", "Camera         -> 0x%p", camera.address);
                        }
                        else {
                            logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "FAILED", "Camera recovery failed");
                        }

                        // Refresh game detection after teleport (Phantom Forces support)
                        if (is_valid_address(datamodel.address)) {
                            uint64_t placeId = 0;
                            try {
                                placeId = read<uint64_t>(datamodel.address + offsets::PlaceId);
                            }
                            catch (...) {
                                placeId = 0;
                            }

                            if (placeId == 292439477ULL) {
                                globals::instances::gamename = "Phantom Forces";
                            }
                            else if (placeId == 286090429ULL) {
                                globals::instances::gamename = "Arsenal";
                            }
                            else if (placeId == 3233893879ULL) {
                                globals::instances::gamename = "Bad Business";
                            }
                            else {
                                globals::instances::gamename = "Universal";
                            }
                            logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "GAME", std::string("Detected placeId: ") + std::to_string(placeId) + ", name: " + globals::instances::gamename);
                        }

                        logger.Success("TELEPORT RECOVERY COMPLETED SUCCESSFULLY");
                        globals::handlingtp = false;
                    }
                    else {
                        playercache.UpdateCache();
                    }
                    lastTeleportCheck = now;
                }
            }
            catch (...) {
                logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Exception in monitor thread");
            }

            // Performance optimization: Increased sleep time to reduce CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Increased from 2 seconds
        }
        });
    PlayerCacheThread.detach();
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "Monitor thread launched");

    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "HOOKS", "Launching exploitation hooks...");
    try {
        hooking::launchThreads();

        logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "WAIT", "Waiting for all threads to initialize...");
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));

        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "All hook threads launched and initialized");
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Failed to launch hook threads");
    }

    // Auto-detect Aim frame for silent aim in any game
    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, "AIM", "Starting automatic Aim frame detection thread...");
    std::thread aimDetectionThread([]() {
        while (true) {
            if (globals::unattach) break;

            try {
                // Check if datamodel and localplayer are valid
                if (!is_valid_address(globals::instances::datamodel.address) || 
                    !is_valid_address(globals::instances::localplayer.address)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                // Get PlayerGui
                roblox::instance playerGui = globals::instances::localplayer.findfirstchild("PlayerGui");
                if (!is_valid_address(playerGui.address)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    continue;
                }

                // Recursively search for "Aim" frame in PlayerGui
                // Search through all children and grandchildren
                auto children = playerGui.get_children();
                for (auto& child : children) {
                    if (!is_valid_address(child.address)) continue;
                    
                    // Check direct children first
                    if (child.get_name() == "Aim") {
                        globals::instances::aim = child;
                        continue;
                    }
                    
                    // Check grandchildren
                    auto grandchildren = child.get_children();
                    for (auto& grandchild : grandchildren) {
                        if (!is_valid_address(grandchild.address)) continue;
                        
                        if (grandchild.get_name() == "Aim") {
                            globals::instances::aim = grandchild;
                            break;
                        }
                        
                        // Check great-grandchildren (for nested structures)
                        auto greatGrandchildren = grandchild.get_children();
                        for (auto& ggc : greatGrandchildren) {
                            if (!is_valid_address(ggc.address)) continue;
                            
                            if (ggc.get_name() == "Aim") {
                                globals::instances::aim = ggc;
                                break;
                            }
                        }
                    }
                }
            }
            catch (...) {
                // Silently handle errors
            }

            // Check every 500ms for new Aim frames
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    aimDetectionThread.detach();
    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SUCCESS", "Aim frame detection thread launched");


    logger.CustomLog(FOREGROUND_BLUE | FOREGROUND_INTENSITY, "RUNTIME", "Engine running... Press ENTER to terminate");
    logger.Separator();
    std::cout << "[KYZO] DEBUGGING BELOW" << std::endl;
    try {
        while (true) {
            if (globals::unattach) {
                logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "SHUTDOWN", "Unattach signal received");
                break;
            }

            if (std::cin.peek() != EOF) {
                char c;
                if (std::cin.get(c)) {
                    if (c == '\n' || c == '\r') {
                        logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "SHUTDOWN", "User requested termination");
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (...) {
        logger.CustomLog(FOREGROUND_RED | FOREGROUND_INTENSITY, "ERROR", "Exception in main loop");
    }

    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY, "SHUTDOWN", "Initiating cleanup sequence...");
    globals::unattach = true;

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    logger.CustomLog(FOREGROUND_GREEN | FOREGROUND_INTENSITY, "SHUTDOWN", "Engine terminated successfully");

    return true;
}

// World to screen conversion implementation
math::Vector2 roblox::instance::worldtoscreen(const math::Vector3& world_pos) {
    try {
        // Get camera position and view matrix
        auto camera = globals::instances::camera;
        if (camera.address == 0) {
            return math::Vector2(-1, -1);
        }
        
        auto view_matrix = globals::instances::visualengine.GetViewMatrix();
        auto screen_dimensions = globals::instances::visualengine.get_dimensions();
        
        if (screen_dimensions.x == 0 || screen_dimensions.y == 0) {
            return math::Vector2(-1, -1);
        }
        
        // Simple projection calculation
        // This is a basic implementation - you may need to adjust based on your specific needs
        math::Vector3 camera_pos = camera.getPos();
        math::Vector3 direction = world_pos - camera_pos;
        
        // Normalize direction
        float distance = direction.magnitude();
        if (distance < 0.001f) {
            return math::Vector2(-1, -1);
        }
        
        direction = direction * (1.0f / distance);
        
        // Simple perspective projection
        float fov = 70.0f; // Field of view in degrees
        float fov_rad = fov * (3.14159f / 180.0f);
        float aspect_ratio = screen_dimensions.x / screen_dimensions.y;
        
        float x = direction.x / (direction.z * std::tan(fov_rad / 2.0f));
        float y = direction.y / (direction.z * std::tan(fov_rad / 2.0f) / aspect_ratio);
        
        // Convert to screen coordinates
        float screen_x = (x + 1.0f) * 0.5f * screen_dimensions.x;
        float screen_y = (1.0f - y) * 0.5f * screen_dimensions.y;
        
        // Check if point is behind camera
        if (direction.z <= 0) {
            return math::Vector2(-1, -1);
        }
        
        return math::Vector2(screen_x, screen_y);
    }
    catch (...) {
        return math::Vector2(-1, -1);
    }
}
