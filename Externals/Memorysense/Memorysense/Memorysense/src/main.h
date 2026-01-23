#pragma once
#include <memory>
#include <vector>
#include <memory/memory.h>
#include <render/render.h>

#include <sdk/sdk.h>
#include <features/cache/cache.h>

inline std::unique_ptr<memory_t> memory = std::make_unique<memory_t>();
inline std::unique_ptr<render_t> render;


inline std::unique_ptr<rbx::datamodel_t> g_datamodel;
inline std::unique_ptr<rbx::workspace_t> g_workspace;
inline std::unique_ptr<rbx::replicated_storage_t> g_replicated_storage;
inline std::unique_ptr<rbx::visualengine_t> g_visualengine;
inline std::unique_ptr<rbx::visualengine_t> g_players;
inline std::vector<rbx::player_t> g_cache_players;
inline std::vector<cache::cached_entity> g_player_cache;
static std::mutex g_players_mutex;

namespace memorysense {

	namespace aimbot {
		inline bool enabled = false;
		inline int key = 0;
		inline int mode = 0;
		
		
		inline bool team_check = false;
		inline bool disable_out_of_fov = true;
		inline bool prediction = false;
		inline bool spectate_target = false;
		
		inline int aim_part = 1; 
		inline int aim_type = 0; 
		inline int smooth = 50;
		inline int fov = 100;
		inline int min_distance = 0;
		inline int max_distance = 1000;
		
		inline float pred_x = 1.0f;
		inline float pred_y = 1.0f;
		inline float pred_z = 1.0f;
		
		
		
		inline bool triggerbot_enabled = false;
		inline bool triggerbot_team_check = false;
		inline bool triggerbot_head_only = false;
		inline bool triggerbot_wall_check = false;
		
		inline int triggerbot_fov = 50;
		inline int triggerbot_min_distance = 0;
		inline int triggerbot_max_distance = 1000;
		inline int triggerbot_hit_chance = 100;
		inline int triggerbot_delay_ms = 10;
		
		inline int triggerbot_keybind_type = 0;
		inline int triggerbot_keybind_key = 0;
	}

	namespace visuals {
		inline bool box;
		inline bool flags;
		inline bool healthbar;
		inline bool name;
		inline bool distance;
		inline bool tracer;
		inline bool cham;

		inline int cham_type = 0;

		inline bool clientcheck;
		inline bool teamcheck;

		inline float healthbar_padding = 1.0f;
		inline float healthbar_size = 1.0f;

		inline ImU32 outline_color = IM_COL32(0, 0, 0, 255);
		inline std::vector<int> outline_elements = {1, 1, 1, 1, 1, 1}; 
		
		
		enum class ESP_SIDE {
			LEFT = 0,
			RIGHT = 1,
			TOP = 2,
			BOTTOM = 3
		};
		
		
		inline std::vector<ESP_SIDE> element_sides = {ESP_SIDE::TOP, ESP_SIDE::BOTTOM, ESP_SIDE::RIGHT, ESP_SIDE::BOTTOM, ESP_SIDE::LEFT};
		
		
		inline std::vector<int> element_priorities = {3, 2, 1, 0, 4}; 

		namespace colors {
			namespace visible {
				inline ImU32 box = IM_COL32(255, 255, 255, 255);
				inline ImU32 flags = IM_COL32(255, 255, 255, 255);
				inline ImU32 healthbar = IM_COL32(0, 255, 0, 255);
				inline ImU32 name = IM_COL32(255, 255, 255, 255);
				inline ImU32 distance = IM_COL32(255, 255, 255, 255);
				inline ImU32 tracer = IM_COL32(255, 255, 255, 255);
				inline ImU32 cham_fill = IM_COL32(15, 20, 40, 100);
				inline ImU32 cham_outline = IM_COL32(255, 255, 255, 100);
			}
			namespace invisible {
				inline ImU32 box = IM_COL32(255, 0, 0, 255);
				inline ImU32 healthbar = IM_COL32(255, 0, 0, 255);
				inline ImU32 name = IM_COL32(255, 0, 0, 255);
				inline ImU32 distance = IM_COL32(255, 0, 0, 255);
				inline ImU32 tracer = IM_COL32(255, 0, 0, 255);
				inline ImU32 cham_fill = IM_COL32(15, 20, 40, 100);
				inline ImU32 cham_outline = IM_COL32(255, 255, 255, 100);
			}
		}
	}

	namespace rage {
		namespace movement {

		}
		namespace antiaim {
			
		}
		namespace rage {
			
		}
	}

	namespace custom {

	}
}
