#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

void hooks::jumppower() {
	static bool was_enabled = false;
	static float original_jumppower = 50.0f; // Default Roblox jumppower
	static bool has_stored_original = false;
	
	while(true){
		if (!globals::focused) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			continue;
		}
		
		globals::misc::jumppowerkeybind.update();
		
		bool currently_enabled = globals::misc::jumppowerkeybind.enabled && globals::misc::jumppower;
		
		// If jumppower was just disabled, restore original value
		if (was_enabled && !currently_enabled && has_stored_original) {
			auto humanoid = globals::instances::lp.humanoid;
			if (is_valid_address(humanoid.address)) {
				humanoid.write_jumppower(original_jumppower);
			}
		}
		
		was_enabled = currently_enabled;
		
		if (currently_enabled) {
			auto humanoid = globals::instances::lp.humanoid;
			if (!is_valid_address(humanoid.address)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			
			// Store original jumppower when first enabling
			if (!has_stored_original) {
				original_jumppower = humanoid.read_jumppower();
				if (original_jumppower <= 0 || original_jumppower > 200) {
					original_jumppower = 50.0f; // Fallback to default if invalid
				}
				has_stored_original = true;
			}
			
			if (humanoid.read_jumppower() != globals::misc::jumpowervalue) {
				humanoid.write_jumppower(globals::misc::jumpowervalue);
			}
		}
		else {
			// Reset stored original when feature is completely disabled
			if (!globals::misc::jumppower) {
				has_stored_original = false;
			}
		}

	static LARGE_INTEGER frequency;
	static LARGE_INTEGER lastTime;
	static bool timeInitialized = false;

	if (!timeInitialized) {
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&lastTime);
		timeBeginPeriod(1);
		timeInitialized = true;
	}

	const double targetFrameTime = 1.0 / 45;

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	double elapsedSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

	if (elapsedSeconds < targetFrameTime) {
		DWORD sleepMilliseconds = static_cast<DWORD>((targetFrameTime - elapsedSeconds) * 1000.0);
		if (sleepMilliseconds > 0) {
			Sleep(sleepMilliseconds);
		}
	}

	do {
		QueryPerformanceCounter(&currentTime);
		elapsedSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
	} while (elapsedSeconds < targetFrameTime);

	lastTime = currentTime;

}
}