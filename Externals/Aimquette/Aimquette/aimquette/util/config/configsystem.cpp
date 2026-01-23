#include "configsystem.h"
#include <iostream>
#include "../notification/notification.h"
#include <algorithm>
#include <shellapi.h>

ConfigManager::ConfigManager() {
    // Get executable directory
    char exe_path[MAX_PATH];
    std::string exe_dir = "";
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        exe_dir = std::string(exe_path);
        size_t last_slash = exe_dir.find_last_of("\\/");
        if (last_slash != std::string::npos) {
            exe_dir = exe_dir.substr(0, last_slash + 1);
        }
    }
    
    // Set config directory to "config" folder next to the executable
    if (!exe_dir.empty()) {
        config_directory = exe_dir + "config";
    } else {
        // Fallback to current directory if we can't get exe path
        config_directory = "config";
    }
        if (!fs::exists(config_directory)) {
        fs::create_directories(config_directory);
    }

    refresh_config_list();
}

void ConfigManager::refresh_config_list() {
    config_files.clear();

    if (fs::exists(config_directory)) {
        for (const auto& entry : fs::directory_iterator(config_directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                config_files.push_back(entry.path().stem().string());
            }
        }
    }
    
    std::sort(config_files.begin(), config_files.end());
}

bool ConfigManager::save_config(const std::string& name) {
    if (name.empty()) return false;

    json config_json;

    // Combat settings
    config_json["combat"]["aimbot"] = globals::combat::aimbot;
    config_json["combat"]["stickyaim"] = globals::combat::stickyaim;
    config_json["combat"]["unlockondeath"] = globals::combat::unlockondeath;
    config_json["combat"]["aimbottype"] = globals::combat::aimbottype;
    config_json["combat"]["nosleep_aimbot"] = globals::combat::nosleep_aimbot;
    config_json["combat"]["usefov"] = globals::combat::usefov;
    config_json["combat"]["drawfov"] = globals::combat::drawfov;
    config_json["combat"]["fovsize"] = globals::combat::fovsize;
    config_json["combat"]["fovcolor"] = std::vector<float>(globals::combat::fovcolor, globals::combat::fovcolor + 4);
    config_json["combat"]["smoothing"] = globals::combat::smoothing;
    config_json["combat"]["smoothingx"] = globals::combat::smoothingx;
    config_json["combat"]["smoothingy"] = globals::combat::smoothingy;
    config_json["combat"]["smoothingstyle"] = globals::combat::smoothingstyle;
    config_json["combat"]["sensitivity_enabled"] = globals::combat::sensitivity_enabled;
    config_json["combat"]["cam_sensitivity"] = globals::combat::cam_sensitivity;
    config_json["combat"]["camlock_shake"] = globals::combat::camlock_shake;
    config_json["combat"]["camlock_shake_x"] = globals::combat::camlock_shake_x;
    config_json["combat"]["camlock_shake_y"] = globals::combat::camlock_shake_y;
    config_json["combat"]["camlock_shake_z"] = globals::combat::camlock_shake_z;
    config_json["combat"]["mouse_sensitivity"] = globals::combat::mouse_sensitivity;
    config_json["combat"]["predictions"] = globals::combat::predictions;
    config_json["combat"]["predictionsx"] = globals::combat::predictionsx;
    config_json["combat"]["predictionsy"] = globals::combat::predictionsy;
    config_json["combat"]["deadzone"] = globals::combat::deadzone;
    config_json["combat"]["deadzonex"] = globals::combat::deadzonex;
    config_json["combat"]["deadzoney"] = globals::combat::deadzoney;
    config_json["combat"]["teamcheck"] = globals::combat::teamcheck;
    config_json["combat"]["grabbedcheck"] = globals::combat::grabbedcheck;
    config_json["combat"]["arsenal_flick_fix"] = globals::combat::arsenal_flick_fix;
    config_json["combat"]["aimpart"] = globals::combat::aimpart;
    config_json["combat"]["airpart_enabled"] = globals::combat::airpart_enabled;
    config_json["combat"]["airpart"] = globals::combat::airpart;
    // Save ignore parts for camera and mouse
    if (globals::combat::ignore_parts_camera && !globals::combat::ignore_parts_camera->empty()) {
        config_json["combat"]["ignore_parts_camera"] = *globals::combat::ignore_parts_camera;
    }
    if (globals::combat::ignore_parts_mouse && !globals::combat::ignore_parts_mouse->empty()) {
        config_json["combat"]["ignore_parts_mouse"] = *globals::combat::ignore_parts_mouse;
    }
    config_json["combat"]["target_method"] = globals::combat::target_method;
    config_json["combat"]["silentaim"] = globals::combat::silentaim;
    config_json["combat"]["stickyaimsilent"] = globals::combat::stickyaimsilent;
    config_json["combat"]["spoofmouse"] = globals::combat::spoofmouse;
    config_json["combat"]["hitchance"] = globals::combat::hitchance;
    config_json["combat"]["closestpartsilent"] = globals::combat::closestpartsilent;
    config_json["combat"]["silentaimpart"] = globals::combat::silentaimpart;
    config_json["combat"]["usesfov"] = globals::combat::usesfov;
    config_json["combat"]["drawsfov"] = globals::combat::drawsfov;
    config_json["combat"]["sfovsize"] = globals::combat::sfovsize;
    config_json["combat"]["sfovcolor"] = std::vector<float>(globals::combat::sfovcolor, globals::combat::sfovcolor + 4);
    config_json["combat"]["silentpredictions"] = globals::combat::silentpredictions;
    config_json["combat"]["silentpredictionsx"] = globals::combat::silentpredictionsx;
    config_json["combat"]["silentpredictionsy"] = globals::combat::silentpredictionsy;
    config_json["combat"]["triggerbot"] = globals::combat::triggerbot;
    config_json["combat"]["delay"] = globals::combat::delay;
    config_json["combat"]["crew_check"] = globals::combat::crew_check;

    config_json["combat"]["keybinds"]["aimbotkeybind"]["key"] = globals::combat::aimbotkeybind.key;
    config_json["combat"]["keybinds"]["aimbotkeybind"]["type"] = static_cast<int>(globals::combat::aimbotkeybind.type);
    config_json["combat"]["keybinds"]["silentaimkeybind"]["key"] = globals::combat::silentaimkeybind.key;
    config_json["combat"]["keybinds"]["silentaimkeybind"]["type"] = static_cast<int>(globals::combat::silentaimkeybind.type);
    config_json["combat"]["keybinds"]["triggerbotkeybind"]["key"] = globals::combat::triggerbotkeybind.key;
    config_json["combat"]["keybinds"]["triggerbotkeybind"]["type"] = static_cast<int>(globals::combat::triggerbotkeybind.type);

    // Visual settings
    config_json["visuals"]["visuals"] = globals::visuals::visuals;
    config_json["visuals"]["boxes"] = globals::visuals::boxes;
    config_json["visuals"]["lockedindicator"] = globals::visuals::lockedindicator;
    config_json["visuals"]["boxtype"] = globals::visuals::boxtype;
    config_json["visuals"]["healthbar"] = globals::visuals::healthbar;
    config_json["visuals"]["healthtext"] = globals::visuals::healthtext;
    config_json["visuals"]["name"] = globals::visuals::name;
    config_json["visuals"]["nametype"] = globals::visuals::nametype;
    config_json["visuals"]["toolesp"] = globals::visuals::toolesp;
    config_json["visuals"]["distance"] = globals::visuals::distance;
    config_json["visuals"]["skeletons"] = globals::visuals::skeletons;
    config_json["visuals"]["chinahat"] = globals::visuals::chinahat;
    config_json["visuals"]["chinahat_target_only"] = globals::visuals::chinahat_target_only;
    config_json["visuals"]["chinahat_color"] = std::vector<float>(globals::visuals::chinahat_color, globals::visuals::chinahat_color + 4);
    config_json["visuals"]["tracers"] = globals::visuals::tracers;
    config_json["visuals"]["tracers_glow"] = globals::visuals::tracers_glow;
    config_json["visuals"]["tracerstype"] = globals::visuals::tracerstype;
    config_json["visuals"]["sonar"] = globals::visuals::sonar;
    config_json["visuals"]["sonar_range"] = globals::visuals::sonar_range;
    config_json["visuals"]["sonar_thickness"] = globals::visuals::sonar_thickness;
    config_json["visuals"]["boxcolors"] = std::vector<float>(globals::visuals::boxcolors, globals::visuals::boxcolors + 4);
    config_json["visuals"]["boxfillcolor"] = std::vector<float>(globals::visuals::boxfillcolor, globals::visuals::boxfillcolor + 4);
    config_json["visuals"]["box_gradient_color1"] = std::vector<float>(globals::visuals::box_gradient_color1, globals::visuals::box_gradient_color1 + 4);
    config_json["visuals"]["box_gradient_color2"] = std::vector<float>(globals::visuals::box_gradient_color2, globals::visuals::box_gradient_color2 + 4);
    config_json["visuals"]["box_gradient"] = globals::visuals::box_gradient;
    config_json["visuals"]["box_gradient_rotation"] = globals::visuals::box_gradient_rotation;
    config_json["visuals"]["box_gradient_rotation_speed"] = globals::visuals::box_gradient_rotation_speed;
    config_json["visuals"]["namecolor"] = std::vector<float>(globals::visuals::namecolor, globals::visuals::namecolor + 4);
    config_json["visuals"]["healthbarcolor"] = std::vector<float>(globals::visuals::healthbarcolor, globals::visuals::healthbarcolor + 4);
    config_json["visuals"]["healthbarcolor1"] = std::vector<float>(globals::visuals::healthbarcolor1, globals::visuals::healthbarcolor1 + 4);
    config_json["visuals"]["distancecolor"] = std::vector<float>(globals::visuals::distancecolor, globals::visuals::distancecolor + 4);
    config_json["visuals"]["toolespcolor"] = std::vector<float>(globals::visuals::toolespcolor, globals::visuals::toolespcolor + 4);
    config_json["visuals"]["skeletonscolor"] = std::vector<float>(globals::visuals::skeletonscolor, globals::visuals::skeletonscolor + 4);
    config_json["visuals"]["tracerscolor"] = std::vector<float>(globals::visuals::tracerscolor, globals::visuals::tracerscolor + 4);
    config_json["visuals"]["sonarcolor"] = std::vector<float>(globals::visuals::sonarcolor, globals::visuals::sonarcolor + 4);
    config_json["visuals"]["sonar_dot_color"] = std::vector<float>(globals::visuals::sonar_dot_color, globals::visuals::sonar_dot_color + 4);
    config_json["visuals"]["fog"] = globals::visuals::fog;
    config_json["visuals"]["fog_start"] = globals::visuals::fog_start;
    config_json["visuals"]["fog_end"] = globals::visuals::fog_end;
    config_json["visuals"]["fog_color"] = std::vector<float>(globals::visuals::fog_color, globals::visuals::fog_color + 4);
    config_json["visuals"]["rainbow_fog"] = globals::visuals::rainbow_fog;
    config_json["visuals"]["rainbow_fog_speed"] = globals::visuals::rainbow_fog_speed;
    config_json["visuals"]["fog_glow"] = globals::visuals::fog_glow;
    config_json["visuals"]["fog_glow_intensity"] = globals::visuals::fog_glow_intensity;
    config_json["visuals"]["fog_glow_color"] = std::vector<float>(globals::visuals::fog_glow_color, globals::visuals::fog_glow_color + 4);
    config_json["visuals"]["lighting_modifications"] = globals::visuals::lighting_modifications;
    config_json["visuals"]["lighting_brightness"] = globals::visuals::lighting_brightness;
    config_json["visuals"]["lighting_contrast"] = globals::visuals::lighting_contrast;
    config_json["visuals"]["lighting_color_correction"] = std::vector<float>(globals::visuals::lighting_color_correction, globals::visuals::lighting_color_correction + 4);
    config_json["visuals"]["lighting_shadows"] = globals::visuals::lighting_shadows;
    config_json["visuals"]["lighting_ambient"] = std::vector<float>(globals::visuals::lighting_ambient, globals::visuals::lighting_ambient + 3);
    config_json["visuals"]["workspace_viewer"] = globals::visuals::workspace_viewer;
    config_json["visuals"]["workspace_show_position"] = globals::visuals::workspace_show_position;
    config_json["visuals"]["workspace_show_size"] = globals::visuals::workspace_show_size;
    config_json["visuals"]["workspace_show_velocity"] = globals::visuals::workspace_show_velocity;
    config_json["visuals"]["workspace_transparency_modifier"] = globals::visuals::workspace_transparency_modifier;
    config_json["visuals"]["workspace_transparency"] = globals::visuals::workspace_transparency;
    config_json["visuals"]["workspace_cancollide_modifier"] = globals::visuals::workspace_cancollide_modifier;
    config_json["visuals"]["workspace_cancollide_value"] = globals::visuals::workspace_cancollide_value;
    config_json["visuals"]["workspace_anchored_modifier"] = globals::visuals::workspace_anchored_modifier;
    config_json["visuals"]["workspace_anchored_value"] = globals::visuals::workspace_anchored_value;

    try {
        if (globals::visuals::box_overlay_flags && !globals::visuals::box_overlay_flags->empty())
            config_json["visuals"]["overlay"]["box"] = *globals::visuals::box_overlay_flags;
        if (globals::visuals::skeleton_overlay_flags && !globals::visuals::skeleton_overlay_flags->empty())
            config_json["visuals"]["overlay"]["skeleton"] = *globals::visuals::skeleton_overlay_flags;
        if (globals::visuals::healthbar_overlay_flags && !globals::visuals::healthbar_overlay_flags->empty())
            config_json["visuals"]["overlay"]["healthbar"] = *globals::visuals::healthbar_overlay_flags;
    } catch (...) {
        // ignore serialization issues
    }

    // Misc settings
    config_json["misc"]["speed"] = globals::misc::speed;
    config_json["misc"]["speedtype"] = globals::misc::speedtype;
    config_json["misc"]["speedvalue"] = globals::misc::speedvalue;
    config_json["misc"]["flight"] = globals::misc::flight;
    config_json["misc"]["flighttype"] = globals::misc::flighttype;
    config_json["misc"]["flightvalue"] = globals::misc::flightvalue;
    config_json["misc"]["spin360"] = globals::misc::spin360;
    config_json["misc"]["spin360speed"] = globals::misc::spin360speed;
    config_json["misc"]["autoreload"] = globals::misc::autoreload;
    config_json["misc"]["bikefly"] = globals::misc::bikefly;
    config_json["misc"]["vsync"] = globals::misc::vsync;
    config_json["misc"]["targethud"] = globals::misc::targethud;
    config_json["misc"]["playerlist"] = globals::misc::playerlist;
    config_json["misc"]["explorer"] = globals::misc::explorer;
    config_json["misc"]["override_overlay_fps"] = globals::misc::override_overlay_fps;
    config_json["misc"]["overlay_fps"] = globals::misc::overlay_fps;
    config_json["misc"]["streamproof"] = globals::misc::streamproof;
    config_json["misc"]["desync_visualizer"] = globals::misc::desync_visualizer;
    config_json["misc"]["desync_viz_color"] = std::vector<float>(globals::misc::desync_viz_color, globals::misc::desync_viz_color + 4);
    config_json["misc"]["npc_check"] = globals::misc::npc_check;

    try {
        config_json["misc"]["keybinds_data"]["speedkeybind"]["key"] = globals::misc::speedkeybind.key;
        config_json["misc"]["keybinds_data"]["speedkeybind"]["type"] = static_cast<int>(globals::misc::speedkeybind.type);
        config_json["misc"]["keybinds_data"]["flightkeybind"]["key"] = globals::misc::flightkeybind.key;
        config_json["misc"]["keybinds_data"]["flightkeybind"]["type"] = static_cast<int>(globals::misc::flightkeybind.type);
        config_json["misc"]["keybinds_data"]["spin360keybind"]["key"] = globals::misc::spin360keybind.key;
        config_json["misc"]["keybinds_data"]["spin360keybind"]["type"] = static_cast<int>(globals::misc::spin360keybind.type);
    }
    catch (...) {
        config_json["misc"]["keybinds_data"]["speedkeybind"]["key"] = 0;
        config_json["misc"]["keybinds_data"]["speedkeybind"]["type"] = 1;
        config_json["misc"]["keybinds_data"]["flightkeybind"]["key"] = 0;
        config_json["misc"]["keybinds_data"]["flightkeybind"]["type"] = 1;
        config_json["misc"]["keybinds_data"]["spin360keybind"]["key"] = 0;
        config_json["misc"]["keybinds_data"]["spin360keybind"]["type"] = 1;
    }

    std::string filepath = config_directory + "\\" + name + ".json";
    std::ofstream file(filepath);

    if (file.is_open()) {
        file << config_json.dump(2);
        file.close();

        refresh_config_list();
        current_config_name = name;
        std::cout << "[CONFIG] Successfully saved config: " << name << "\n";
        return true;
    }

    std::cout << "[CONFIG] Failed to save config: " << name << "\n";
    return false;
}

bool ConfigManager::load_config(const std::string& name) {
    if (name.empty()) return false;

    std::string filepath = config_directory + "\\" + name + ".json";
    std::ifstream file(filepath);

    if (file.is_open()) {
        try {
            json config_json;
            file >> config_json;

            std::cout << "[CONFIG] Loading config: " << name << "\n";

            if (config_json.contains("combat")) {
                auto& combat = config_json["combat"];
                if (combat.contains("aimbot")) globals::combat::aimbot = combat["aimbot"];
                if (combat.contains("stickyaim")) globals::combat::stickyaim = combat["stickyaim"];
                if (combat.contains("unlockondeath")) globals::combat::unlockondeath = combat["unlockondeath"];
                if (combat.contains("aimbottype")) globals::combat::aimbottype = combat["aimbottype"];
                if (combat.contains("nosleep_aimbot")) globals::combat::nosleep_aimbot = combat["nosleep_aimbot"];
                if (combat.contains("usefov")) globals::combat::usefov = combat["usefov"];
                if (combat.contains("drawfov")) globals::combat::drawfov = combat["drawfov"];
                if (combat.contains("fovsize")) globals::combat::fovsize = combat["fovsize"];
                if (combat.contains("fovcolor")) {
                    auto colors = combat["fovcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::combat::fovcolor[i] = colors[i];
                }
                if (combat.contains("smoothing")) globals::combat::smoothing = combat["smoothing"];
                if (combat.contains("smoothingx")) globals::combat::smoothingx = combat["smoothingx"];
                if (combat.contains("smoothingy")) globals::combat::smoothingy = combat["smoothingy"];
                if (combat.contains("smoothingstyle")) globals::combat::smoothingstyle = combat["smoothingstyle"];
                if (combat.contains("sensitivity_enabled")) globals::combat::sensitivity_enabled = combat["sensitivity_enabled"];
                if (combat.contains("cam_sensitivity")) globals::combat::cam_sensitivity = combat["cam_sensitivity"];
                if (combat.contains("camlock_shake")) globals::combat::camlock_shake = combat["camlock_shake"];
                if (combat.contains("camlock_shake_x")) globals::combat::camlock_shake_x = combat["camlock_shake_x"];
                if (combat.contains("camlock_shake_y")) globals::combat::camlock_shake_y = combat["camlock_shake_y"];
                if (combat.contains("camlock_shake_z")) globals::combat::camlock_shake_z = combat["camlock_shake_z"];
                if (combat.contains("mouse_sensitivity")) globals::combat::mouse_sensitivity = combat["mouse_sensitivity"];
                if (combat.contains("predictions")) globals::combat::predictions = combat["predictions"];
                if (combat.contains("predictionsx")) globals::combat::predictionsx = combat["predictionsx"];
                if (combat.contains("predictionsy")) globals::combat::predictionsy = combat["predictionsy"];
                if (combat.contains("deadzone")) globals::combat::deadzone = combat["deadzone"];
                if (combat.contains("deadzonex")) globals::combat::deadzonex = combat["deadzonex"];
                if (combat.contains("deadzoney")) globals::combat::deadzoney = combat["deadzoney"];
                if (combat.contains("teamcheck")) globals::combat::teamcheck = combat["teamcheck"];
                if (combat.contains("grabbedcheck")) globals::combat::grabbedcheck = combat["grabbedcheck"];
                if (combat.contains("arsenal_flick_fix")) globals::combat::arsenal_flick_fix = combat["arsenal_flick_fix"];
                if (combat.contains("aimpart")) globals::combat::aimpart = combat["aimpart"];
                if (combat.contains("airpart_enabled")) globals::combat::airpart_enabled = combat["airpart_enabled"];
                if (combat.contains("airpart")) globals::combat::airpart = combat["airpart"];
                // Load ignore parts for camera and mouse
                if (combat.contains("ignore_parts_camera")) {
                    auto v = combat["ignore_parts_camera"].get<std::vector<int>>();
                    if (globals::combat::ignore_parts_camera) *globals::combat::ignore_parts_camera = v;
                }
                if (combat.contains("ignore_parts_mouse")) {
                    auto v = combat["ignore_parts_mouse"].get<std::vector<int>>();
                    if (globals::combat::ignore_parts_mouse) *globals::combat::ignore_parts_mouse = v;
                }
                if (combat.contains("target_method")) globals::combat::target_method = combat["target_method"];
                if (combat.contains("silentaim")) globals::combat::silentaim = combat["silentaim"];
                if (combat.contains("stickyaimsilent")) globals::combat::stickyaimsilent = combat["stickyaimsilent"];
                if (combat.contains("spoofmouse")) globals::combat::spoofmouse = combat["spoofmouse"];
                if (combat.contains("hitchance")) globals::combat::hitchance = combat["hitchance"];
                if (combat.contains("closestpartsilent")) globals::combat::closestpartsilent = combat["closestpartsilent"];
                if (combat.contains("silentaimpart")) globals::combat::silentaimpart = combat["silentaimpart"];
                if (combat.contains("usesfov")) globals::combat::usesfov = combat["usesfov"];
                if (combat.contains("drawsfov")) globals::combat::drawsfov = combat["drawsfov"];
                if (combat.contains("sfovsize")) globals::combat::sfovsize = combat["sfovsize"];
                if (combat.contains("sfovcolor")) {
                    auto colors = combat["sfovcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::combat::sfovcolor[i] = colors[i];
                }
                if (combat.contains("silentpredictions")) globals::combat::silentpredictions = combat["silentpredictions"];
                if (combat.contains("silentpredictionsx")) globals::combat::silentpredictionsx = combat["silentpredictionsx"];
                if (combat.contains("silentpredictionsy")) globals::combat::silentpredictionsy = combat["silentpredictionsy"];
                if (combat.contains("triggerbot")) globals::combat::triggerbot = combat["triggerbot"];
                if (combat.contains("delay")) globals::combat::delay = combat["delay"];

                if (combat.contains("keybinds")) {
                    auto& keybinds = combat["keybinds"];
                    if (keybinds.contains("aimbotkeybind")) {
                        if (keybinds["aimbotkeybind"].contains("key"))
                            globals::combat::aimbotkeybind.key = keybinds["aimbotkeybind"]["key"];
                        if (keybinds["aimbotkeybind"].contains("type"))
                            globals::combat::aimbotkeybind.type = static_cast<keybind::c_keybind_type>(keybinds["aimbotkeybind"]["type"].get<int>());
                    }
                    if (keybinds.contains("silentaimkeybind")) {
                        if (keybinds["silentaimkeybind"].contains("key"))
                            globals::combat::silentaimkeybind.key = keybinds["silentaimkeybind"]["key"];
                        if (keybinds["silentaimkeybind"].contains("type"))
                            globals::combat::silentaimkeybind.type = static_cast<keybind::c_keybind_type>(keybinds["silentaimkeybind"]["type"].get<int>());
                    }
                    if (keybinds.contains("triggerbotkeybind")) {
                        if (keybinds["triggerbotkeybind"].contains("key"))
                            globals::combat::triggerbotkeybind.key = keybinds["triggerbotkeybind"]["key"];
                        if (keybinds["triggerbotkeybind"].contains("type"))
                            globals::combat::triggerbotkeybind.type = static_cast<keybind::c_keybind_type>(keybinds["triggerbotkeybind"]["type"].get<int>());
                    }
                }
                if (combat.contains("crew_check")) globals::combat::crew_check = combat["crew_check"];
            }

            if (config_json.contains("visuals")) {
                auto& visuals = config_json["visuals"];
                if (visuals.contains("visuals")) globals::visuals::visuals = visuals["visuals"];
                if (visuals.contains("boxes")) globals::visuals::boxes = visuals["boxes"];
                if (visuals.contains("lockedindicator")) globals::visuals::lockedindicator = visuals["lockedindicator"];
                if (visuals.contains("boxtype")) globals::visuals::boxtype = visuals["boxtype"];
                if (visuals.contains("healthbar")) globals::visuals::healthbar = visuals["healthbar"];
                if (visuals.contains("healthtext")) globals::visuals::healthtext = visuals["healthtext"];
                if (visuals.contains("name")) globals::visuals::name = visuals["name"];
                if (visuals.contains("nametype")) globals::visuals::nametype = visuals["nametype"];
                if (visuals.contains("toolesp")) globals::visuals::toolesp = visuals["toolesp"];
                if (visuals.contains("distance")) globals::visuals::distance = visuals["distance"];
                if (visuals.contains("skeletons")) globals::visuals::skeletons = visuals["skeletons"];
                if (visuals.contains("chinahat")) globals::visuals::chinahat = visuals["chinahat"];
                if (visuals.contains("chinahat_target_only")) globals::visuals::chinahat_target_only = visuals["chinahat_target_only"];
                if (visuals.contains("chinahat_color")) {
                    auto color = visuals["chinahat_color"];
                    if (color.is_array() && color.size() >= 4) {
                        globals::visuals::chinahat_color[0] = color[0];
                        globals::visuals::chinahat_color[1] = color[1];
                        globals::visuals::chinahat_color[2] = color[2];
                        globals::visuals::chinahat_color[3] = color[3];
                    }
                }
                if (visuals.contains("tracers")) globals::visuals::tracers = visuals["tracers"];
                if (visuals.contains("tracers_glow")) globals::visuals::tracers_glow = visuals["tracers_glow"];
                if (visuals.contains("tracerstype")) globals::visuals::tracerstype = visuals["tracerstype"];
                if (visuals.contains("sonar")) globals::visuals::sonar = visuals["sonar"];
                if (visuals.contains("sonar_range")) globals::visuals::sonar_range = visuals["sonar_range"];
                if (visuals.contains("sonar_thickness")) globals::visuals::sonar_thickness = visuals["sonar_thickness"];
                if (visuals.contains("boxcolors")) {
                    auto colors = visuals["boxcolors"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::boxcolors[i] = colors[i];
                }
                if (visuals.contains("boxfillcolor")) {
                    auto colors = visuals["boxfillcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::boxfillcolor[i] = colors[i];
                }
                if (visuals.contains("box_gradient")) globals::visuals::box_gradient = visuals["box_gradient"];
                if (visuals.contains("box_gradient_rotation")) globals::visuals::box_gradient_rotation = visuals["box_gradient_rotation"];
                if (visuals.contains("box_gradient_rotation_speed")) globals::visuals::box_gradient_rotation_speed = visuals["box_gradient_rotation_speed"];
                if (visuals.contains("box_gradient_color1")) {
                    auto colors = visuals["box_gradient_color1"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::box_gradient_color1[i] = colors[i];
                }
                if (visuals.contains("box_gradient_color2")) {
                    auto colors = visuals["box_gradient_color2"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::box_gradient_color2[i] = colors[i];
                }
                if (visuals.contains("namecolor")) {
                    auto colors = visuals["namecolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::namecolor[i] = colors[i];
                }
                if (visuals.contains("healthbarcolor")) {
                    auto colors = visuals["healthbarcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::healthbarcolor[i] = colors[i];
                }
                if (visuals.contains("healthbarcolor1")) {
                    auto colors = visuals["healthbarcolor1"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::healthbarcolor1[i] = colors[i];
                }
                if (visuals.contains("distancecolor")) {
                    auto colors = visuals["distancecolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::distancecolor[i] = colors[i];
                }
                if (visuals.contains("toolespcolor")) {
                    auto colors = visuals["toolespcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::toolespcolor[i] = colors[i];
                }
                if (visuals.contains("skeletonscolor")) {
                    auto colors = visuals["skeletonscolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::skeletonscolor[i] = colors[i];
                }
                if (visuals.contains("tracerscolor")) {
                    auto colors = visuals["tracerscolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::tracerscolor[i] = colors[i];
                }
                if (visuals.contains("sonarcolor")) {
                    auto colors = visuals["sonarcolor"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::sonarcolor[i] = colors[i];
                }
                if (visuals.contains("sonar_dot_color")) {
                    auto colors = visuals["sonar_dot_color"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::sonar_dot_color[i] = colors[i];
                }
                if (visuals.contains("fog")) globals::visuals::fog = visuals["fog"];
                if (visuals.contains("fog_start")) globals::visuals::fog_start = visuals["fog_start"];
                if (visuals.contains("fog_end")) globals::visuals::fog_end = visuals["fog_end"];
                if (visuals.contains("fog_color")) {
                    auto colors = visuals["fog_color"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::fog_color[i] = colors[i];
                }
                if (visuals.contains("rainbow_fog")) globals::visuals::rainbow_fog = visuals["rainbow_fog"];
                if (visuals.contains("rainbow_fog_speed")) globals::visuals::rainbow_fog_speed = visuals["rainbow_fog_speed"];
                if (visuals.contains("fog_glow")) globals::visuals::fog_glow = visuals["fog_glow"];
                if (visuals.contains("fog_glow_intensity")) globals::visuals::fog_glow_intensity = visuals["fog_glow_intensity"];
                if (visuals.contains("fog_glow_color")) {
                    auto colors = visuals["fog_glow_color"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::fog_glow_color[i] = colors[i];
                }
                if (visuals.contains("lighting_modifications")) globals::visuals::lighting_modifications = visuals["lighting_modifications"];
                if (visuals.contains("lighting_brightness")) globals::visuals::lighting_brightness = visuals["lighting_brightness"];
                if (visuals.contains("lighting_contrast")) globals::visuals::lighting_contrast = visuals["lighting_contrast"];
                if (visuals.contains("lighting_color_correction")) {
                    auto colors = visuals["lighting_color_correction"].get<std::vector<float>>();
                    for (int i = 0; i < 4 && i < colors.size(); i++) globals::visuals::lighting_color_correction[i] = colors[i];
                }
                if (visuals.contains("lighting_shadows")) globals::visuals::lighting_shadows = visuals["lighting_shadows"];
                if (visuals.contains("lighting_ambient")) {
                    auto colors = visuals["lighting_ambient"].get<std::vector<float>>();
                    for (int i = 0; i < 3 && i < colors.size(); i++) globals::visuals::lighting_ambient[i] = colors[i];
                }
                if (visuals.contains("workspace_viewer")) globals::visuals::workspace_viewer = visuals["workspace_viewer"];
                if (visuals.contains("workspace_show_position")) globals::visuals::workspace_show_position = visuals["workspace_show_position"];
                if (visuals.contains("workspace_show_size")) globals::visuals::workspace_show_size = visuals["workspace_show_size"];
                if (visuals.contains("workspace_show_velocity")) globals::visuals::workspace_show_velocity = visuals["workspace_show_velocity"];
                if (visuals.contains("workspace_transparency_modifier")) globals::visuals::workspace_transparency_modifier = visuals["workspace_transparency_modifier"];
                if (visuals.contains("workspace_transparency")) globals::visuals::workspace_transparency = visuals["workspace_transparency"];
                if (visuals.contains("workspace_cancollide_modifier")) globals::visuals::workspace_cancollide_modifier = visuals["workspace_cancollide_modifier"];
                if (visuals.contains("workspace_cancollide_value")) globals::visuals::workspace_cancollide_value = visuals["workspace_cancollide_value"];
                if (visuals.contains("workspace_anchored_modifier")) globals::visuals::workspace_anchored_modifier = visuals["workspace_anchored_modifier"];
                if (visuals.contains("workspace_anchored_value")) globals::visuals::workspace_anchored_value = visuals["workspace_anchored_value"];

                if (visuals.contains("overlay")) {
                    auto& ov = visuals["overlay"];
                    try {
                        if (ov.contains("box")) {
                            auto v = ov["box"].get<std::vector<int>>();
                            if (globals::visuals::box_overlay_flags) *globals::visuals::box_overlay_flags = v;
                        }
                        if (ov.contains("skeleton")) {
                            auto v = ov["skeleton"].get<std::vector<int>>();
                            if (globals::visuals::skeleton_overlay_flags) *globals::visuals::skeleton_overlay_flags = v;
                        }
                        if (ov.contains("healthbar")) {
                            auto v = ov["healthbar"].get<std::vector<int>>();
                            if (globals::visuals::healthbar_overlay_flags) *globals::visuals::healthbar_overlay_flags = v;
                        }
                    } catch (...) {
                        // ignore parse issues
                    }
                }
            }

            if (config_json.contains("misc")) {
                auto& misc = config_json["misc"];
                if (misc.contains("speed")) globals::misc::speed = misc["speed"];
                if (misc.contains("speedtype")) globals::misc::speedtype = misc["speedtype"];
                if (misc.contains("speedvalue")) globals::misc::speedvalue = misc["speedvalue"];
                if (misc.contains("flight")) globals::misc::flight = misc["flight"];
                if (misc.contains("flighttype")) globals::misc::flighttype = misc["flighttype"];
                if (misc.contains("flightvalue")) globals::misc::flightvalue = misc["flightvalue"];
                if (misc.contains("spin360")) globals::misc::spin360 = misc["spin360"];
                if (misc.contains("spin360speed")) globals::misc::spin360speed = misc["spin360speed"];
                if (misc.contains("autoreload")) globals::misc::autoreload = misc["autoreload"];
                if (misc.contains("bikefly")) globals::misc::bikefly = misc["bikefly"];
                if (misc.contains("vsync")) globals::misc::vsync = misc["vsync"];
                if (misc.contains("targethud")) globals::misc::targethud = misc["targethud"];
                if (misc.contains("playerlist")) globals::misc::playerlist = misc["playerlist"];
                if (misc.contains("explorer")) globals::misc::explorer = misc["explorer"];
                if (misc.contains("override_overlay_fps")) globals::misc::override_overlay_fps = misc["override_overlay_fps"];
                if (misc.contains("overlay_fps")) globals::misc::overlay_fps = misc["overlay_fps"];
                if (misc.contains("streamproof")) globals::misc::streamproof = misc["streamproof"];
                if (misc.contains("desync_visualizer")) globals::misc::desync_visualizer = misc["desync_visualizer"];
                if (misc.contains("npc_check")) globals::misc::npc_check = misc["npc_check"];
                if (misc.contains("desync_viz_color")) {
                    auto color_array = misc["desync_viz_color"];
                    if (color_array.is_array() && color_array.size() >= 4) {
                        globals::misc::desync_viz_color[0] = color_array[0];
                        globals::misc::desync_viz_color[1] = color_array[1];
                        globals::misc::desync_viz_color[2] = color_array[2];
                        globals::misc::desync_viz_color[3] = color_array[3];
                    }
                }

                if (misc.contains("keybinds_data")) {
                    auto& keybinds = misc["keybinds_data"];
                    if (keybinds.contains("speedkeybind")) {
                        if (keybinds["speedkeybind"].contains("key"))
                            globals::misc::speedkeybind.key = keybinds["speedkeybind"]["key"];
                        if (keybinds["speedkeybind"].contains("type"))
                            globals::misc::speedkeybind.type = static_cast<keybind::c_keybind_type>(keybinds["speedkeybind"]["type"].get<int>());
                    }
                    if (keybinds.contains("flightkeybind")) {
                        if (keybinds["flightkeybind"].contains("key"))
                            globals::misc::flightkeybind.key = keybinds["flightkeybind"]["key"];
                        if (keybinds["flightkeybind"].contains("type"))
                            globals::misc::flightkeybind.type = static_cast<keybind::c_keybind_type>(keybinds["flightkeybind"]["type"].get<int>());
                    }
                    if (keybinds.contains("spin360keybind")) {
                        if (keybinds["spin360keybind"].contains("key"))
                            globals::misc::spin360keybind.key = keybinds["spin360keybind"]["key"];
                        if (keybinds["spin360keybind"].contains("type"))
                            globals::misc::spin360keybind.type = static_cast<keybind::c_keybind_type>(keybinds["spin360keybind"]["type"].get<int>());
                    }
                }
            }

            file.close();
            current_config_name = name;
            std::cout << "[CONFIG] Successfully loaded config: " << name << "\n";
            return true;
        }
        catch (const json::parse_error& e) {
            std::cout << "[CONFIG] Failed to parse config file: " << e.what() << "\n";
            file.close();
            return false;
        }
    }

    std::cout << "[CONFIG] Failed to open config file: " << name << "\n";
    return false;
}

bool ConfigManager::delete_config(const std::string& name) {
    if (name.empty()) return false;

    std::string filepath = config_directory + "\\" + name + ".json";

    if (fs::exists(filepath)) {
        fs::remove(filepath);
        refresh_config_list();

        if (current_config_name == name) {
            current_config_name.clear();
        }

        return true;
    }

    return false;
}

void ConfigManager::render_config_ui(float width, float height) {
    // Calculate sizes: config list box takes most space, input and buttons at bottom
    float input_height = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
    float button_height = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y;
    float buttons_area_height = button_height * 2 + ImGui::GetStyle().ItemSpacing.y; // 2 rows of buttons
    float list_height = height - input_height - buttons_area_height - ImGui::GetStyle().ItemSpacing.y * 2;
    
    // Config list box - only shows configs (no highlight)
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    // Disable hover and active highlights
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
    ImGui::BeginChild("config_list", ImVec2(width, list_height), true);
    {
        if (!config_files.empty()) {
            for (const auto& config : config_files) {
                // Use Selectable without selection highlighting and no hover/active states
                if (ImGui::Selectable(config.c_str(), false, ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_DontClosePopups)) {
                    std::cout << "[CONFIG] Selected config: " << config << "\n";
                    strcpy_s(config_name_buffer, sizeof(config_name_buffer), config.c_str());
                    current_config_name = config;
                    Notifications::Info("Selected config: " + config, 2.0f);
                }
            }
        }
        else {
            ImVec2 text_size = ImGui::CalcTextSize("No configs found");
            ImVec2 window_size = ImGui::GetWindowSize();
            ImGui::SetCursorPos(ImVec2((window_size.x - text_size.x) * 0.5f, (window_size.y - text_size.y) * 0.5f));
            ImGui::TextDisabled("No configs found");
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    
    // Config name input field
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##config_name", config_name_buffer, sizeof(config_name_buffer));
    
    ImGui::Spacing();
    
    // Buttons in 2x2 grid: Delete, Load, Save (3 buttons)
    // First row: Delete (left), Load (right)
    bool delete_pressed = gui->button("Delete", 2);
    ImGui::SameLine();
    bool load_pressed = gui->button("Load", 2);
    
    if (delete_pressed) {
        std::cout << "[CONFIG] Delete button pressed" << "\n";
        if (strlen(config_name_buffer) > 0) {
            std::string config_name = std::string(config_name_buffer);
            if (delete_config(config_name)) {
                Notifications::Success("Config was deleted!");
                memset(config_name_buffer, 0, sizeof(config_name_buffer));
                current_config_name.clear();
                refresh_config_list();
            }
            else {
                Notifications::Error("Failed To Delete!");
            }
        }
        else {
            Notifications::Warning("Select A Config!");
        }
    }
    
    if (load_pressed) {
        std::cout << "[CONFIG] Load button pressed" << "\n";
        if (strlen(config_name_buffer) > 0) {
            if (load_config(std::string(config_name_buffer))) {
                Notifications::Success("Config '" + std::string(config_name_buffer) + "' loaded successfully!");
                current_config_name = std::string(config_name_buffer);
            }
            else {
                Notifications::Error("Failed to load config '" + std::string(config_name_buffer) + "'!");
            }
        }
        else {
            Notifications::Warning("Please enter a config name or select from list!");
        }
    }
    
    ImGui::Spacing();
    
    // Second row: Save (left), Open Folder (right)
    bool save_pressed = gui->button("Save", 2);
    ImGui::SameLine();
    bool open_folder_pressed = gui->button("Open Folder", 2);
    
    if (save_pressed) {
        std::cout << "[CONFIG] Save button pressed" << "\n";
        if (strlen(config_name_buffer) > 0) {
            if (save_config(std::string(config_name_buffer))) {
                Notifications::Success("Config '" + std::string(config_name_buffer) + "' saved successfully!");
            }
            else {
                Notifications::Error("Failed to save config '" + std::string(config_name_buffer) + "'!");
            }
        }
        else {
            Notifications::Warning("Please enter a config name!");
        }
    }
    
    if (open_folder_pressed) {
        std::cout << "[CONFIG] Open Folder button pressed" << "\n";
        // Ensure directory exists
        if (!fs::exists(config_directory)) {
            fs::create_directories(config_directory);
        }
        // Open folder in Windows Explorer
        ShellExecuteA(NULL, "open", config_directory.c_str(), NULL, NULL, SW_SHOWDEFAULT);
        Notifications::Info("Opening config folder...", 1.5f);
    }
}

const std::string& ConfigManager::get_current_config() const {
    return current_config_name;
}
