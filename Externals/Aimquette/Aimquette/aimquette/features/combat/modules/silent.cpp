#include "../combat.h"
#include <thread>
#include <chrono>
#include <Windows.h>
#include "../../../util/console/console.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "../../hook.h"
#include "../../wallcheck/wallcheck.h"

/*
 * Silent Aim Part Position Fixes:
 * 
 * 1. Fixed inconsistent partpos calculation across different target selection modes
 * 2. Added proper validation for part positions to prevent invalid coordinates
 * 3. Improved closest part calculation with better candidate filtering
 * 4. Enhanced closest point mode with dynamic bounding box calculation
 * 5. Added fallback mechanisms when part positions fail to calculate
 * 6. Fixed worldtoscreen function call (GetDimensins -> get_dimensions)
 * 7. Added screen bounds validation and clamping
 * 8. Improved prediction handling for all modes
 * 9. Added game ID detection for instance validation
 * 10. Implemented game-specific validation settings for different Roblox games
 */

#define max
#undef max
#define min
#undef min

using namespace roblox;

bool foundTarget = false;

// Helper function to check if player is in same crew/group (checks team membership)
static bool isSameCrew(const roblox::player& target) {
    if (!globals::combat::crew_check) return false;
    
    try {
        // Check if local player and target have valid teams
        if (!is_valid_address(globals::instances::lp.main.address) || !is_valid_address(target.main.address)) {
            return false;
        }
        
        // Get teams
        roblox::instance localTeam = globals::instances::lp.team;
        roblox::instance targetTeam = target.team;
        
        // If both have valid teams and they're the same, they're in the same crew
        if (localTeam.is_valid() && targetTeam.is_valid()) {
            return localTeam.address == targetTeam.address;
        }
        
        // Alternative: Check if they have the same team name (crew/group indicator)
        if (localTeam.is_valid() && targetTeam.is_valid()) {
            std::string localTeamName = localTeam.get_name();
            std::string targetTeamName = targetTeam.get_name();
            if (!localTeamName.empty() && !targetTeamName.empty()) {
                return localTeamName == targetTeamName;
            }
        }
    } catch (...) {
        // Silently handle errors
    }
    
    return false;
}
static math::Vector2 partpos = {};
static uint64_t cachedPositionX = 0;
static uint64_t cachedPositionY = 0;
static bool dataReady = false;
static bool targetNeedsReset = false;


using namespace roblox;
inline float fastdist(const math::Vector2& a, const math::Vector2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool isAutoFunctionActive() {
    return globals::bools::kill || globals::bools::autokill;
}

bool shouldSilentAimBeActive() {
    return (globals::focused && globals::combat::silentaim && globals::combat::silentaimkeybind.enabled) || isAutoFunctionActive();
}


// Helper function to get target closest to camera for silent aim
roblox::player gettargetclosesttocamerasilent() {
    // Add bots to the player list
    std::vector<roblox::player> bots = GetBots(base_address);
    std::vector<roblox::player> all_players = globals::instances::cachedplayers;
    all_players.insert(all_players.end(), bots.begin(), bots.end());
    const auto& players = all_players;
    
    if (players.empty()) return {};

    const bool useKnockCheck = (*globals::combat::flags)[1] != 0;
    const bool useHealthCheck = (*globals::combat::flags)[3] != 0;
    const bool useWallCheck = (*globals::combat::flags)[4] != 0;
    const bool useTeamCheck = (*globals::combat::flags)[0] != 0;
    const bool useRangeCheck = (*globals::combat::flags)[2] != 0;
    const bool useFov = globals::combat::usesfov;
    const float fovSize = globals::combat::sfovsize;
    const float fovSizeSquared = fovSize * fovSize;
    const float healthThreshold = globals::combat::healththreshhold;
    const bool isArsenal = (globals::instances::gamename == "Arsenal");
    const std::string& localPlayerName = globals::instances::localplayer.get_name();
    const math::Vector3 cameraPos = globals::instances::camera.getPos();

    roblox::player closest = {};
    float closestDistanceSquared = 9e18f;

    for (auto player : players) {
        if (!is_valid_address(player.main.address) || !is_valid_address(player.head.address)) continue;
        if (player.name == localPlayerName) continue;

        // Arsenal-specific checks (from seizure)
        if (isArsenal) {
            auto nrpbs = player.main.findfirstchild("NRPBS");
            if (nrpbs.address) {
                auto health = nrpbs.findfirstchild("Health");
                if (health.address && (health.read_double_value() == 0.0 || player.hrp.get_pos().y < 0.0f)) {
                    continue;
                }
            }
        }

        // BodyEffects check (from seizure)
        auto bodyEffects = player.instance.findfirstchild("BodyEffects");
        // Death check (always skip dead players during target selection) - from layuh22
        try {
            auto bodyEffects = player.instance.findfirstchild("BodyEffects");
            if (bodyEffects.address) {
                // Check for Dead flag in BodyEffects
                if (bodyEffects.findfirstchild("Dead").read_bool_value()) continue;
                
                // Check for K.O. state
                if (useKnockCheck && bodyEffects.findfirstchild("K.O").read_bool_value()) continue;
                
                // Check for respawn state
                if (bodyEffects.findfirstchild("Respawn").read_bool_value()) continue;
            }
            
            // Check health
            if (player.health <= 0) continue;
            
            // Check if player is below ground (common death indicator)
            math::Vector3 playerPos = player.hrp.get_pos();
            if (playerPos.y < -50.0f) continue; // Below ground threshold
            
            // Arsenal-specific death check
            if (isArsenal) {
                auto nrpbs = player.main.findfirstchild("NRPBS");
                if (nrpbs.address) {
                    auto health = nrpbs.findfirstchild("Health");
                    if (health.address && health.read_double_value() == 0.0) continue;
                }
            }
        } catch (...) { continue; }

        // Forcefield check (from seizure)
        const bool useforcefieldcheck = (*globals::combat::flags)[6];
        bool hasaforcefield = false;
        if (useforcefieldcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "ForceField") {
                    hasaforcefield = true;
                    break;
                }
            }
            if (hasaforcefield) {
                continue;
            }
        }

        // Grabbed check (from seizure - uses GRABBING_CONSTRAINT)
        const bool usegrabbedcheck = (*globals::combat::flags)[5];
        bool isgrabbed = false;
        if (usegrabbedcheck) {
            auto children = player.instance.get_children();
            for (roblox::instance& instance : children) {
                if (instance.get_name() == "GRABBING_CONSTRAINT") {
                    isgrabbed = true;
                    break;
                }
            }
            if (isgrabbed) {
                continue;
            }
        }

        // Health check (from seizure) - works with custom models
        if (useHealthCheck) {
            int playerHealth = player.health;
            // Fallback: read from humanoid if health is 0 or not set (for custom models)
            if (playerHealth == 0 && player.humanoid.address) {
                playerHealth = player.humanoid.read_health();
            }
            if (playerHealth <= static_cast<int>(healthThreshold)) continue;
        }

        // Range check (from seizure) - works with custom models
        if (useRangeCheck) {
            math::Vector3 targetPos = player.hrp.get_pos();
            float dx = cameraPos.x - targetPos.x;
            float dy = cameraPos.y - targetPos.y;
            float dz = cameraPos.z - targetPos.z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (distance > globals::combat::range) continue;
        }

        // Crew check - skip players in same roblox group/crew
        if (globals::combat::crew_check) {
            if (isSameCrew(player)) {
                continue; // Same crew/group, skip
            }
        }
        
        // Team check (from seizure) - works with custom models
        if (useTeamCheck) {
            roblox::instance localPlayerTeam = globals::instances::lp.team;
            if (localPlayerTeam.address != 0 && player.team.address != 0) {
                if (localPlayerTeam.address == player.team.address) {
                    continue; // Same team, skip
                }
            }
        }

        // Wall check (from seizure)
        if (useWallCheck && !wallcheck::can_see(player.head.get_pos(), cameraPos)) continue;

        math::Vector3 targetPos = player.hrp.get_pos();
        float distanceSquared = (cameraPos.x - targetPos.x) * (cameraPos.x - targetPos.x) + 
                               (cameraPos.y - targetPos.y) * (cameraPos.y - targetPos.y) + 
                               (cameraPos.z - targetPos.z) * (cameraPos.z - targetPos.z);

        // FOV check
        if (useFov) {
            math::Vector2 screenPos = roblox::worldtoscreen(targetPos);
            if (screenPos.x == -1.0f || screenPos.y == -1.0f) {
                continue;
            } else {
                math::Vector2 screenCenter = { 960.0f, 540.0f }; // Assuming 1920x1080, adjust as needed
                float screenDistanceSquared = (screenPos.x - screenCenter.x) * (screenPos.x - screenCenter.x) + 
                                            (screenPos.y - screenCenter.y) * (screenPos.y - screenCenter.y);
                if (screenDistanceSquared > fovSizeSquared) continue;
            }
        }

        // Wall check (from seizure)
        if (useWallCheck && !wallcheck::can_see(player.head.get_pos(), cameraPos)) {
            continue;
        }

        if (distanceSquared < closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closest = player;
        }
    }

    // Return closest target
    return closest;
}

roblox::player gettargetclosesttomousesilent() {
    static HWND robloxWindow = nullptr;
    static auto lastWindowCheck = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();
    if (!robloxWindow || std::chrono::duration_cast<std::chrono::seconds>(now - lastWindowCheck).count() > 5) {
        robloxWindow = FindWindowA(nullptr, "Roblox");
        lastWindowCheck = now;
    }

    if (!robloxWindow) return {};

    POINT point;
    if (!GetCursorPos(&point) || !ScreenToClient(robloxWindow, &point)) {
        return {};
    }

    const math::Vector2 curpos = { static_cast<float>(point.x), static_cast<float>(point.y) };
    
    // Add bots to the player list
    std::vector<roblox::player> bots = GetBots(base_address);
    std::vector<roblox::player> all_players = globals::instances::cachedplayers;
    all_players.insert(all_players.end(), bots.begin(), bots.end());
    const auto& players = all_players;

    if (players.empty()) return {};

    const bool useKnockCheck = (*globals::combat::flags)[1] != 0;
    const bool useHealthCheck = (*globals::combat::flags)[3] != 0;
    const bool useWallCheck = (*globals::combat::flags)[4] != 0;
    const bool useTeamCheck = (*globals::combat::flags)[0] != 0;
    const bool useRangeCheck = (*globals::combat::flags)[2] != 0;
    const bool allowWallFallback = true; // fallback built-in; UI toggle optional
            const bool useFov = globals::combat::usesfov;
        const float fovSize = globals::combat::sfovsize;
        const float fovSizeSquared = fovSize * fovSize;
    const float healthThreshold = 0.0f;
    const bool isArsenal = (globals::instances::gamename == "Arsenal");
    const std::string& localPlayerName = globals::instances::localplayer.get_name();
    const math::Vector3 cameraPos = globals::instances::camera.getPos();

    roblox::player closest = {};
    roblox::player closestVisible = {};
    float closestDistanceSquared = 9e18f;
    float closestVisibleDistanceSquared = 9e18f;

    for (auto player : players) {
        if (!is_valid_address(player.main.address) ||
            player.name == localPlayerName ||
            player.head.address == 0) {
            continue;
        }

        // Use player.bodyeffects if available (works for both regular players and custom models)
        auto bodyEffects = player.bodyeffects;
        if (!bodyEffects.address) {
            // Fallback: try to find BodyEffects from instance (for custom models)
            bodyEffects = player.instance.findfirstchild("BodyEffects");
        }

        // Death check (always skip dead players during target selection) - works with custom models
        if (bodyEffects.address) {
            try {
                auto deadFlag = bodyEffects.findfirstchild("Dead");
                if (deadFlag.address && deadFlag.read_bool_value()) continue;
                
                auto respawnFlag = bodyEffects.findfirstchild("Respawn");
                if (respawnFlag.address && respawnFlag.read_bool_value()) continue;
            } catch (...) { }
        }
        
        // Check health - works with custom models
        if (player.humanoid.address) {
            int playerHealth = player.humanoid.read_health();
            if (playerHealth <= 0) continue;
        }
        
        // Check if player is below ground (common death indicator)
        math::Vector3 playerPos = player.hrp.get_pos();
        if (playerPos.y < -50.0f) continue; // Below ground threshold

        // Knock check - works with custom models (checks for knocked or dead)
        if (globals::combat::knockcheck && bodyEffects.address) {
            try {
                // Check for knocked (K.O flag)
                auto koFlag = bodyEffects.findfirstchild("K.O");
                if (koFlag.address && koFlag.read_bool_value()) continue;
                // Check for dead (Dead flag)
                auto deadFlag = bodyEffects.findfirstchild("Dead");
                if (deadFlag.address && deadFlag.read_bool_value()) continue;
            } catch (...) { }
        }

        // Health check - works with custom models
        if (globals::combat::healthcheck) {
            if (player.humanoid.address) {
                int playerHealth = player.humanoid.read_health();
                if (playerHealth <= static_cast<int>(globals::combat::healththreshhold)) continue;
            }
        }

        // Range check - works with custom models
        if ((*globals::combat::flags)[2] != 0) { // Range check flag
            math::Vector3 targetPos = player.hrp.get_pos();
            float dx = cameraPos.x - targetPos.x;
            float dy = cameraPos.y - targetPos.y;
            float dz = cameraPos.z - targetPos.z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (distance > globals::combat::range) continue;
        }

        // Team check - skip teammates if teamcheck is enabled (works with custom models)
        if (globals::combat::teamcheck && globals::is_teammate(player)) {
            continue; // Skip teammate
        }

        // Grabbed check - skip players who are grabbed if enabled (works with custom models)
        if (globals::combat::grabbedcheck && globals::is_grabbed(player)) {
            continue; // Skip grabbed player
        }

        const math::Vector2 screenCoords = roblox::worldtoscreen(player.head.get_pos());
        if (screenCoords.x == -1.0f || screenCoords.y == -1.0f) {
            continue;
        }

        const float dx = curpos.x - screenCoords.x;
        const float dy = curpos.y - screenCoords.y;
        const float distanceSquared = dx * dx + dy * dy;

        if (useFov && distanceSquared > fovSizeSquared) continue;

        if (isArsenal) {
            auto nrpbs = player.main.findfirstchild("NRPBS");
            if (nrpbs.address) {
                auto health = nrpbs.findfirstchild("Health");
                if (health.address && (health.read_double_value() == 0.0 || player.hrp.get_pos().y < 0.0f)) {
                    continue;
                }
            }
        }

        // All secondary checks removed

        // Removed ForceField/Grabbed checks

        // Wall check using raycasting - check from local player position
        bool isVisible = true;
        if (useWallCheck && wallcheck::g_wall_checker) {
            Vector3 localPlayerPos = globals::instances::lp.hrp.get_pos();
            math::Vector3 targetPartPos = player.hrp.get_pos();
            Vector3 from = { localPlayerPos.x, localPlayerPos.y, localPlayerPos.z };
            Vector3 to = { targetPartPos.x, targetPartPos.y, targetPartPos.z };
            isVisible = wallcheck::g_wall_checker->can_see(from, to);
        }

        // Forcefield check - skip players with ForceField
        bool hasForceField = false;
        if ((*globals::combat::flags)[6] != 0) { // Forcefield check flag
            auto children = player.instance.get_children();
            for (const auto& child : children) {
                if (child.get_name() == "ForceField") {
                    hasForceField = true;
                    break;
                }
            }
        }
        if (hasForceField) continue;

        if (isVisible) {
            if (distanceSquared < closestVisibleDistanceSquared) {
                closestVisibleDistanceSquared = distanceSquared;
                closestVisible = player;
            }
        } else {
            if (distanceSquared < closestDistanceSquared) {
                closestDistanceSquared = distanceSquared;
                closest = player;
            }
        }
    }

    if (is_valid_address(closestVisible.main.address)) return closestVisible;
    if (allowWallFallback) return closest;
    return {};
}

void hooks::silentrun() {
    roblox::player target;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    HWND robloxWindow = FindWindowA(0, "Roblox");
    const std::chrono::milliseconds sleepTime(10);

    while (true) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(47000));
        if (globals::handlingtp) {
            dataReady = false;
            globals::instances::cachedtarget = {};
            foundTarget = false;
            targetNeedsReset = false;
            continue;
        }
        globals::combat::silentaimkeybind.update();

        if (!shouldSilentAimBeActive()) {
            dataReady = false;
            // Clear cached target when silent aim is deactivated to allow manual unlocking
            if (globals::combat::stickyaimsilent) {
                globals::instances::cachedtarget = {};
            }
            target = {};
            foundTarget = false;
            targetNeedsReset = false;
            continue;
        }

        // Dynamic aim instance search through PlayerGui children
        roblox::instance plrgui = globals::instances::localplayer.findfirstchild("PlayerGui");
        if (plrgui.address) {
            auto chdr = plrgui.get_children();
            for (auto& child : chdr) {
                auto schild = child.get_children();
                for (auto& child2 : schild) {
                    if (child2.get_name() == "Aim") {
                        globals::instances::aim = child2;
                        break;
                    }
                }
            }
        }

        if (isAutoFunctionActive()) {
            target = globals::bools::entity;
            globals::instances::cachedtarget = target;
            foundTarget = (target.head.address != 0);
        }
        else if (globals::combat::connect_to_aimbot) {
            // When connected to aimbot, get the current aimbot target directly
            // This will use the same target that the aimbot (camlock) is currently locked onto
            if (globals::combat::aimbot_locked && is_valid_address(globals::combat::aimbot_current_target.main.address)) {
                target = globals::combat::aimbot_current_target;
                foundTarget = (target.head.address != 0);
                
                // Validate the target is still alive and valid for silent aim
                if (foundTarget) {
                    try {
                        // Check if target is still alive
                        if (target.bodyeffects.findfirstchild("Dead").read_bool_value()) {
                            foundTarget = false;
                            target = {};
                        } else if (target.health <= 0) {
                            foundTarget = false;
                            target = {};
                        } else {
                            math::Vector3 targetPos = target.hrp.get_pos();
                            if (targetPos.y < -50.0f) { // Below ground threshold
                                foundTarget = false;
                                target = {};
                            }
                        }
                    } catch (...) {
                        foundTarget = false;
                        target = {};
                    }
                }
            } else {
                // No aimbot target available or aimbot not locked
                target = {};
                foundTarget = false;
            }
            // Don't update cachedtarget here to avoid interfering with aimbot
            // Don't use sticky aim when connected to aimbot - always follow aimbot's target
        }
        else {
            // Check if we have a valid cached target and sticky aim is enabled
            if (globals::combat::stickyaimsilent && is_valid_address(globals::instances::cachedtarget.main.address)) {
                // Validate the current target
                bool currentTargetValid = true;
                
                // Check if target is still alive and valid
                try {
                    if (globals::instances::cachedtarget.bodyeffects.findfirstchild("Dead").read_bool_value()) {
                        currentTargetValid = false;
                    } else if (globals::instances::cachedtarget.health <= 0) {
                        currentTargetValid = false;
                    } else {
                        math::Vector3 targetPos = globals::instances::cachedtarget.hrp.get_pos();
                        if (targetPos.y < -50.0f) { // Below ground threshold
                            currentTargetValid = false;
                        }
                    }
                } catch (...) {
                    currentTargetValid = false;
                }
                
                if (currentTargetValid) {
                    // Keep the current target
                    target = globals::instances::cachedtarget;
                    foundTarget = true;
                } else {
                    // Current target is invalid, look for new one
                    if (globals::combat::target_method == 0) {
                        target = gettargetclosesttomousesilent();
                    } else {
                        target = gettargetclosesttocamerasilent();
                    }
                    globals::instances::cachedlasttarget = target;
                    foundTarget = (target.head.address != 0);
                    globals::instances::cachedtarget = target;
                }
            } else {
                // No sticky aim or no cached target, use target method to determine which function to call
                if (globals::combat::target_method == 0) {
                    target = gettargetclosesttomousesilent();
                } else {
                    target = gettargetclosesttocamerasilent();
                }
                globals::instances::cachedlasttarget = target;
                foundTarget = (target.head.address != 0);
                globals::instances::cachedtarget = target;
            }
        }

        // Death check for current target (unlock on death)
        // Works for both regular players and custom models
        if (foundTarget && globals::combat::unlockondeath && is_valid_address(target.main.address)) {
            try {
                // Use target.bodyeffects if available (works for both regular players and custom models)
                auto bodyEffects = target.bodyeffects;
                if (!bodyEffects.address) {
                    // Fallback: try to find BodyEffects from instance (for custom models)
                    bodyEffects = target.instance.findfirstchild("BodyEffects");
                }
                if (bodyEffects.address) {
                    // Check for Dead flag in BodyEffects
                    auto deadFlag = bodyEffects.findfirstchild("Dead");
                    if (deadFlag.address && deadFlag.read_bool_value()) {
                        foundTarget = false; // Drop target if they die
                        target = {};
                        globals::instances::cachedtarget = {};
                        continue;
                    }
                    
                    // Check for K.O. state
                    auto koFlag = bodyEffects.findfirstchild("K.O");
                    if (koFlag.address && koFlag.read_bool_value()) {
                        foundTarget = false; // Drop target if they're knocked out
                        target = {};
                        globals::instances::cachedtarget = {};
                        continue;
                    }
                    
                    // Check for respawn state
                    auto respawnFlag = bodyEffects.findfirstchild("Respawn");
                    if (respawnFlag.address && respawnFlag.read_bool_value()) {
                        foundTarget = false; // Drop target if they're respawning
                        target = {};
                        globals::instances::cachedtarget = {};
                        continue;
                    }
                }
                
                // Check health
                if (target.health <= 0) {
                    foundTarget = false; // Drop target if they die
                    target = {};
                    globals::instances::cachedtarget = {};
                    continue;
                }
                
                // Check if target is below ground (common death indicator)
                math::Vector3 targetPosCheck = target.hrp.get_pos();
                if (targetPosCheck.y < -50.0f) { // Below ground threshold
                    foundTarget = false; // Drop target if they're below ground
                    target = {};
                    globals::instances::cachedtarget = {};
                    continue;
                }
                
                // Arsenal-specific death check
                if (globals::instances::gamename == "Arsenal") {
                    auto nrpbs = target.main.findfirstchild("NRPBS");
                    if (nrpbs.address) {
                        auto health = nrpbs.findfirstchild("Health");
                        if (health.address && health.read_double_value() == 0.0) {
                            foundTarget = false; // Drop target if they die
                            target = {};
                            globals::instances::cachedtarget = {};
                            continue;
                        }
                    }
                }
            } catch (...) { }
        }

        // Knock check (from seizure) - unlock when knocked or dead (don't auto-switch)
        if ((*globals::combat::flags)[1] != 0) {
            auto bodyEffects = target.instance.findfirstchild("BodyEffects");
            if (bodyEffects.address) {
                // Check for knocked (K.O flag)
                if (bodyEffects.findfirstchild("K.O").read_bool_value()) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                // Check for dead (Dead flag)
                if (bodyEffects.findfirstchild("Dead").read_bool_value()) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
            // Also check health <= 0 as death indicator
            int targetHealth = target.health;
            if (targetHealth == 0 && target.humanoid.address) {
                targetHealth = target.humanoid.read_health();
            }
            if (targetHealth == 0) {
                auto bodyEffects = target.instance.findfirstchild("BodyEffects");
                if (bodyEffects.address) {
                    auto health = bodyEffects.findfirstchild("Health");
                    if (health.address) {
                        targetHealth = static_cast<int>(health.read_double_value());
                    }
                }
            }
            if (targetHealth <= 0) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            // Check if HRP is below ground (death indicator)
            math::Vector3 hrpPos = target.hrp.get_pos();
            if (hrpPos.y < -50.0f) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Range check (from seizure) - unlock if target is too far (don't auto-switch)
        if ((*globals::combat::flags)[2] != 0) {
            math::Vector3 cameraPos = globals::instances::camera.getPos();
            math::Vector3 targetPos = target.hrp.get_pos();
            float dx = cameraPos.x - targetPos.x;
            float dy = cameraPos.y - targetPos.y;
            float dz = cameraPos.z - targetPos.z;
            float distance = sqrt(dx * dx + dy * dy + dz * dz);
            if (distance > globals::combat::range) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        // Team check (from seizure) - unlock if same team (don't auto-switch)
        if ((*globals::combat::flags)[0] != 0) {
            roblox::instance localPlayerTeam = globals::instances::lp.team;
            if (localPlayerTeam.address != 0 && target.team.address != 0) {
                if (localPlayerTeam.address == target.team.address) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
            }
        }

        // Wall check (from seizure) - unlock if wall is blocking (don't auto-switch)
        if ((*globals::combat::flags)[4] != 0) {
            math::Vector3 cameraPos = globals::instances::camera.getPos();
            Vector3 from = { cameraPos.x, cameraPos.y, cameraPos.z };
            Vector3 to = { target.head.get_pos().x, target.head.get_pos().y, target.head.get_pos().z };
            if (!wallcheck::can_see(to, from)) {
                foundTarget = false;
                target = {};
                globals::instances::cachedtarget = {};
                // Don't auto-switch, wait before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }
        if (foundTarget && globals::instances::cachedtarget.head.address != 0) {
            if (isAutoFunctionActive()) {
                        }
            roblox::instance part = globals::instances::cachedtarget.head;
            
            // Enhanced target selection logic with body part selection
            if (globals::combat::closestpartsilent == 1) {
                // Choose closest body part to crosshair for silent aim
                struct Candidate { roblox::instance inst; };
                std::vector<Candidate> candidates;
                candidates.push_back({ globals::instances::cachedtarget.head });
                candidates.push_back({ globals::instances::cachedtarget.uppertorso });
                candidates.push_back({ globals::instances::cachedtarget.lowertorso });
                candidates.push_back({ globals::instances::cachedtarget.hrp });
                candidates.push_back({ globals::instances::cachedtarget.lefthand });
                candidates.push_back({ globals::instances::cachedtarget.righthand });
                candidates.push_back({ globals::instances::cachedtarget.leftfoot });
                candidates.push_back({ globals::instances::cachedtarget.rightfoot });
                POINT p; HWND rw = FindWindowA(nullptr, "Roblox"); if (rw) { GetCursorPos(&p); ScreenToClient(rw, &p); }
                math::Vector2 mouse = { (float)p.x, (float)p.y };
                float bestDist = 9e18f; roblox::instance best = part;
                
                // Filter out invalid candidates first
                std::vector<Candidate> validCandidates;
                for (auto& c : candidates) {
                    if (is_valid_address(c.inst.address)) {
                        // For sticky aim, include all valid body parts even if off screen
                        validCandidates.push_back(c);
                    }
                }
                
                // Find the closest valid part
                for (auto& c : validCandidates) {
                    math::Vector2 scr = roblox::worldtoscreen(c.inst.get_pos());
                    float dx, dy, d2;
                    
                    if (scr.x == -1.0f || scr.y == -1.0f) {
                        // Use a large distance to prioritize on-screen parts
                        scr = { 999999.0f, 999999.0f };
                        dx = mouse.x - scr.x; dy = mouse.y - scr.y; d2 = dx * dx + dy * dy;
                    } else {
                        dx = mouse.x - scr.x; dy = mouse.y - scr.y; d2 = dx * dx + dy * dy;
                    }
                    
                    if (d2 < bestDist) { bestDist = d2; best = c.inst; }
                }
                
                if (is_valid_address(best.address)) part = best;
                
                // Calculate partpos for the selected closest part
                math::Vector3 part3d = part.get_pos();
                if (globals::combat::silentpredictions) {
                    math::Vector3 velocity = part.get_velocity();
                    math::Vector3 veloVector = velocity / math::Vector3(globals::combat::silentpredictionsx, globals::combat::silentpredictionsy, globals::combat::silentpredictionsx);
                    part3d = part3d + veloVector;
                }
                partpos = roblox::worldtoscreen(part3d);
                
                // Validate the calculated partpos
                if (partpos.x == -1.0f || partpos.y == -1.0f) {
                    // If the selected part failed, try to find any valid part
                    for (auto& c : validCandidates) {
                        math::Vector3 fallbackPos = c.inst.get_pos();
                        if (globals::combat::silentpredictions) {
                            math::Vector3 velocity = c.inst.get_velocity();
                            math::Vector3 veloVector = velocity / math::Vector3(globals::combat::silentpredictionsx, globals::combat::silentpredictionsy, globals::combat::silentpredictionsx);
                            fallbackPos = fallbackPos + veloVector;
                        }
                        partpos = roblox::worldtoscreen(fallbackPos);
                        if (partpos.x != -1.0f && partpos.y != -1.0f) {
                            break;
                        }
                    }
                }
            }
            else if (globals::combat::closestpartsilent == 2) {
                // Choose closest point on the target to crosshair for silent aim
                POINT p; HWND rw = FindWindowA(nullptr, "Roblox"); if (rw) { GetCursorPos(&p); ScreenToClient(rw, &p); }
                math::Vector2 mouse = { (float)p.x, (float)p.y };
                
                // Get target's position and convert to screen coordinates
                math::Vector3 targetPos = globals::instances::cachedtarget.hrp.get_pos();
                math::Vector2 targetScreen = roblox::worldtoscreen(targetPos);
                
                // Check if target is visible on screen
                if (targetScreen.x == -1.0f || targetScreen.y == -1.0f) {
                    // Target not visible, fallback to HRP
                    math::Vector3 fallbackPos = targetPos;
                    if (globals::combat::silentpredictions) {
                        math::Vector3 velocity = globals::instances::cachedtarget.hrp.get_velocity();
                        math::Vector3 veloVector = velocity / math::Vector3(globals::combat::silentpredictionsx, globals::combat::silentpredictionsy, globals::combat::silentpredictionsx);
                        fallbackPos = fallbackPos + veloVector;
                    }
                    partpos = roblox::worldtoscreen(fallbackPos);
                } else {
                    // Calculate dynamic bounding box based on target's actual size
                    // Use multiple body parts to determine target bounds
                    std::vector<math::Vector2> bodyPartScreens;
                    std::vector<roblox::instance> bodyParts = {
                        globals::instances::cachedtarget.head,
                        globals::instances::cachedtarget.uppertorso,
                        globals::instances::cachedtarget.lowertorso,
                        globals::instances::cachedtarget.lefthand,
                        globals::instances::cachedtarget.righthand,
                        globals::instances::cachedtarget.leftfoot,
                        globals::instances::cachedtarget.rightfoot
                    };
                    
                    for (auto& bp : bodyParts) {
                        if (is_valid_address(bp.address)) {
                            math::Vector2 screenPos = roblox::worldtoscreen(bp.get_pos());
                            if (screenPos.x != -1.0f && screenPos.y != -1.0f) {
                                bodyPartScreens.push_back(screenPos);
                            }
                        }
                    }
                    
                    if (bodyPartScreens.empty()) {
                        // Fallback to simple bounding box
                        float boxWidth = 60.0f;
                        float boxHeight = 120.0f;
                        float closestX = std::max(targetScreen.x - boxWidth/2, std::min(targetScreen.x + boxWidth/2, mouse.x));
                        float closestY = std::max(targetScreen.y - boxHeight/2, std::min(targetScreen.y + boxHeight/2, mouse.y));
                        partpos = { closestX, closestY };
                    } else {
                        // Calculate actual bounding box from visible body parts
                        float minX = bodyPartScreens[0].x, maxX = bodyPartScreens[0].x;
                        float minY = bodyPartScreens[0].y, maxY = bodyPartScreens[0].y;
                        
                        for (const auto& pos : bodyPartScreens) {
                            minX = std::min(minX, pos.x);
                            maxX = std::max(maxX, pos.x);
                            minY = std::min(minY, pos.y);
                            maxY = std::max(maxY, pos.y);
                        }
                        
                        // Add some padding to the bounding box
                        float padding = 10.0f;
                        minX -= padding; maxX += padding;
                        minY -= padding; maxY += padding;
                        
                        // Find closest point on the bounding box to mouse cursor
                        float closestX = std::max(minX, std::min(maxX, mouse.x));
                        float closestY = std::max(minY, std::min(maxY, mouse.y));
                        partpos = { closestX, closestY };
                    }
                }
            }
            else {
                // Use selected body part for silent aim
                roblox::instance selectedPart = part;
                
                switch (globals::combat::silentaimpart) {
                    case 0: // Head
                        selectedPart = globals::instances::cachedtarget.head;
                        break;
                    case 1: // Upper Torso
                        selectedPart = globals::instances::cachedtarget.uppertorso;
                        break;
                    case 2: // Lower Torso
                        selectedPart = globals::instances::cachedtarget.lowertorso;
                        break;
                    case 3: // HumanoidRootPart
                        selectedPart = globals::instances::cachedtarget.hrp;
                        break;
                    case 4: // Left Hand
                        selectedPart = globals::instances::cachedtarget.lefthand;
                        break;
                    case 5: // Right Hand
                        selectedPart = globals::instances::cachedtarget.righthand;
                        break;
                    case 6: // Left Foot
                        selectedPart = globals::instances::cachedtarget.leftfoot;
                        break;
                    case 7: // Right Foot
                        selectedPart = globals::instances::cachedtarget.rightfoot;
                        break;
                    case 8: // Closest Part
                        // Choose closest body part to crosshair
                        {
                            struct Candidate { roblox::instance inst; };
                            std::vector<Candidate> candidates;
                            candidates.push_back({ globals::instances::cachedtarget.head });
                            candidates.push_back({ globals::instances::cachedtarget.uppertorso });
                            candidates.push_back({ globals::instances::cachedtarget.lowertorso });
                            candidates.push_back({ globals::instances::cachedtarget.hrp });
                            candidates.push_back({ globals::instances::cachedtarget.lefthand });
                            candidates.push_back({ globals::instances::cachedtarget.righthand });
                            candidates.push_back({ globals::instances::cachedtarget.leftfoot });
                            candidates.push_back({ globals::instances::cachedtarget.rightfoot });
                            POINT p; HWND rw = FindWindowA(nullptr, "Roblox"); if (rw) { GetCursorPos(&p); ScreenToClient(rw, &p); }
                            math::Vector2 mouse = { (float)p.x, (float)p.y };
                            float bestDist = 9e18f;
                            for (auto& c : candidates) {
                                if (!is_valid_address(c.inst.address)) continue;
                                math::Vector2 scr = roblox::worldtoscreen(c.inst.get_pos());
                                float dx, dy, d2;
                                
                                if (scr.x == -1.0f || scr.y == -1.0f) {
                                    // Use a large distance to prioritize on-screen parts
                                    scr = { 999999.0f, 999999.0f };
                                    dx = mouse.x - scr.x; dy = mouse.y - scr.y; d2 = dx * dx + dy * dy;
                                } else {
                                    dx = mouse.x - scr.x; dy = mouse.y - scr.y; d2 = dx * dx + dy * dy;
                                }
                                
                                if (d2 < bestDist) { bestDist = d2; selectedPart = c.inst; }
                            }
                        }
                        break;
                    case 9: // Random Part
                        {
                            std::vector<roblox::instance> candidates = {
                                globals::instances::cachedtarget.head,
                                globals::instances::cachedtarget.uppertorso,
                                globals::instances::cachedtarget.lowertorso,
                                globals::instances::cachedtarget.hrp,
                                globals::instances::cachedtarget.lefthand,
                                globals::instances::cachedtarget.righthand,
                                globals::instances::cachedtarget.leftfoot,
                                globals::instances::cachedtarget.rightfoot
                            };
                            // Filter out invalid candidates
                            std::vector<roblox::instance> validCandidates;
                            for (auto& c : candidates) {
                                if (is_valid_address(c.address)) {
                                    validCandidates.push_back(c);
                                }
                            }
                            if (!validCandidates.empty()) {
                                int randomIndex = rand() % validCandidates.size();
                                selectedPart = validCandidates[randomIndex];
                            }
                        }
                        break;
                }
                
                // Use the selected part
                if (is_valid_address(selectedPart.address)) {
                    part = selectedPart;
                }
                
                math::Vector3 part3d = part.get_pos();
                
                // Apply prediction for silent aim
                math::Vector3 predictedPos = part3d;
                if (globals::combat::silentpredictions) {
                    math::Vector3 velocity = part.get_velocity();
                    math::Vector3 veloVector = velocity / math::Vector3(globals::combat::silentpredictionsx, globals::combat::silentpredictionsy, globals::combat::silentpredictionsx);
                    predictedPos = part3d + veloVector;
                }
                
                partpos = roblox::worldtoscreen(predictedPos);
            }
            
            // Ensure partpos is valid before proceeding
            if (partpos.x == -1.0f || partpos.y == -1.0f) {
                // Fallback: use target's HRP position if partpos is invalid
                math::Vector3 fallbackPos = globals::instances::cachedtarget.hrp.get_pos();
                if (globals::combat::silentpredictions) {
                    math::Vector3 velocity = globals::instances::cachedtarget.hrp.get_velocity();
                    math::Vector3 veloVector = velocity / math::Vector3(globals::combat::silentpredictionsx, globals::combat::silentpredictionsy, globals::combat::silentpredictionsx);
                    fallbackPos = fallbackPos + veloVector;
                }
                partpos = roblox::worldtoscreen(fallbackPos);
            }
            
            // Additional validation: ensure partpos is within screen bounds
            math::Vector2 dimensions = globals::instances::visualengine.get_dimensions();
            if (partpos.x < 0 || partpos.x > dimensions.x || partpos.y < 0 || partpos.y > dimensions.y) {
                // If partpos is outside screen bounds, clamp it to screen edges
                partpos.x = std::max(0.0f, std::min(partpos.x, dimensions.x));
                partpos.y = std::max(0.0f, std::min(partpos.y, dimensions.y));
            }
            
            // Final validation: ensure we have a valid part position
            if (partpos.x == -1.0f || partpos.y == -1.0f || 
                std::isnan(partpos.x) || std::isnan(partpos.y) ||
                std::isinf(partpos.x) || std::isinf(partpos.y)) {
                // Last resort: use screen center
                partpos = { dimensions.x / 2.0f, dimensions.y / 2.0f };
            }
            
            // If using closest point mode, partpos is already set above
            if (globals::combat::closestpartsilent != 2) {
                // partpos is already set above for the selected body part
            }

            
            
            // Health check - MUST run right before mouse lock to break it immediately
            // Read fresh health every frame to catch real-time changes
            if ((*globals::combat::flags)[3] != 0 && is_valid_address(target.main.address)) {
                int targetHealth = 0;
                // Try reading from humanoid first (most reliable)
                if (target.humanoid.address) {
                    targetHealth = target.humanoid.read_health();
                }
                // Fallback: try reading from BodyEffects for custom models
                if (targetHealth == 0) {
                    auto bodyEffects = target.instance.findfirstchild("BodyEffects");
                    if (bodyEffects.address) {
                        auto health = bodyEffects.findfirstchild("Health");
                        if (health.address) {
                            targetHealth = static_cast<int>(health.read_double_value());
                        }
                    }
                }
                // Fallback: use cached health if available
                if (targetHealth == 0) {
                    targetHealth = target.health;
                }
                // Unlock immediately if health is at or below threshold - BREAKS MOUSE LOCK
                if (targetHealth > 0 && targetHealth <= static_cast<int>(globals::combat::healththreshhold)) {
                    foundTarget = false;
                    target = {};
                    globals::instances::cachedtarget = {};
                    dataReady = false;
                    // Don't auto-switch, wait before checking again
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue; // Immediately break mouse lock and skip aiming
                }
            }

            globals::instances::cachedlasttarget = target;

            POINT cursorPoint;
            GetCursorPos(&cursorPoint);
            ScreenToClient(robloxWindow, &cursorPoint);

            cachedPositionX = static_cast<uint64_t>(cursorPoint.x);
            cachedPositionY = static_cast<uint64_t>(dimensions.y - std::abs(dimensions.y - (cursorPoint.y)) - 58);
            dataReady = true;
        }
        else {
            dataReady = false;
                   }
    }
}

void hooks::silentrun2() {
    roblox::instance mouseServiceInstance;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(-100.0, 100.0);

    while (true) {
        if (globals::handlingtp) {
            continue;
        }
        if (!shouldSilentAimBeActive()) {
            if (globals::instances::cachedtarget.head.address != 0) targetNeedsReset = true;
            continue;
        }

        if (globals::instances::cachedtarget.head.address != 0 && dataReady) {
            if (globals::combat::spoofmouse) {
                globals::instances::aim.setFramePosX(cachedPositionX);
                globals::instances::aim.setFramePosY(cachedPositionY);
                mouseServiceInstance.initialize_mouse_service(globals::instances::mouseservice);
                mouseServiceInstance.write_mouse_position(globals::instances::mouseservice, partpos.x, partpos.y);
            }
            else {
                mouseServiceInstance.initialize_mouse_service(globals::instances::mouseservice);
                mouseServiceInstance.write_mouse_position(globals::instances::mouseservice, partpos.x, partpos.y);
            }
        }
    }
}

void hooks::silent() {
    std::thread primaryThread(&hooks::silentrun);
    std::thread secondaryThread(&hooks::silentrun2);
    primaryThread.detach();
    secondaryThread.detach();
}
