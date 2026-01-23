#include "../thread.h"


void utils::antistomp() {
    while (true) {
        Sleep(20);

        if (!globals::antistomp) continue;

        auto players = globals::workspace.FindFirstChild("Players");
        if (!players.isValid()) continue;

        auto local = players.FindFirstChild(globals::players.GetLocalPlayer().GetName().c_str());
        if (!local.isValid()) continue;

        auto bodyeffects = local.FindFirstChild("BodyEffects");
        if (!bodyeffects.isValid()) continue;

        auto humanoid = local.FindFirstChild("Humanoid");
        auto ko = bodyeffects.FindFirstChild("K.O");

        if (ko.getBoolFromValue() || humanoid.GetHealth() == 0) {
            bodyeffects.FindFirstChild("Dead").SetBoolFromValue(true);
            bodyeffects.FindFirstChild("SDeath").SetBoolFromValue(false);
            ko.SetBoolFromValue(true);
            humanoid.write_health(0);
        }
    }
}