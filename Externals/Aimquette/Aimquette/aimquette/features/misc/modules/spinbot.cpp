#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <Windows.h>
#include "../../hook.h"

static float clamp_angle(float a) {
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

void hooks::spinbot() {
    static float spin_angle = 0.0f;
    
    while (true) {
        if (!globals::focused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            continue;
        }

        if (globals::misc::spin360) {
            globals::misc::spin360keybind.update();

            if (globals::misc::spin360keybind.enabled) {
                // Get local player's HRP
                auto hrp = globals::instances::lp.hrp;
                if (hrp.address && is_valid_address(hrp.address)) {
                    try {
                        // Calculate spin speed (degrees per frame)
                        // Map UI scale (1..10) to degrees per frame: 15..150 degrees per frame for very fast spin
                        float deg_per_frame = std::clamp(globals::misc::spin360speed, 1.0f, 10.0f) * 15.0f;
                        
                        // Add rotation to Y axis (spinning around vertical axis - makes player spin in place)
                        spin_angle += deg_per_frame;
                        if (spin_angle >= 360.0f) spin_angle -= 360.0f;
                        
                        // Get current rotation
                        Matrix3 current_rotation = hrp.read_part_cframe();
                        Vector3 euler = current_rotation.MatrixToEulerAngles();
                        
                        // Only rotate Y axis (yaw), preserve pitch and roll
                        Vector3 new_euler = euler;
                        new_euler.y = spin_angle;
                        
                        // Create new rotation matrix from euler angles
                        Matrix3 base_matrix;
                        Matrix3 new_rotation = base_matrix.EulerAnglesToMatrix(new_euler);
                        
                        // Write the new rotation to HRP multiple times to ensure it sticks
                        for (int i = 0; i < 5; i++) {
                            hrp.write_part_cframe(new_rotation);
                        }
                    } catch (...) {
                        // Skip if error occurs
                    }
                }
            } else {
                // Reset angle when key is released
                spin_angle = 0.0f;
            }
        } else {
            spin_angle = 0.0f;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Fast update rate for smooth spinning
    }
}



void hooks::hipheight() {}
