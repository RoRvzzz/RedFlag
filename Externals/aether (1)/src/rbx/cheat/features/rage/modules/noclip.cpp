#include "../thread.h"

void utils::noclip() {
    if (globals::noclip) {
        globals::noclipbind.update();
    }

    // orrangized


    if (globals::noclip) {
        auto localPlayerName = globals::players.GetLocalPlayer().GetModelInstance().GetName();
        auto localPlayer = globals::players.GetLocalPlayer().GetModelInstance();
        if (globals::noclipbind.enabled) {
            for (RBX::Instance dance : localPlayer.GetChildren()) {
                auto goozz = dance.GetParent();
                if (dance.GetClass () == "MeshPart" || dance.GetClass() == "Part" || dance.GetClass() == "BasePart") {
                    if (!globals::noclip || !globals::noclipbind.enabled) {
                        if (dance.GetName() == "LeftFoot" || dance.GetName() == "RightFoot" || dance.GetName() == "RightLowerLeg" || dance.GetName() == "LeftLowerLeg") {
                            dance.SetCanCollide(true);
                            //    continue;
                        }
                        else {
                            dance.SetCanCollide(true);
                        }
                       // Sleep(30);
                    }
                    else if (globals::noclip && globals::noclipbind.enabled) {
                        dance.SetCanCollide(false);
                        Sleep(30);
                    }
                }

            }
        }
    }
    else {
        if (!globals::noclip || !globals::noclipbind.enabled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}