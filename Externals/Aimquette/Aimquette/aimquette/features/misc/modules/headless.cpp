#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <Windows.h>

void hooks::headless() {
    while (true) {
        if (!globals::focused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        if (!globals::misc::headless) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto localPlayer = globals::instances::localplayer;
        if (!localPlayer.address) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto character = localPlayer.model_instance();
        if (!character.address) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto head = character.findfirstchild("Head");
        if (!head.address) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        head.write_part_size(Vector3(0.f, 0.f, 0.f));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}




