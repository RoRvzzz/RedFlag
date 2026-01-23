#include "../../hook.h"
#include "../../../util/console/console.h"
#include "../../wallcheck/wallcheck.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

static Vector3 lookvec(const Matrix3& rotationMatrix) {
	return rotationMatrix.getColumn(2);
}

static Vector3 rightvec(const Matrix3& rotationMatrix) {
	return rotationMatrix.getColumn(0);
}

void hooks::flight() {
	using clock = std::chrono::high_resolution_clock;
	
	// Performance optimization: Use proper frame rate limiting
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER lastTime;
	static bool timeInitialized = false;
	static const double targetFrameTime = 1.0 / 60.0; // 60 FPS instead of 165

	if (!timeInitialized) {
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&lastTime);
		timeBeginPeriod(1);
		timeInitialized = true;
	}

	// Flight state tracking
	static bool was_flight_enabled = false;
	static int consecutive_failures = 0;
	static const int max_failures = 10;

	while (true) {
		try {
			auto start_time = clock::now();
			
			// Check if flight is enabled
			bool flight_enabled = globals::misc::flight;
			
			if (flight_enabled) {
				// Reset failure counter when flight is enabled
				if (!was_flight_enabled) {
					consecutive_failures = 0;
					was_flight_enabled = true;
				}

				globals::misc::flightkeybind.update();

				if (globals::misc::flightkeybind.enabled) {
					// Validate all required instances before proceeding
					bool instances_valid = true;
					
					// Check humanoid
					if (!is_valid_address(globals::instances::lp.humanoid.address)) {
						instances_valid = false;
					}
					
					// Check hrp
					if (!is_valid_address(globals::instances::lp.hrp.address)) {
						instances_valid = false;
					}
					
					// Check camera
					if (!is_valid_address(globals::instances::camera.address)) {
						instances_valid = false;
					}

					if (!instances_valid) {
						consecutive_failures++;
						if (consecutive_failures > max_failures) {
							// Too many failures, wait longer before retrying
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							consecutive_failures = 0;
						} else {
							std::this_thread::sleep_for(std::chrono::milliseconds(16));
						}
						continue;
					}

					// Reset failure counter on successful validation
					consecutive_failures = 0;

					auto humanoid = globals::instances::lp.humanoid;
					auto hrp = globals::instances::lp.hrp;
					roblox::camera camera = globals::instances::camera;
					
					// Validate camera rotation matrix with more lenient checks
					Matrix3 rotation_matrix;
					try {
						rotation_matrix = camera.getRot();
					} catch (...) {
						// Camera rotation failed, skip this frame
						std::this_thread::sleep_for(std::chrono::milliseconds(16));
						continue;
					}

					// More lenient matrix validation
					if (rotation_matrix.getColumn(0).magnitude() < 0.01f || rotation_matrix.getColumn(2).magnitude() < 0.01f) {
						std::this_thread::sleep_for(std::chrono::milliseconds(16));
						continue;
					}

					Vector3 look_vector = lookvec(rotation_matrix);
					Vector3 right_vector = rightvec(rotation_matrix);
					bool aircheck = false;

					switch (globals::misc::flighttype) {
					case 0: { // CFrame
						Vector3 direction = { 0.0f, 0.0f, 0.0f };
						
						// Check WASD keys
						if (GetAsyncKeyState('W') & 0x8000) {
							direction = direction - look_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('S') & 0x8000) {
							direction = direction + look_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('A') & 0x8000) {
							direction = direction - right_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('D') & 0x8000) {
							direction = direction + right_vector;
							aircheck = true;
						}

						// Normalize direction if it has magnitude
						if (direction.magnitude() > 0.01f) {
							direction = direction.normalize();
						}

						// Apply flight
						if (aircheck) {
							try {
								Vector3 current_pos = hrp.get_pos();
								Vector3 new_pos = current_pos + (direction * (globals::misc::flightvalue * 0.016f));
								hrp.write_position(new_pos);
								hrp.write_velocity({ 0.0f, 0.0f, 0.0f });
							} catch (...) {
								// Position update failed, skip this frame
								std::this_thread::sleep_for(std::chrono::milliseconds(16));
								continue;
							}
						} else {
							try {
								hrp.write_velocity({ 0.0f, 0.0f, 0.0f });
							} catch (...) {
								// Velocity update failed, skip this frame
								std::this_thread::sleep_for(std::chrono::milliseconds(16));
								continue;
							}
						}
						break;
					}
					case 1: { // Velocity
						Vector3 direction = { 0.0f, 0.0f, 0.0f };
						
						// Check WASD keys
						if (GetAsyncKeyState('W') & 0x8000) {
							direction = direction - look_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('S') & 0x8000) {
							direction = direction + look_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('A') & 0x8000) {
							direction = direction - right_vector;
							aircheck = true;
						}
						if (GetAsyncKeyState('D') & 0x8000) {
							direction = direction + right_vector;
							aircheck = true;
						}

						// Normalize direction if it has magnitude
						if (direction.magnitude() > 0.01f) {
							direction = direction.normalize();
						}

						// Apply flight using velocity
						if (aircheck) {
							try {
								hrp.write_velocity(direction * globals::misc::flightvalue);
							} catch (...) {
								// Velocity update failed, skip this frame
								std::this_thread::sleep_for(std::chrono::milliseconds(16));
								continue;
							}
						} else {
							try {
								hrp.write_velocity({ 0.0f, 0.0f, 0.0f });
							} catch (...) {
								// Velocity update failed, skip this frame
								std::this_thread::sleep_for(std::chrono::milliseconds(16));
								continue;
							}
						}
						break;
					}
					}
				}
			} else {
				was_flight_enabled = false;
				consecutive_failures = 0;
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
			
			// Optimized frame rate limiting
			LARGE_INTEGER currentTime;
			QueryPerformanceCounter(&currentTime);
			double elapsedSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

			if (elapsedSeconds < targetFrameTime) {
				DWORD sleepMilliseconds = static_cast<DWORD>((targetFrameTime - elapsedSeconds) * 1000.0);
				if (sleepMilliseconds > 0) {
					Sleep(sleepMilliseconds);
				}
			}

			lastTime = currentTime;
		}
		catch (const std::exception& e) {
			// Log specific exceptions for debugging
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
		catch (...) {
			// Catch any other exceptions and continue
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
}