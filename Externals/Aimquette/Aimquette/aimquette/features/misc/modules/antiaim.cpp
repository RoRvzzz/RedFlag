#include "../../hook.h"
#include "../../../util/console/console.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>

int random_ints(int min, int max) {
    return min + rand() % (max - min + 1);
}

void hooks::antiaim() {
    srand(time(NULL));
    
    while (true) {
        if (!globals::firstreceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (globals::misc::antiaim) {
            try {
                auto hrp = globals::instances::lp.hrp;
                if (!is_valid_address(hrp.address)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    continue;
                }

                Vector3 oldvel = hrp.get_pos();
                Vector3 newvec(
                    oldvel.x + random_ints(0, 5), 
                    oldvel.y + random_ints(0, 5), 
                    oldvel.z + random_ints(0, 5)
                );

                hrp.write_position(newvec);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                hrp.write_position(oldvel);
            }
            catch (const std::exception& e) {
                // Silently handle exceptions to prevent crashes
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
