#include "../thread.h"


void utils::walkspeed() {
    DWORD last_check = 0;
    const DWORD check_interval = 1;
    static int initial_speed = -1;
    while (true) {
        DWORD current_time = GetTickCount();
        globals::WalkSpeed_Bind.update();
        if ( !globals::speed_enabled || !globals::WalkSpeed_Bind.enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }


        if (globals::speed_enabled) {
            if (current_time - last_check >= check_interval) {
                auto goof = RBX::read_walkspeed();
                if (initial_speed == -1 && goof != 0) {
                    initial_speed = goof;
                }
                if (globals::walkspeed_amount && globals::WalkSpeed_Bind.enabled) {
                    RBX::walkspeedloop(globals::walkspeed_amount);

                }
                else if (initial_speed != -1 && goof == globals::walkspeed_amount) {
                    RBX::walkspeedloop(initial_speed);
                }
            }
            Sleep(1);
        }
        else {
            Sleep(50);
        }
    }
}