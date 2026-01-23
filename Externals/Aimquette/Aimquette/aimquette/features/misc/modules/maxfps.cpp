#include "../../hook.h"
#include "../../../util/globals.h"
#include "../../../util/offsets.h"
#include "../../../util/driver/driver.h"
#include <chrono>
#include <thread>

namespace hooks {
    void max_fps() {
        while (true) {
            if (!globals::firstreceived) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            static int last_max_fps = -1;
            
            // Only update if the value changed
            if (last_max_fps != globals::misc::max_fps) {
                last_max_fps = globals::misc::max_fps;
                
                // Read TaskScheduler pointer
                uintptr_t task_scheduler_ptr = read<uintptr_t>(base_address + offsets::TaskSchedulerPointer);
                
                if (is_valid_address(task_scheduler_ptr)) {
                    // Write max FPS to TaskScheduler
                    try {
                        write<float>(task_scheduler_ptr + offsets::TaskSchedulerMaxFPS, static_cast<float>(globals::misc::max_fps));
                    }
                    catch (...) {
                        // Ignore errors
                    }
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
