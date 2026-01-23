#include "../thread.h"
#include <chrono>

void utils::waypoint_loop() {
    static RBX::Instance lastHrp;

    globals::keybind_clear.update();
    globals::keybind_teleport.update();
    globals::keybind_set.update();

    RBX::Instance Character = globals::players.GetLocalPlayer().GetModelInstance();
    RBX::Instance Hrp = Character.FindFirstChild("HumanoidRootPart");

    if (!Hrp.isValid()) {
        lastHrp = RBX::Instance();
        return;
    }

    if (globals::tp_on_respawn && lastHrp.isValid() && Hrp != lastHrp) {
        if (globals::waypoint_enabled) {
            Hrp.SetPartPos(globals::waypoint);
        }
    }
    lastHrp = Hrp;

    if (globals::keybind_set.enabled) {
        globals::waypoint = Hrp.GetPosition();
        globals::waypoint_enabled = true;
    }

    if (globals::keybind_teleport.enabled) {
        if (globals::waypoint_enabled) {
            Hrp.SetPartPos(globals::waypoint);
        }
    }

    if (globals::keybind_clear.enabled) {
        globals::waypoint_enabled = false;
        globals::waypoint = { 0.0f, 0.0f, 0.0f };
    }
}
