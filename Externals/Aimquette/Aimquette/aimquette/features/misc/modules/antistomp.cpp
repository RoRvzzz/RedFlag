#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>

void hooks::antistomp() {
    while (true) {
        if (!globals::firstreceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
        }

        if (globals::misc::antistomp) {
            try {
                auto character = globals::instances::lp.main;
                if (!is_valid_address(character.address)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                    continue;
                }

                                auto bodyeffects = character.findfirstchild("BodyEffects");
                if (is_valid_address(bodyeffects.address)) {
                    auto ko = bodyeffects.findfirstchild("K.O");
                    if (is_valid_address(ko.address)) {
                        bool isKnocked = ko.read_bool_value();
                        if (isKnocked) {
                            auto humanoid = character.findfirstchild("Humanoid");
                            if (is_valid_address(humanoid.address)) {
                                humanoid.write_health(0.0f);
                            }
                        }
                    }
                }
            }
            catch (...) {
                // Silently handle exceptions to prevent crashes
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}
