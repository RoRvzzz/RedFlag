#include <thread>
#include <chrono>
#include <thread>
#include <Windows.h>
#include <mmsystem.h>
#include <vector>
#include <memory>
#include <thread>
#include <iostream>
#include "../../combat.h"
#include "../../../hook.h"
int downcount;

void hooks::rapidfire() {
    while (true) {
      if (!globals::focused || !globals::misc::rapidfire) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        continue;
    }

       if(globals::focused && globals::misc::rapidfire){
        // Apply to tools in Backpack
        {
            roblox::instance backpack = globals::instances::lp.main.findfirstchild("Backpack");
            for (roblox::instance tool : backpack.get_children()) {
                if (tool.get_class_name() != "Tool" && tool.get_name() != "Combat") continue;
                roblox::instance shooting = tool.findfirstchild("ShootingCooldown");
                roblox::instance tyolerance = tool.findfirstchild("ToleranceCooldown");
                if (shooting.address) shooting.write_double_value(1e-9);
                if (tyolerance.address) tyolerance.write_double_value(1e-9);
            }
        }

        // Also apply to currently equipped tool in character
        {
            roblox::instance tool = globals::instances::lp.instance.read_service("Tool");
            if (tool.address) {
                roblox::instance shooting = tool.findfirstchild("ShootingCooldown");
                roblox::instance tyolerance = tool.findfirstchild("ToleranceCooldown");
                if (shooting.address) shooting.write_double_value(1e-9);
                if (tyolerance.address) tyolerance.write_double_value(1e-9);
            }
        }
       }
    }

}