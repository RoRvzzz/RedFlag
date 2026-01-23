#include "configs.hpp"

#include <iostream>
#include <fstream>
#include <ShlObj.h>
#include <filesystem>
#include "../library/json/json.hpp"
#include "../../globals/globals.hpp"
using namespace nlohmann;

// Function to replace text within a string
void replace(std::string& str, const std::string& from, const std::string& to) {
    std::size_t start_position = str.find(from);
    if (start_position == std::string::npos) return;
    str.replace(start_position, from.length(), to);
}

// Function to save data to a file
void save_file(const std::string& path, const std::string& data) {
    std::ofstream out_file(path);
    if (out_file.is_open()) {
        out_file << data;
        out_file.close();
    }
    else {
        std::cout << "Failed to open file for saving: " << path << std::endl;
    }
}


void load_file(const std::string& path, std::string& data) {
    std::ifstream in_file(path);
    if (in_file.is_open()) {
        in_file >> data;
        in_file.close();
    }
    else {
        std::cout << "Failed to open file for loading: " << path << std::endl;
    }
}


template<typename T>
void load_cfg_option(T& globalsetting, const json& cfg_obj, const std::string& cfg_setting) {
    if (!cfg_obj.contains(cfg_setting)) return;
    globalsetting = cfg_obj[cfg_setting];
}


    void RBX::configs::save(const char* name) {
        json config;

#define SAVE_GLOBAL(var) config[#var] = globals::var;

        SAVE_GLOBAL(chams_opacity)
            SAVE_GLOBAL(menu_glow)
            SAVE_GLOBAL(tracer_esp)
            SAVE_GLOBAL(skeletonThickness)
            SAVE_GLOBAL(health_bar_text)
            SAVE_GLOBAL(silentaim)
            SAVE_GLOBAL(glow_opacity)
            SAVE_GLOBAL(glow_size)
            SAVE_GLOBAL(HeavyOptimize)
            SAVE_GLOBAL(skeletonOutline)
            SAVE_GLOBAL(crosshair_gap)
            SAVE_GLOBAL(crosshair_size)
            SAVE_GLOBAL(crosshair)
            SAVE_GLOBAL(alpha)
            SAVE_GLOBAL(crosshair_speed)
            SAVE_GLOBAL(OptimizeCache)
       
            SAVE_GLOBAL(fonttype)
            SAVE_GLOBAL(tool_outline)
            SAVE_GLOBAL(fasttools)
            SAVE_GLOBAL(distancecheck)
            SAVE_GLOBAL(offscreen_Check)
           
            SAVE_GLOBAL(onlylocal)
            SAVE_GLOBAL(autoparry)
            SAVE_GLOBAL(skeleton_esp)
            SAVE_GLOBAL(onlytarget)
            SAVE_GLOBAL(chams)
            SAVE_GLOBAL(camera_fov)
        
            SAVE_GLOBAL(fill_box)
            SAVE_GLOBAL(flame_bar)
            SAVE_GLOBAL(triggerbot_delay)
            SAVE_GLOBAL(fill_chams)
            SAVE_GLOBAL(threadcrash)
            SAVE_GLOBAL(high_cpu_usage)
           
            SAVE_GLOBAL(teambasedcolor)
            SAVE_GLOBAL(localplayercheck)
            SAVE_GLOBAL(tool_esp)
            SAVE_GLOBAL(noclip)
            SAVE_GLOBAL(aimbot)
            SAVE_GLOBAL(rescan)
            SAVE_GLOBAL(draw_aimbot_fov)
            SAVE_GLOBAL(aimbot_fov_size)
            SAVE_GLOBAL(fly)
            SAVE_GLOBAL(max_aimbot_distance)
            SAVE_GLOBAL(triggerbot)
            SAVE_GLOBAL(triggerbot_sticky)
            SAVE_GLOBAL(draw_triggerbot_fov)
            SAVE_GLOBAL(esp)
            SAVE_GLOBAL(max_render_distance)
            SAVE_GLOBAL(box_esp)
            SAVE_GLOBAL(health_bar)
            SAVE_GLOBAL(shield_bar)
            SAVE_GLOBAL(name_esp)
            SAVE_GLOBAL(name_outline)
            SAVE_GLOBAL(distance_esp)
            SAVE_GLOBAL(distance_outline)
            SAVE_GLOBAL(nojumpcooldown)
            SAVE_GLOBAL(walkspeed_amount)
            SAVE_GLOBAL(JumpPower)
            SAVE_GLOBAL(fly_speed)
            SAVE_GLOBAL(force_projection_update)
            SAVE_GLOBAL(highcpuusageesp)
            SAVE_GLOBAL(vsync)
            SAVE_GLOBAL(streamproof)
            SAVE_GLOBAL(font_type)
            SAVE_GLOBAL(chams_outline)
            SAVE_GLOBAL(debug_info)
            SAVE_GLOBAL(use_class_names)
            SAVE_GLOBAL(name_color)
            SAVE_GLOBAL(TracerColor)
            SAVE_GLOBAL(HealthBasedColor)
            SAVE_GLOBAL(free_aim)
            SAVE_GLOBAL(free_aim_sticky)
            SAVE_GLOBAL(localplayercheck)
            SAVE_GLOBAL(free_aim_resolver)
            SAVE_GLOBAL(free_aim_is_in_fov)
            SAVE_GLOBAL(free_aim_draw_fov)
            SAVE_GLOBAL(flag_esp)
            SAVE_GLOBAL(TeamVisual)
            SAVE_GLOBAL(silent_aim_checks)
            SAVE_GLOBAL(camera_fov)
            SAVE_GLOBAL(max_free_aim_distance)
            SAVE_GLOBAL(free_aim_fov)
           
            SAVE_GLOBAL(free_aim_part)
            SAVE_GLOBAL(free_aim_closest_part)
            SAVE_GLOBAL(free_aim_prediction)
            SAVE_GLOBAL(trigger_bot_prediction)
            SAVE_GLOBAL(free_aim_prediction_x)
            SAVE_GLOBAL(free_aim_prediction_y)
            SAVE_GLOBAL(trigger_bot_prediction_x)
            SAVE_GLOBAL(trigger_bot_prediction_y)
            SAVE_GLOBAL(triggerbot)
            SAVE_GLOBAL(triggerbot_sticky)
            SAVE_GLOBAL(draw_triggerbot_fov)
            SAVE_GLOBAL(fly)
            SAVE_GLOBAL(triggerbot_checks)
            SAVE_GLOBAL(max_triggerbot_distance)
            SAVE_GLOBAL(cashier_esp)
            SAVE_GLOBAL(soccer_ball_esp)
            SAVE_GLOBAL(jail_esp)
            SAVE_GLOBAL(triggerbot_radius)
            SAVE_GLOBAL(esp)
            SAVE_GLOBAL(cash_esp)
            SAVE_GLOBAL(max_render_distance)
            SAVE_GLOBAL(fly_speed)
            SAVE_GLOBAL(box_esp)
            SAVE_GLOBAL(box_type)
            SAVE_GLOBAL(healthBar_Color)
            SAVE_GLOBAL(skeleton_Color)
            SAVE_GLOBAL(box_color)
            SAVE_GLOBAL(color_3)
            SAVE_GLOBAL(color_2)
            SAVE_GLOBAL(color_1)
            SAVE_GLOBAL(color_5)
            SAVE_GLOBAL(color_4)
            SAVE_GLOBAL(TracerColor)
            SAVE_GLOBAL(HighLight_color)
            SAVE_GLOBAL(health_bar)
            SAVE_GLOBAL(highlight)
            SAVE_GLOBAL(shield_bar)
            SAVE_GLOBAL(name_esp)
            SAVE_GLOBAL(name_outline)
            SAVE_GLOBAL(name_type)
            SAVE_GLOBAL(name_color)
            SAVE_GLOBAL(distance_esp)
            SAVE_GLOBAL(distance_outline)
            SAVE_GLOBAL(distance_color)
            SAVE_GLOBAL(teamNameColor)
            SAVE_GLOBAL(FovColor)
            SAVE_GLOBAL(esp_checks)
           // SAVE_GLOBAL(aimbot_bind)
           // SAVE_GLOBAL(free_aim_bind)
           // SAVE_GLOBAL(silent_Aim_Bind)
           // SAVE_GLOBAL(WalkSpeed_Bind)
           // SAVE_GLOBAL(JumpPower_Bind)
         //   SAVE_GLOBAL(HipHeight_Bind)
            SAVE_GLOBAL(noclip)
            SAVE_GLOBAL(nojumpcooldown)
            SAVE_GLOBAL(threadrestarttime)
            SAVE_GLOBAL(threadtime)
            SAVE_GLOBAL(walkspeed_amount)
            SAVE_GLOBAL(JumpPower)
            SAVE_GLOBAL(HipHeight)
            SAVE_GLOBAL(skeletonOutline)
            SAVE_GLOBAL(game_id)
            SAVE_GLOBAL(teambasedcolor)
            SAVE_GLOBAL(skeletonThickness)
            SAVE_GLOBAL(tracer_esp)
            SAVE_GLOBAL(flame_bar)
            SAVE_GLOBAL(force_projection_update)
            SAVE_GLOBAL(highcpuusageesp)
            SAVE_GLOBAL(vsync)
            SAVE_GLOBAL(streamproof)
            SAVE_GLOBAL(font_type)
            SAVE_GLOBAL(menu_glow)
            SAVE_GLOBAL(chams_opacity)
            SAVE_GLOBAL(fill_chams)
            SAVE_GLOBAL(chams_outline)
            SAVE_GLOBAL(debug_info)
            SAVE_GLOBAL(use_class_names)

            SAVE_GLOBAL(fps_cap)
            SAVE_GLOBAL(health_y)
            SAVE_GLOBAL(health_x)
            SAVE_GLOBAL(fov_segmants)
            SAVE_GLOBAL(linkTarget)
            SAVE_GLOBAL(non_zero_check)
            SAVE_GLOBAL(boxGlow)
            SAVE_GLOBAL(allow_tearing)
            SAVE_GLOBAL(autotype)
            SAVE_GLOBAL(enable_health_glow)



            save_file(appdata_path() + "\\Void\\configs\\" + static_cast<std::string>(name) + ".cfg", config.dump());
    
}

template<typename T, std::size_t N>
void LoadArray(T(&Array)[N], const json& Config, const std::string& Name)
{
    if (!Config.contains(Name) || !Config[Name].is_array()) return;

    for (std::size_t I = 0; I < N && I < Config[Name].size(); ++I)
    {
        Array[I] = Config[Name][I].get<T>();
    }
}

void RBX::configs::load(const char* name) {
    std::string cfg_json = "";
    load_file(appdata_path() + "\\Void\\configs\\" + static_cast<std::string>(name) + ".cfg", cfg_json);

    if (cfg_json.empty()) {
        std::cout << "Config file is empty or invalid: " << name << std::endl;
        return;
    }

    json config = json::parse(cfg_json);

#define LOAD_GLOBAL(var)   globals::var = config[#var];

    LOAD_GLOBAL(chams_opacity)
        LOAD_GLOBAL(menu_glow)
        LOAD_GLOBAL(tracer_esp)
        LOAD_GLOBAL(skeletonThickness)
        LOAD_GLOBAL(health_bar_text)
        LOAD_GLOBAL(silentaim)
        LOAD_GLOBAL(glow_opacity)
        LOAD_GLOBAL(glow_size)
        LOAD_GLOBAL(HeavyOptimize)
        LOAD_GLOBAL(skeletonOutline)
        LOAD_GLOBAL(crosshair_gap)
        LOAD_GLOBAL(crosshair_size)
        LOAD_GLOBAL(crosshair)
        LOAD_GLOBAL(alpha)
        LOAD_GLOBAL(crosshair_speed)
        LOAD_GLOBAL(OptimizeCache)

        LOAD_GLOBAL(fonttype)
        LOAD_GLOBAL(tool_outline)
        LOAD_GLOBAL(fasttools)
        LOAD_GLOBAL(distancecheck)
        LOAD_GLOBAL(offscreen_Check)

        LOAD_GLOBAL(onlylocal)
        LOAD_GLOBAL(autoparry)
        LOAD_GLOBAL(skeleton_esp)
        LOAD_GLOBAL(onlytarget)
        LOAD_GLOBAL(chams)
        LOAD_GLOBAL(camera_fov)

        LOAD_GLOBAL(fill_box)
        LOAD_GLOBAL(flame_bar)
        LOAD_GLOBAL(triggerbot_delay)
        LOAD_GLOBAL(fill_chams)
        LOAD_GLOBAL(threadcrash)
        LOAD_GLOBAL(high_cpu_usage)

        LOAD_GLOBAL(teambasedcolor)
        LOAD_GLOBAL(localplayercheck)
        LOAD_GLOBAL(tool_esp)
        LOAD_GLOBAL(noclip)
        LOAD_GLOBAL(aimbot)
        LOAD_GLOBAL(rescan)
        LOAD_GLOBAL(draw_aimbot_fov)
        LOAD_GLOBAL(aimbot_fov_size)
        LOAD_GLOBAL(fly)
        LOAD_GLOBAL(max_aimbot_distance)
        LOAD_GLOBAL(triggerbot)
        LOAD_GLOBAL(triggerbot_sticky)
        LOAD_GLOBAL(draw_triggerbot_fov)
        LOAD_GLOBAL(esp)
        LOAD_GLOBAL(max_render_distance)
        LOAD_GLOBAL(box_esp)
        LOAD_GLOBAL(health_bar)
        LOAD_GLOBAL(shield_bar)
        LOAD_GLOBAL(name_esp)
        LOAD_GLOBAL(name_outline)
        LOAD_GLOBAL(distance_esp)
        LOAD_GLOBAL(distance_outline)
        LOAD_GLOBAL(nojumpcooldown)
        LOAD_GLOBAL(walkspeed_amount)
        LOAD_GLOBAL(JumpPower)
        LOAD_GLOBAL(fly_speed)
        LOAD_GLOBAL(force_projection_update)
        LOAD_GLOBAL(highcpuusageesp)
        LOAD_GLOBAL(vsync)
        LOAD_GLOBAL(streamproof)
        LOAD_GLOBAL(font_type)
        LOAD_GLOBAL(chams_outline)
        LOAD_GLOBAL(debug_info)
        LOAD_GLOBAL(use_class_names)
       
        LoadArray(globals::TracerColor, config, "TracerColor");
    LoadArray(globals::name_color, config, "NameColor");
      
        LOAD_GLOBAL(HealthBasedColor)
        LOAD_GLOBAL(free_aim)
        LOAD_GLOBAL(free_aim_sticky)
        LOAD_GLOBAL(localplayercheck)
        LOAD_GLOBAL(free_aim_resolver)
        LOAD_GLOBAL(free_aim_is_in_fov)
        LOAD_GLOBAL(free_aim_draw_fov)
        LOAD_GLOBAL(flag_esp)
        LOAD_GLOBAL(TeamVisual)
     
        LOAD_GLOBAL(camera_fov)
        LOAD_GLOBAL(max_free_aim_distance)
        LOAD_GLOBAL(free_aim_fov)

        LOAD_GLOBAL(free_aim_part)
        LOAD_GLOBAL(free_aim_closest_part)
        LOAD_GLOBAL(free_aim_prediction)
        LOAD_GLOBAL(trigger_bot_prediction)
        LOAD_GLOBAL(free_aim_prediction_x)
        LOAD_GLOBAL(free_aim_prediction_y)
        LOAD_GLOBAL(trigger_bot_prediction_x)
        LOAD_GLOBAL(trigger_bot_prediction_y)
        LOAD_GLOBAL(triggerbot)
        LOAD_GLOBAL(triggerbot_sticky)
        LOAD_GLOBAL(draw_triggerbot_fov)
        LOAD_GLOBAL(fly)
      
        LOAD_GLOBAL(max_triggerbot_distance)
        LOAD_GLOBAL(cashier_esp)
        LOAD_GLOBAL(soccer_ball_esp)
        LOAD_GLOBAL(jail_esp)
        LOAD_GLOBAL(triggerbot_radius)
        LOAD_GLOBAL(esp)
        LOAD_GLOBAL(cash_esp)
        LOAD_GLOBAL(max_render_distance)
        LOAD_GLOBAL(fly_speed)
        LOAD_GLOBAL(box_esp)
        LOAD_GLOBAL(box_type)
        LoadArray(globals::healthBar_Color, config, "HealthBarColor");
        LoadArray(globals::skeleton_Color, config, "SkeletonColor");
        LoadArray(globals::box_color, config, "BoxColor");
        LoadArray(globals::color_3, config, "Color3");
        LoadArray(globals::color_2, config, "Color2");
        LoadArray(globals::color_1, config, "Color1");
        LoadArray(globals::color_5, config, "Color5");
        LoadArray(globals::color_4, config, "Color4");
        LoadArray(globals::HighLight_color, config, "HightLightColor");
      
        LOAD_GLOBAL(health_bar)
        LOAD_GLOBAL(highlight)
        LOAD_GLOBAL(shield_bar)
        LOAD_GLOBAL(name_esp)
        LOAD_GLOBAL(name_outline)
        LOAD_GLOBAL(name_type)
       
        LOAD_GLOBAL(distance_esp)
        LOAD_GLOBAL(distance_outline)
   
            LoadArray(globals::teamNameColor, config, "TeamNameColor");
        LoadArray(globals::distance_color, config, "DistanceColor");
        LoadArray(globals::FovColor, config, "FovColor");
        // LOAD_GLOBAL(aimbot_bind)
        // LOAD_GLOBAL(free_aim_bind)
        // LOAD_GLOBAL(silent_Aim_Bind)
        // LOAD_GLOBAL(WalkSpeed_Bind)
        // LOAD_GLOBAL(JumpPower_Bind)
      //   LOAD_GLOBAL(HipHeight_Bind)
        LOAD_GLOBAL(noclip)
        LOAD_GLOBAL(nojumpcooldown)
        LOAD_GLOBAL(threadrestarttime)
        LOAD_GLOBAL(threadtime)
        LOAD_GLOBAL(walkspeed_amount)
        LOAD_GLOBAL(JumpPower)
        LOAD_GLOBAL(HipHeight)
        LOAD_GLOBAL(skeletonOutline)
        LOAD_GLOBAL(game_id)
        LOAD_GLOBAL(teambasedcolor)
        LOAD_GLOBAL(skeletonThickness)
        LOAD_GLOBAL(tracer_esp)
        LOAD_GLOBAL(flame_bar)
        LOAD_GLOBAL(force_projection_update)
        LOAD_GLOBAL(highcpuusageesp)
        LOAD_GLOBAL(vsync)
        LOAD_GLOBAL(streamproof)
        LOAD_GLOBAL(font_type)
        LOAD_GLOBAL(menu_glow)
        LOAD_GLOBAL(chams_opacity)
        LOAD_GLOBAL(fill_chams)
        LOAD_GLOBAL(chams_outline)
        LOAD_GLOBAL(debug_info)
        LOAD_GLOBAL(use_class_names)

        LOAD_GLOBAL(fps_cap)
        LOAD_GLOBAL(health_y)
        LOAD_GLOBAL(health_x)
        LOAD_GLOBAL(fov_segmants)
        LOAD_GLOBAL(linkTarget)
        LOAD_GLOBAL(non_zero_check)
        LOAD_GLOBAL(boxGlow)
        LOAD_GLOBAL(allow_tearing)
        LOAD_GLOBAL(autotype)
        LOAD_GLOBAL(enable_health_glow)

}


