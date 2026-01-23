#include "../thread.h"

void utils::jump() {
    while (true) {
        RBX::Instance Humanoid = globals::localplayer.humanoid;

        if (!Humanoid.isValid()) {
            continue;
        }

        if (globals::WalkSpeed_Bind.update(), globals::WalkSpeed_Bind.enabled) {
            Humanoid.SetWalkSpeedCompensator(globals::walkspeed_amount);
            Humanoid.SetWalkSpeed(globals::walkspeed_amount);
        }

        if (globals::JumpPower_Bind.update(), globals::JumpPower_Bind.enabled) {
            Humanoid.SetJumpPower(globals::JumpPower);
        }

        if (globals::HipHeight_Bind.update(), globals::HipHeight_Bind.enabled) {
            Humanoid.SetHipHeight(globals::HipHeight);
        }

    
    }
}




