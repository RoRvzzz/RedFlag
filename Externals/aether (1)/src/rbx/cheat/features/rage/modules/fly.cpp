#include "../thread.h"
#include <chrono>
#include "../variables.h"
void utils::fly() {
    bool air_check = false;

    while (true) {
        globals::fly_bind.update();
        if (!globals::fly || !globals::fly_bind.enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        RBX::Instance character = globals::players.GetLocalPlayer().GetModelInstance();
        RBX::Instance hrp = character.FindFirstChild("HumanoidRootPart");

        if (!hrp.isValid()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        RBX::Matrix3x3 rotation_matrix = globals::camera.GetCameraRotation();
        RBX::Vector3 look_vector = lookvec(rotation_matrix);
        RBX::Vector3 right_vector = rightvec(rotation_matrix);
        RBX::Vector3 direction = { 0.0f, 0.0f, 0.0f };

        if (GetAsyncKeyState('W') & 0x8000) {
            direction = direction - look_vector;
            air_check = true;
        }
        if (GetAsyncKeyState('S') & 0x8000) {
            direction = direction + look_vector;
            air_check = true;
        }
        if (GetAsyncKeyState('A') & 0x8000) {
            direction = direction - right_vector;
            air_check = true;
        }
        if (GetAsyncKeyState('D') & 0x8000) {
            direction = direction + right_vector;
            air_check = true;
        }

        if (direction.magnitude() > 0) {
            direction = direction.normalize();
        }

        if (!air_check) {
            hrp.SetPartVelocity({ 0.0f, 0.0f, 0.0f });
        }
        else {
            hrp.SetPartVelocity(direction * globals::fly_speed * 10);
        }

        air_check = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void utils::desync_loop_thread() {
    while (true) {
        RageGlobals::desync_key.update();
        if (RageGlobals::desync_enabled && RageGlobals::desync_key.enabled) {
            RBX::Instance character = globals::players.GetLocalPlayer().GetModelInstance();
            RBX::Instance hrp = character.FindFirstChild("HumanoidRootPart");
            if (!hrp.isValid()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            RBX::Vector3 current_velocity = hrp.GetVelocity();
            float factor = RageGlobals::desync_jitter ? ((rand() % 2 == 0) ? -1.0f : 1.0f) : 1.0f;
            RBX::Vector3 desync_velocity = current_velocity + RBX::Vector3(0.0f, factor * RageGlobals::desync_strength, 0.0f);
            hrp.SetPartVelocity(desync_velocity);
            std::this_thread::sleep_for(std::chrono::milliseconds((int)(RageGlobals::desync_delay * 1000)));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

