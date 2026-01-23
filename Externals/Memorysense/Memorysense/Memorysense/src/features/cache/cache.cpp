#include "cache.h"
#include <cmath>
#include <sdk/offsets/offsets.h>
#include <memory/memory.h>
#include <main.h>

extern std::unique_ptr<memory_t> memory;
extern std::unique_ptr<rbx::datamodel_t> g_datamodel;
extern std::unique_ptr<rbx::visualengine_t> g_visualengine;
extern std::unique_ptr<rbx::visualengine_t> g_players;
extern std::vector<cache::cached_entity> g_player_cache;
extern std::vector<rbx::player_t> g_cache_players;

void player_cache::start_cache() {
    while (true) {
        auto* game = g_datamodel.get();
        if (!game)
            return;

        std::vector<cache::cached_entity> temp;
        temp.reserve(32);

        {
            std::scoped_lock lock(g_players_mutex);
            for (rbx::player_t player : g_cache_players) {
                if (!player.address)
                    continue;

                cache::cached_entity cached_player;
                cached_player.player = player;
                cached_player.character = player.get_model_instance();
                
                if (!cached_player.character.address)
                    continue;
                
                cached_player.children = cached_player.character.get_children();
                cached_player.team = player.get_team();
                cached_player.name = player.get_name();

                std::uint64_t humanoid = cached_player.character.find_first_child("Humanoid");
                if (!humanoid)
                    continue;
                    
                std::uint8_t rig_type = memory->read<std::uint8_t>(humanoid + Offsets::Humanoid::RigType);

                for (rbx::part_t part : cached_player.character.get_children<rbx::part_t>()) {
                    std::string part_name = part.get_name();

                    if (rig_type == 0) {
                        cached_player.r6.r6parts[part_name] = part;
                    }

                    if (rig_type == 1) {
                        cached_player.r15.r15parts[part_name] = part;
                    }
                }

                temp.push_back(cached_player);
            }
        }

        g_player_cache = temp;

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
