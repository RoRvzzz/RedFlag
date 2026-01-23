#include "../../hook.h"
#include "../../../util/console/console.h"
#include "../misc.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <random>
#include <Windows.h>

void hooks::rapidfire() {
    while (true) {
        if (!globals::firstreceived) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (globals::misc::rapidfire) {
            CMisc::rapid_fire();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
