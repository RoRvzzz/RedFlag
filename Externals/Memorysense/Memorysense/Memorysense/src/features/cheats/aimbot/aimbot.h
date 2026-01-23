#pragma once
#include <Windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <string>
#include <tuple>
#include <random>
#include "../../../ext/framework/settings/variables.h"
#include "../../../ext/framework/settings/functions.h"
#include "../../../src/sdk/sdk.h"
#include "../../../src/memory/memory.h"

namespace memorysense {
    namespace aimbot {
        
        
        struct AimbotSettings {
            bool enabled = false;
            bool team_check = false;
            bool disable_out_of_fov = true;
            bool prediction = false;
            bool spectate_target = false;
            
            int aim_part = 1; 
            int aim_type = 0; 
            int smooth = 50;
            int fov = 100;
            int min_distance = 0;
            int max_distance = 1000;
            
            float pred_x = 1.0f;
            float pred_y = 1.0f;
            float pred_z = 1.0f;
            
            int keybind_type = 0; 
            int keybind_key = 0;
            bool keybind_enabled = false;
        };
        
        
        struct TriggerbotSettings {
            bool enabled = false;
            bool team_check = false;
            bool head_only = false;
            bool wall_check = false;
            
            int fov = 50;
            int min_distance = 0;
            int max_distance = 1000;
            int hit_chance = 100;
            int delay_ms = 10;
            
            int keybind_type = 0;
            int keybind_key = 0;
            bool keybind_enabled = false;
        };
        
        
        extern AimbotSettings settings;
        extern TriggerbotSettings trigger_settings;
        
        
        struct SavedTarget {
            rbx::instance_t player;
            rbx::part_t body_part;
        };
        
        extern SavedTarget saved_target;
        extern math::vector3_t aimbot_body_part;
        
        
        float calculate_distance(math::vector2_t a, math::vector2_t b);
        math::vector3_t cross_product(const math::vector3_t& vec1, const math::vector3_t& vec2);
        math::vector3_t normalize(const math::vector3_t& vec);
        math::matrix3_t lerp_matrix3(const math::matrix3_t& a, const math::matrix3_t& b, float t);
        math::matrix3_t look_at_to_matrix(const math::vector3_t& camera_position, const math::vector3_t& target_position);
        
        
        std::tuple<rbx::part_t, rbx::instance_t> get_closest_player_to_cursor();
        
        
        void aimbot_thread();
        
        
        void triggerbot_thread();
        
        
        void initialize_threads();
        
        
        void update_keybind(int key, bool& enabled, int type);
        
        
        int random_int(int min, int max);
    }
}
