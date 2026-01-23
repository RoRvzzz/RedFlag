#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

void hooks::cframe() {
    static bool was_enabled = false;
    static float original_walkspeed = 16.0f; // Default Roblox walkspeed
    static bool has_stored_original = false;
    
    while (true) {
        if (!globals::firstreceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        bool currently_enabled = false;
        
        if (globals::misc::cframe) {
            globals::misc::cframekeybind.update();

            switch (globals::misc::cframekeybind.type) {
            case keybind::TOGGLE:
                if (globals::misc::cframekeybind.enabled)
                    currently_enabled = true;
                break;
            case keybind::HOLD:
                currently_enabled = globals::misc::cframekeybind.enabled;
                break;
            case keybind::ALWAYS:
                currently_enabled = true;
                break;
            }

            // If cframe was just disabled, restore original walkspeed
            if (was_enabled && !currently_enabled && has_stored_original) {
                auto humanoid = globals::instances::lp.humanoid;
                if (is_valid_address(humanoid.address)) {
                    humanoid.write_walkspeed(original_walkspeed);
                }
            }

            if (currently_enabled) {
                // Apply cframe movement
                auto humanoid = globals::instances::lp.humanoid;
                if (is_valid_address(humanoid.address)) {
                    // Store original walkspeed when first enabling
                    if (!has_stored_original) {
                        original_walkspeed = humanoid.read_walkspeed();
                        if (original_walkspeed <= 0 || original_walkspeed > 200) {
                            original_walkspeed = 16.0f; // Fallback to default if invalid
                        }
                        has_stored_original = true;
                    }
                    
                    humanoid.write_walkspeed(static_cast<float>(globals::misc::cframespeed));
                }
            }
        }
        else {
            // If cframe feature is completely disabled, restore original and reset
            if (was_enabled && has_stored_original) {
                auto humanoid = globals::instances::lp.humanoid;
                if (is_valid_address(humanoid.address)) {
                    humanoid.write_walkspeed(original_walkspeed);
                }
                has_stored_original = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        
        was_enabled = currently_enabled;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
