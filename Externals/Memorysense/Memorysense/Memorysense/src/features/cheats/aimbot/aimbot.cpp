#include "aimbot.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <mutex>
#include "../console/console.h"
#include "../../../src/main.h"

namespace memorysense {
    namespace aimbot {
        
        
        AimbotSettings settings;
        TriggerbotSettings trigger_settings;
        
        
        SavedTarget saved_target = {0, 0};
        math::vector3_t aimbot_body_part = {-1, -1, -1};
        
        
        float calculate_distance(math::vector2_t a, math::vector2_t b) {
            return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        }
        
        math::vector3_t cross_product(const math::vector3_t& vec1, const math::vector3_t& vec2) {
            return {
                vec1.y * vec2.z - vec1.z * vec2.y,
                vec1.z * vec2.x - vec1.x * vec2.z,
                vec1.x * vec2.y - vec1.y * vec2.x
            };
        }
        
        math::vector3_t normalize(const math::vector3_t& vec) {
            float length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
            if (length != 0) {
                return { vec.x / length, vec.y / length, vec.z / length };
            } else {
                return vec;
            }
        }
        
        math::matrix3_t lerp_matrix3(const math::matrix3_t& a, const math::matrix3_t& b, float t) {
            math::matrix3_t result{};
            for (int i = 0; i < 9; ++i) {
                result.data[i] = a.data[i] + (b.data[i] - a.data[i]) * t;
            }
            return result;
        }
        
        math::matrix3_t look_at_to_matrix(const math::vector3_t& camera_position, const math::vector3_t& target_position) {
            math::vector3_t forward = normalize(math::vector3_t{
                (target_position.x - camera_position.x),
                (target_position.y - camera_position.y),
                (target_position.z - camera_position.z)
            });
            math::vector3_t right = normalize(cross_product({0, 1, 0}, forward));
            math::vector3_t up = cross_product(forward, right);
            
            math::matrix3_t look_at_matrix{};
            look_at_matrix.data[0] = (right.x * -1);  look_at_matrix.data[1] = up.x;  look_at_matrix.data[2] = -forward.x;
            look_at_matrix.data[3] = right.y;  look_at_matrix.data[4] = up.y;  look_at_matrix.data[5] = -forward.y;
            look_at_matrix.data[6] = (right.z * -1);  look_at_matrix.data[7] = up.z;  look_at_matrix.data[8] = -forward.z;
            
            return look_at_matrix;
        }
        
        std::tuple<rbx::part_t, rbx::instance_t> get_closest_player_to_cursor() {
            POINT cursor_point;
            GetCursorPos(&cursor_point);
            ScreenToClient(FindWindowA(0, "Roblox"), &cursor_point);
            math::vector2_t cursor = {
                static_cast<float>(cursor_point.x),
                static_cast<float>(cursor_point.y)
            };
            
            rbx::instance_t closest_player;
            rbx::part_t closest_body_part;
            float min_dist = 9e9;
            
            
            std::lock_guard<std::mutex> lock(g_players_mutex);
            for (const auto& cached_player : g_player_cache) {
                if (cached_player.localplayer || cached_player.character.address == 0) continue;
                if (cached_player.health <= 0) continue;
                if (cached_player.distance > memorysense::aimbot::max_distance) continue;
                if (cached_player.distance < memorysense::aimbot::min_distance) continue;
                
                
                if (memorysense::aimbot::team_check && cached_player.team.address != 0) {
                    
                }
                
                rbx::part_t target_part;
                switch (memorysense::aimbot::aim_part) {
                    case 0: 
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("Head") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("Head");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Head") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Head");
                        } else {
                            continue;
                        }
                        break;
                    case 1: 
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("HumanoidRootPart") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("HumanoidRootPart");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Torso") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Torso");
                        } else {
                            continue;
                        }
                        break;
                    case 2: 
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("HumanoidRootPart") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("HumanoidRootPart");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Torso") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Torso");
                        } else {
                            continue;
                        }
                        break;
                    default:
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("HumanoidRootPart") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("HumanoidRootPart");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Torso") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Torso");
                        } else {
                            continue;
                        }
                        break;
                }
                
                if (target_part.address == 0) continue;
                
                
                auto part_primitive = target_part.get_primitive();
                auto part_pos = part_primitive.get_position();
                auto screen_pos = g_visualengine->world_to_screen(part_pos);
                
                if (screen_pos.x < 0 || screen_pos.y < 0) continue;
                
                float dist = calculate_distance(screen_pos, cursor);
                if (dist < min_dist && (!memorysense::aimbot::disable_out_of_fov || dist <= memorysense::aimbot::fov)) {
                    min_dist = dist;
                    closest_player = cached_player.player;
                    closest_body_part = target_part;
                }
            }
            
            return {closest_body_part, closest_player};
        }
        
        void aimbot_thread() {
            while (true) {
                if (!memorysense::aimbot::enabled || !g_datamodel || g_datamodel->find_first_child("Players") == 0) {
                    saved_target.player = 0;
                    aimbot_body_part = math::vector3_t(-1, -1, -1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                
                {
                    std::lock_guard<std::mutex> lock(g_players_mutex);
                    if (g_player_cache.empty()) {
                        static int cache_debug_counter = 0;
                        if (cache_debug_counter++ % 1000 == 0) {
                            CONSOLE_DEBUG("Aimbot: No players in cache");
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    }
                }
                
                update_keybind(memorysense::aimbot::key, memorysense::aimbot::enabled, memorysense::aimbot::mode);
                
                if (memorysense::aimbot::key == 0) {
                    memorysense::aimbot::enabled = true;
                }
                
                bool should_aimbot = false;
                switch (memorysense::aimbot::mode) {
                    case 0: if (memorysense::aimbot::enabled) should_aimbot = true; break;
                    case 1: should_aimbot = memorysense::aimbot::enabled; break;
                    case 2: should_aimbot = true; break;
                }
                
                static int debug_counter = 0;
                if (debug_counter++ % 1000 == 0) {
                    CONSOLE_DEBUG("Aimbot Debug: enabled=%d, key=%d, mode=%d, should_aimbot=%d", 
                        memorysense::aimbot::enabled, memorysense::aimbot::key, memorysense::aimbot::mode, should_aimbot);
                    
                    if (memorysense::aimbot::key != 0) {
                        bool key_pressed = (GetAsyncKeyState(memorysense::aimbot::key) & 0x8000) != 0;
                        CONSOLE_DEBUG("Key %d pressed: %d", memorysense::aimbot::key, key_pressed);
                    }
                }
                
                if (!should_aimbot) {
                    saved_target.player = 0;
                    aimbot_body_part = math::vector3_t(-1, -1, -1);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                
                rbx::part_t body_part;
                rbx::instance_t player;
                if (saved_target.player.address == 0) {
                    std::tie(body_part, player) = get_closest_player_to_cursor();
                    saved_target = {player, body_part};
                    
                    if (player.address != 0) {
                        CONSOLE_DEBUG("Aimbot: Found target player");
                    }
                } else {
                    body_part = saved_target.body_part;
                    player = saved_target.player;
                }
                
                if (body_part.address == 0 || player.address == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                
                static int aim_debug_counter = 0;
                if (aim_debug_counter++ % 100 == 0) {
                    CONSOLE_DEBUG("Aimbot: Attempting to aim at target");
                }
                
                if (saved_target.body_part.address != 0) {
                    auto part_primitive = saved_target.body_part.get_primitive();
                    auto target_pos = part_primitive.get_position();
                    
                    if (memorysense::aimbot::prediction) {
                        auto velocity = part_primitive.get_velocity();
                        target_pos.x += velocity.x * memorysense::aimbot::pred_x;
                        target_pos.y += velocity.y * memorysense::aimbot::pred_y;
                        target_pos.z += velocity.z * memorysense::aimbot::pred_z;
                    }
                    
                    
                    auto workspace = g_datamodel->get_workspace();
                    auto camera_addr = workspace.find_first_child("Camera");
                    if (camera_addr == 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    
                    auto camera_obj = rbx::camera_t(camera_addr);
                    auto camera_pos = camera_obj.get_camera_position();
                    auto camera_rot = camera_obj.get_camera_rotation();
                    
                    
                    auto screen_pos = g_visualengine->world_to_screen(target_pos);
                    if (screen_pos.x < 0 || screen_pos.y < 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        continue;
                    }
                    
                    POINT cursor_point;
                    GetCursorPos(&cursor_point);
                    ScreenToClient(FindWindowA(NULL, "Roblox"), &cursor_point);
                    
                    float smoothness = ((static_cast<float>(101) - static_cast<float>(memorysense::aimbot::smooth)) / 100.0f);
                    
                    if (memorysense::aimbot::aim_type == 0) { 
                        math::vector2_t relative = {
                            (screen_pos.x - cursor_point.x) * 0.5f / memorysense::aimbot::smooth,
                            (screen_pos.y - cursor_point.y) * 0.5f / memorysense::aimbot::smooth
                        };
                        
                        if (relative.x != -1 && relative.y != -1) {
                            INPUT input;
                            input.mi.time = 0;
                            input.type = INPUT_MOUSE;
                            input.mi.mouseData = 0;
                            input.mi.dx = static_cast<LONG>(relative.x);
                            input.mi.dy = static_cast<LONG>(relative.y);
                            input.mi.dwFlags = MOUSEEVENTF_MOVE;
                            SendInput(1, &input, sizeof(input));
                        }
                    }
                    else if (memorysense::aimbot::aim_type == 1) { 
                        math::matrix3_t hit_matrix = look_at_to_matrix(camera_pos, target_pos);
                        math::matrix3_t relative_matrix = lerp_matrix3(camera_rot, hit_matrix, smoothness);
                        camera_obj.set_camera_rotation(relative_matrix);
                    }
                    else if (memorysense::aimbot::aim_type == 2) { 
                        
                        
                    }
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        void triggerbot_thread() {
            while (true) {
                if (!memorysense::aimbot::triggerbot_enabled || !g_datamodel || g_datamodel->find_first_child("Players") == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                
                update_keybind(memorysense::aimbot::triggerbot_keybind_key, memorysense::aimbot::triggerbot_enabled, memorysense::aimbot::triggerbot_keybind_type);
                
                bool should_trigger = false;
                switch (memorysense::aimbot::triggerbot_keybind_type) {
                    case 0: if (memorysense::aimbot::triggerbot_enabled) should_trigger = true; break;
                    case 1: should_trigger = memorysense::aimbot::triggerbot_enabled; break;
                    case 2: should_trigger = true; break;
                }
                
                if (!should_trigger) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
                
                
                POINT cursor_point;
                GetCursorPos(&cursor_point);
                ScreenToClient(FindWindowA(NULL, "Roblox"), &cursor_point);
                math::vector2_t cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
                
                bool should_fire = false;
                std::lock_guard<std::mutex> lock(g_players_mutex);
                for (const auto& cached_player : g_player_cache) {
                    if (cached_player.localplayer || cached_player.character.address == 0) continue;
                    if (cached_player.health <= 0) continue;
                    if (cached_player.distance > memorysense::aimbot::triggerbot_max_distance) continue;
                    if (cached_player.distance < memorysense::aimbot::triggerbot_min_distance) continue;
                    
                    
                    if (memorysense::aimbot::triggerbot_team_check && cached_player.team.address != 0) {
                        
                    }
                    
                    rbx::part_t target_part;
                    if (memorysense::aimbot::triggerbot_head_only) {
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("Head") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("Head");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Head") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Head");
                        } else {
                            continue;
                        }
                    } else {
                        if (!cached_player.r15.r15parts.empty() && cached_player.r15.r15parts.find("HumanoidRootPart") != cached_player.r15.r15parts.end()) {
                            target_part = cached_player.r15.r15parts.at("HumanoidRootPart");
                        } else if (!cached_player.r6.r6parts.empty() && cached_player.r6.r6parts.find("Torso") != cached_player.r6.r6parts.end()) {
                            target_part = cached_player.r6.r6parts.at("Torso");
                        } else {
                            continue;
                        }
                    }
                    
                    if (target_part.address == 0) continue;
                    
                    
                    auto part_primitive = target_part.get_primitive();
                    auto part_pos = part_primitive.get_position();
                    auto screen_pos = g_visualengine->world_to_screen(part_pos);
                    
                    if (screen_pos.x < 0 || screen_pos.y < 0) continue;
                    
                    float dist = calculate_distance(cursor, screen_pos);
                    if (dist <= memorysense::aimbot::triggerbot_fov) {
                        
                        if (memorysense::aimbot::triggerbot_hit_chance < 100) {
                            if ((rand() % 100) > memorysense::aimbot::triggerbot_hit_chance) {
                                continue;
                            }
                        }
                        
                        should_fire = true;
                        break;
                    }
                }
                
                if (should_fire) {
                    
                    INPUT input{};
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(input));
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(input));
                    std::this_thread::sleep_for(std::chrono::milliseconds(memorysense::aimbot::triggerbot_delay_ms));
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        
        void initialize_threads() {
            
            std::thread(aimbot_thread).detach();
            std::thread(triggerbot_thread).detach();
        }
        
        
        static bool aimbot_was_pressed = false;
        static bool triggerbot_was_pressed = false;
        
        void update_keybind(int key, bool& enabled, int type) {
            
            if (key == 0) return; 
            
            bool is_pressed = (GetAsyncKeyState(key) & 0x8000) != 0;
            
            if (is_pressed) {
                if (type == 0) { 
                    if (!aimbot_was_pressed) {
                        enabled = !enabled;
                        aimbot_was_pressed = true;
                        CONSOLE_DEBUG("Aimbot: Toggle pressed, enabled=%d", enabled);
                    }
                } else if (type == 1) { 
                    enabled = true;
                } else if (type == 2) { 
                    enabled = true;
                }
            } else {
                if (type == 0) { 
                    aimbot_was_pressed = false;
                } else if (type == 1) { 
                    enabled = false;
                } else if (type == 2) { 
                    enabled = true;
                }
            }
        }
        
        int random_int(int min, int max) {
            return min + rand() % (max - min + 1);
        }
    }
}
