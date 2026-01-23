#pragma once

#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "../json/json.h"
#include "../../drawing/framework/settings/colors.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

class ThemeManager {
private:
    std::string theme_directory;
    std::vector<std::string> theme_files;
    char theme_name_buffer[256] = "";

public:
    ThemeManager();
    void refresh_theme_list();
    bool export_theme(const std::string& name);
    bool import_theme(const std::string& name);
    std::vector<std::string> get_theme_list();
    std::string get_theme_directory() const;
};





