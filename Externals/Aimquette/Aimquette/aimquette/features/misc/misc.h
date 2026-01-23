#pragma once

#include "../../util/globals.h"
#include "../../util/classes/classes.h"
#include "../../util/offsets.h"
#include "../../util/driver/driver.h"

namespace hooks {
    void desync();
    void spam_tp();
}

class CMisc {
public:
    static void rapid_fire() {
        if (!globals::misc::rapidfire) return;
        
        roblox::instance character = globals::instances::lp.main;
        if (!is_valid_address(character.address))
            return;

        roblox::instance humanoid = character.findfirstchild("Humanoid");
        if (!is_valid_address(humanoid.address))
            return;

        roblox::instance players = globals::instances::players;
        if (!is_valid_address(players.address))
            return;

        roblox::instance player = players.findfirstchild(globals::instances::lp.name);
        if (!is_valid_address(player.address))
            return;

        roblox::instance backpack = player.findfirstchild("Backpack");
        if (!is_valid_address(backpack.address))
            return;

        std::vector<roblox::instance> children = backpack.get_children();

        for (auto& tool : children)
        {
            if (!is_valid_address(tool.address))
                continue;

            std::string className = tool.get_class_name();
            std::string toolName = tool.get_name();
            
            if (className != "Tool" && toolName != "Combat")
                continue;

            roblox::instance shootingCooldown = tool.findfirstchild("ShootingCooldown");
            roblox::instance toleranceCooldown = tool.findfirstchild("ToleranceCooldown");
            
            if (is_valid_address(shootingCooldown.address))
            {
                shootingCooldown.write_double_value(0.000000001);
            }
            if (is_valid_address(toleranceCooldown.address))
            {
                toleranceCooldown.write_double_value(0.000000001);
            }
        }

        // Also apply to currently equipped tool in character
        roblox::instance equippedTool = globals::instances::lp.instance.read_service("Tool");
        if (is_valid_address(equippedTool.address))
        {
            roblox::instance shootingCooldown = equippedTool.findfirstchild("ShootingCooldown");
            roblox::instance toleranceCooldown = equippedTool.findfirstchild("ToleranceCooldown");
            
            if (is_valid_address(shootingCooldown.address))
            {
                shootingCooldown.write_double_value(0.000000001);
            }
            if (is_valid_address(toleranceCooldown.address))
            {
                toleranceCooldown.write_double_value(0.000000001);
            }
        }
    }
};
