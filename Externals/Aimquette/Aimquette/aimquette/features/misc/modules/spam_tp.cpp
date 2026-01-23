#include "../../hook.h"
#include "../../../util/console/console.h"
#include "../../../util/classes/math/math.h"
#include <chrono>
#include <thread>
#include <iostream>

void hooks::spam_tp() {
    while (true) {
        if (!globals::firstreceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (globals::misc::spam_tp) {
            // Use the same target that aimbot is locked onto
            if (!globals::combat::aimbot_locked || !is_valid_address(globals::combat::aimbot_current_target.main.address)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            roblox::player target = globals::combat::aimbot_current_target;
            
            // Get target's HumanoidRootPart
            roblox::instance targetHRP = target.hrp;
            if (!is_valid_address(targetHRP.address)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            // Get local player's HumanoidRootPart
            roblox::instance localHRP = globals::instances::lp.hrp;
            if (!is_valid_address(localHRP.address)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            try {
                // Get target position
                Vector3 targetPos = targetHRP.get_pos();
                
                // Teleport to target position
                localHRP.write_position(targetPos);
            } catch (...) {
                // Position update failed, skip this iteration
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Sleep for 1ms between teleports for maximum spam
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
