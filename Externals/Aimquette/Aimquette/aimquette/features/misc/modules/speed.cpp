#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

void hooks::speed() {
	static bool was_enabled = false;
	static float original_walkspeed = 16.0f; // Default Roblox walkspeed
	static bool has_stored_original = false;
	
	while (true) {
		if (!globals::firstreceived) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		
		globals::misc::speedkeybind.update();
		
		bool currently_enabled = globals::misc::speed && globals::misc::speedkeybind.enabled;
		
		// If speed was just disabled, restore original walkspeed
		if (was_enabled && !currently_enabled && has_stored_original && globals::misc::speedtype == 0) {
			auto humanoid = globals::instances::lp.humanoid;
			if (is_valid_address(humanoid.address)) {
				humanoid.write_walkspeed(original_walkspeed);
			}
		}
		
		was_enabled = currently_enabled;
		
		if (currently_enabled) {
			auto humanoid = globals::instances::lp.humanoid;
			auto hrp = globals::instances::lp.hrp;
			if (!is_valid_address(humanoid.address) || !is_valid_address(hrp.address)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			
			// Store original walkspeed when first enabling (WalkSpeed type only)
			if (!has_stored_original && globals::misc::speedtype == 0) {
				original_walkspeed = humanoid.read_walkspeed();
				if (original_walkspeed <= 0 || original_walkspeed > 200) {
					original_walkspeed = 16.0f; // Fallback to default if invalid
				}
				has_stored_original = true;
			}
			
			switch (globals::misc::speedtype) {
				case 0:
					if (humanoid.read_walkspeed() != globals::misc::speedvalue) {
						humanoid.write_walkspeed(globals::misc::speedvalue);
					}
					break;
				case 1: {
					auto dir = humanoid.get_move_dir();
					hrp.write_velocity(Vector3(
						dir.x * globals::misc::speedvalue,
						hrp.get_velocity().y,
						dir.z * globals::misc::speedvalue
					));
					break;
				}
				case 2: {
					auto dir2 = humanoid.get_move_dir();
					auto pos = hrp.get_pos();
					auto finalpos = Vector3(
						pos.x + dir2.x * globals::misc::speedvalue * 0.016f,
						pos.y,
						pos.z + dir2.z * globals::misc::speedvalue * 0.016f
					);
					hrp.write_position(finalpos);
					break;
				}
			}
		}
		else {
			// Reset stored original when feature is completely disabled
			if (!globals::misc::speed) {
				has_stored_original = false;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}