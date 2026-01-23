#pragma once

#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "../json/json.h"
#include "../../drawing/imgui/addons/imgui_addons.h"
#include "../../drawing/framework/settings/functions.h"
#include "../globals.h"

using json = nlohmann::json;

namespace fs = std::filesystem;

class ConfigManager {
private:
    std::string config_directory;
    std::vector<std::string> config_files;
    std::string current_config_name;
    char config_name_buffer[256] = "";
    
    // Keybinds for config buttons
    int delete_keybind = 0;
    int delete_keybind_mode = 1;
    int load_keybind = 0;
    int load_keybind_mode = 1;
    int save_keybind = 0;
    int save_keybind_mode = 1;
    int open_folder_keybind = 0;
    int open_folder_keybind_mode = 1;

public:
    ConfigManager();
    void refresh_config_list();
    bool save_config(const std::string& name);
    bool load_config(const std::string& name);
    bool delete_config(const std::string& name);
    void render_config_ui(float width, float height);
    const std::string& get_current_config() const;
};
