#include "../combat.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <string>
#include <cctype>
#include "../../hook.h"
#include "triggerbot.h"
#include "../../../util/classes/classes.h"
#include "../../wallcheck/wallcheck.h"

using namespace roblox;
extern uintptr_t base_address;

static float TriggerBotDistance2D(Vector2 first, Vector2 sec) {
    return sqrt(pow(first.x - sec.x, 2) + pow(first.y - sec.y, 2));
}
static float TriggerBotDistance3D(Vector3 first, Vector3 sec) {
    return sqrt(pow(first.x - sec.x, 2) + pow(first.y - sec.y, 2) + pow(first.z - sec.z, 2));
}

constexpr auto SLEEP_DURATION = std::chrono::milliseconds(1);

roblox::player TriggerBotClosest()
{
    // Add bots to the player list
    std::vector<roblox::player> bots = GetBots(base_address);
    std::vector<roblox::player> all_players = globals::instances::cachedplayers;
    all_players.insert(all_players.end(), bots.begin(), bots.end());
    const auto& players = all_players;
    
    roblox::player closest = {};
    float closestdistance = 9e9;
    POINT point;
    GetCursorPos(&point);
    ScreenToClient(FindWindowA(0, "Roblox"), &point);
    Vector2 curpos = { static_cast<float>(point.x), static_cast<float>(point.y) };

    for (auto player : players) {
        if (!is_valid_address(player.head.address))continue;
        roblox::instance head = player.head;
        if (head.address == 0) continue;
        if (player.name == globals::instances::lp.name)continue;
        
        // Team check - skip teammates if teamcheck is enabled
        if (globals::combat::teamcheck) {
            // Get local player's team from explorer
            roblox::instance localPlayerTeam = globals::instances::lp.team;
            // Compare teams - skip if same team
            if (localPlayerTeam.address != 0 && player.team.address != 0) {
                if (localPlayerTeam.address == player.team.address) {
                    continue; // Same team, skip
                }
            }
        }

        Vector2 screencoords = roblox::worldtoscreen(head.get_pos());
        if (screencoords.x == -1.0f || screencoords.y == -1.0f) continue;
        float distance = TriggerBotDistance2D(curpos, screencoords);
        if (distance < closestdistance) {
            closestdistance = distance;
            closest = player;
        }
    }
    return closest;
}




void hooks::triggerbot() {
    auto lastFireTime = std::chrono::steady_clock::now();

    while (true) {
        std::this_thread::sleep_for(SLEEP_DURATION);
     
        HWND hwnd = FindWindowA(0, "Roblox");
        if (hwnd == nullptr || !IsWindow(hwnd)) continue;

        globals::combat::triggerbotkeybind.update();

        if (!globals::focused || !globals::combat::triggerbotkeybind.enabled || !globals::combat::triggerbot) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        auto player = TriggerBotClosest();
        // Check if player has a valid humanoid (more reliable than checking individual parts)
        if (!player.head.address || !player.humanoid.address) continue;
        
        // Knocked check - unlock triggerbot if target is dead or knocked
        // Use player.bodyeffects if available (works for both regular players and custom models)
        auto bodyEffects = player.bodyeffects;
        if (!bodyEffects.address) {
            // Fallback: try to find BodyEffects from instance (for custom models)
            bodyEffects = player.instance.findfirstchild("BodyEffects");
        }
        if (bodyEffects.address) {
            try {
                auto deadFlag = bodyEffects.findfirstchild("Dead");
                if (deadFlag.address && deadFlag.read_bool_value()) {
                    continue;
                }
                if (globals::combat::knockcheck) {
                    auto koFlag = bodyEffects.findfirstchild("K.O");
                    if (koFlag.address && koFlag.read_bool_value()) {
                        continue;
                    }
                }
            } catch (...) { }
        }
    
        RECT windowRect;
        if (!GetWindowRect(hwnd, &windowRect)) continue;

        POINT point;
        if (!GetCursorPos(&point)) continue;

        if (point.x < windowRect.left || point.x > windowRect.right ||
            point.y < windowRect.top || point.y > windowRect.bottom) continue;

        if (!ScreenToClient(hwnd, &point)) continue;

        Vector2 curpos = { static_cast<float>(point.x), static_cast<float>(point.y) };
        bool foundTarget = false;
    
        std::unordered_map<std::string, Vector2> screen_positions;
        auto getPos = [&](auto& part, const std::string& name) {
            if (!part.address) return;
            Vector3 pos = part.get_pos();
            Vector2 screen_pos = roblox::worldtoscreen(pos);
            if (screen_pos.x >= 0.0f && screen_pos.y >= 0.0f &&
                screen_pos.x <= (windowRect.right - windowRect.left) &&
                screen_pos.y <= (windowRect.bottom - windowRect.top)) {
                screen_positions[name] = screen_pos;
            }
            };

        getPos(player.head, "Head");
        getPos(player.hrp, "HumanoidRootPart");
        getPos(player.leftupperarm, "LeftUpperArm");
        getPos(player.rightupperarm, "RightUpperArm");
        getPos(player.leftfoot, "LeftFoot");
        getPos(player.rightfoot, "RightFoot");
        getPos(player.leftlowerarm, "LeftLowerArm");
        getPos(player.rightlowerarm, "RightLowerArm");
        getPos(player.lefthand, "LeftHand");
        getPos(player.righthand, "RightHand");

        // Check all body parts for faster detection - shoot immediately when any part is under cursor
        for (const auto& part : screen_positions) {
            float distance = TriggerBotDistance2D(curpos, part.second);
            float range = TriggerBotDistance3D(globals::instances::lp.hrp.get_pos(), player.hrp.get_pos());
            // Use player.bodyeffects if available (works for both regular players and custom models)
            auto bodyEffects = player.bodyeffects;
            if (!bodyEffects.address) {
                // Fallback: try to find BodyEffects from instance (for custom models)
                bodyEffects = player.instance.findfirstchild("BodyEffects");
            }
            bool isKnocked = false;
            if (bodyEffects.address) {
                try {
                    auto koFlag = bodyEffects.findfirstchild("K.O");
                    isKnocked = (koFlag.address && koFlag.read_bool_value());
                } catch (...) { }
            }
            
            // Wall check (from seizure)
            bool canShoot = true;
            if ((*globals::combat::flags)[4] != 0 && !wallcheck::can_see(player.head.get_pos(), globals::instances::camera.getPos())) {
                canShoot = false; // Wall detected, don't shoot
            }

            // Knife check - don't shoot if local player has knife equipped
            bool hasKnife = false;
            if (globals::combat::knifecheck) {
                std::string toolName = globals::instances::lp.toolname;
                std::transform(toolName.begin(), toolName.end(), toolName.begin(), ::tolower);
                if (toolName.find("knife") != std::string::npos || 
                    toolName.find("katana") != std::string::npos ||
                    toolName.find("sword") != std::string::npos ||
                    toolName.find("blade") != std::string::npos) {
                    hasKnife = true;
                }
            }
            
            if (distance <= 10 && !isKnocked && canShoot && !hasKnife) {
                if (globals::combat::triggerbotrange && range <= globals::combat::triggerbotrangevalue) {
                    foundTarget = true;
                    break; // Shoot immediately, don't check other parts
                }
                else if (!globals::combat::triggerbotrange) {
                    foundTarget = true;
                    break; // Shoot immediately, don't check other parts
                }
            }
        }

        // Shoot immediately when target is found, no delay
        if (foundTarget) {
            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dx = 0;
            input.mi.dy = 0;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            input.mi.mouseData = 0;
            input.mi.dwExtraInfo = 0;
            input.mi.time = 0;

            SendInput(1, &input, sizeof(INPUT));
            // Release delay - wait before releasing mouse button
            int release_delay_ms = static_cast<int>(globals::combat::releasedelay);
            if (release_delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(release_delay_ms));
            } else {
                // Minimal delay for proper click if release delay is 0
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));

            lastFireTime = std::chrono::steady_clock::now();
        }
    }
}