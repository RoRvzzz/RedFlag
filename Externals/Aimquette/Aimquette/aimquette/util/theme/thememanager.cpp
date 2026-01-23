#include "thememanager.h"
#include "../notification/notification.h"
#include "../../drawing/framework/settings/colors.h"
#include <algorithm>
#include <shellapi.h>
#include <iostream>

ThemeManager::ThemeManager() {
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
    
    // Set theme directory to "themes" folder next to the executable
    if (!exe_dir.empty()) {
        theme_directory = exe_dir + "themes";
    } else {
        // Fallback to current directory if we can't get exe path
        theme_directory = "themes";
    }

    if (!fs::exists(theme_directory)) {
        fs::create_directories(theme_directory);
    }

    refresh_theme_list();
}

void ThemeManager::refresh_theme_list() {
    theme_files.clear();

    if (fs::exists(theme_directory)) {
        for (const auto& entry : fs::directory_iterator(theme_directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                theme_files.push_back(entry.path().stem().string());
            }
        }
    }
    
    std::sort(theme_files.begin(), theme_files.end());
}

bool ThemeManager::export_theme(const std::string& name) {
    if (name.empty()) return false;

    json theme_json;

    // Export all color values (RGB values 0-255)
    theme_json["accent"]["r"] = static_cast<int>(clr->accent.Value.x * 255.0f);
    theme_json["accent"]["g"] = static_cast<int>(clr->accent.Value.y * 255.0f);
    theme_json["accent"]["b"] = static_cast<int>(clr->accent.Value.z * 255.0f);
    theme_json["accent"]["a"] = static_cast<int>(clr->accent.Value.w * 255.0f);

    theme_json["window"]["background_one"]["r"] = static_cast<int>(clr->window.background_one.Value.x * 255.0f);
    theme_json["window"]["background_one"]["g"] = static_cast<int>(clr->window.background_one.Value.y * 255.0f);
    theme_json["window"]["background_one"]["b"] = static_cast<int>(clr->window.background_one.Value.z * 255.0f);
    theme_json["window"]["background_one"]["a"] = static_cast<int>(clr->window.background_one.Value.w * 255.0f);

    theme_json["window"]["background_two"]["r"] = static_cast<int>(clr->window.background_two.Value.x * 255.0f);
    theme_json["window"]["background_two"]["g"] = static_cast<int>(clr->window.background_two.Value.y * 255.0f);
    theme_json["window"]["background_two"]["b"] = static_cast<int>(clr->window.background_two.Value.z * 255.0f);
    theme_json["window"]["background_two"]["a"] = static_cast<int>(clr->window.background_two.Value.w * 255.0f);

    theme_json["window"]["stroke"]["r"] = static_cast<int>(clr->window.stroke.Value.x * 255.0f);
    theme_json["window"]["stroke"]["g"] = static_cast<int>(clr->window.stroke.Value.y * 255.0f);
    theme_json["window"]["stroke"]["b"] = static_cast<int>(clr->window.stroke.Value.z * 255.0f);
    theme_json["window"]["stroke"]["a"] = static_cast<int>(clr->window.stroke.Value.w * 255.0f);

    theme_json["widgets"]["stroke_two"]["r"] = static_cast<int>(clr->widgets.stroke_two.Value.x * 255.0f);
    theme_json["widgets"]["stroke_two"]["g"] = static_cast<int>(clr->widgets.stroke_two.Value.y * 255.0f);
    theme_json["widgets"]["stroke_two"]["b"] = static_cast<int>(clr->widgets.stroke_two.Value.z * 255.0f);
    theme_json["widgets"]["stroke_two"]["a"] = static_cast<int>(clr->widgets.stroke_two.Value.w * 255.0f);

    theme_json["widgets"]["text"]["r"] = static_cast<int>(clr->widgets.text.Value.x * 255.0f);
    theme_json["widgets"]["text"]["g"] = static_cast<int>(clr->widgets.text.Value.y * 255.0f);
    theme_json["widgets"]["text"]["b"] = static_cast<int>(clr->widgets.text.Value.z * 255.0f);
    theme_json["widgets"]["text"]["a"] = static_cast<int>(clr->widgets.text.Value.w * 255.0f);

    theme_json["widgets"]["text_inactive"]["r"] = static_cast<int>(clr->widgets.text_inactive.Value.x * 255.0f);
    theme_json["widgets"]["text_inactive"]["g"] = static_cast<int>(clr->widgets.text_inactive.Value.y * 255.0f);
    theme_json["widgets"]["text_inactive"]["b"] = static_cast<int>(clr->widgets.text_inactive.Value.z * 255.0f);
    theme_json["widgets"]["text_inactive"]["a"] = static_cast<int>(clr->widgets.text_inactive.Value.w * 255.0f);

    std::string filepath = theme_directory + "\\" + name + ".json";
    std::ofstream file(filepath);

    if (file.is_open()) {
        file << theme_json.dump(2);
        file.close();

        refresh_theme_list();
        std::cout << "[THEME] Successfully exported theme: " << name << "\n";
        Notifications::Success("Theme '" + name + "' exported successfully!");
        return true;
    }

    std::cout << "[THEME] Failed to export theme: " << name << "\n";
    Notifications::Error("Failed to export theme '" + name + "'!");
    return false;
}

bool ThemeManager::import_theme(const std::string& name) {
    if (name.empty()) return false;

    std::string filepath = theme_directory + "\\" + name + ".json";
    std::ifstream file(filepath);

    if (file.is_open()) {
        try {
            json theme_json;
            file >> theme_json;

            std::cout << "[THEME] Loading theme: " << name << "\n";

            // Import accent color
            if (theme_json.contains("accent")) {
                auto& accent = theme_json["accent"];
                if (accent.contains("r") && accent.contains("g") && accent.contains("b")) {
                    clr->accent.Value.x = accent["r"].get<int>() / 255.0f;
                    clr->accent.Value.y = accent["g"].get<int>() / 255.0f;
                    clr->accent.Value.z = accent["b"].get<int>() / 255.0f;
                    if (accent.contains("a")) {
                        clr->accent.Value.w = accent["a"].get<int>() / 255.0f;
                    }
                }
            }

            // Import window colors
            if (theme_json.contains("window")) {
                auto& window = theme_json["window"];
                
                if (window.contains("background_one")) {
                    auto& bg_one = window["background_one"];
                    if (bg_one.contains("r") && bg_one.contains("g") && bg_one.contains("b")) {
                        clr->window.background_one.Value.x = bg_one["r"].get<int>() / 255.0f;
                        clr->window.background_one.Value.y = bg_one["g"].get<int>() / 255.0f;
                        clr->window.background_one.Value.z = bg_one["b"].get<int>() / 255.0f;
                        if (bg_one.contains("a")) {
                            clr->window.background_one.Value.w = bg_one["a"].get<int>() / 255.0f;
                        }
                    }
                }

                if (window.contains("background_two")) {
                    auto& bg_two = window["background_two"];
                    if (bg_two.contains("r") && bg_two.contains("g") && bg_two.contains("b")) {
                        clr->window.background_two.Value.x = bg_two["r"].get<int>() / 255.0f;
                        clr->window.background_two.Value.y = bg_two["g"].get<int>() / 255.0f;
                        clr->window.background_two.Value.z = bg_two["b"].get<int>() / 255.0f;
                        if (bg_two.contains("a")) {
                            clr->window.background_two.Value.w = bg_two["a"].get<int>() / 255.0f;
                        }
                    }
                }

                if (window.contains("stroke")) {
                    auto& stroke = window["stroke"];
                    if (stroke.contains("r") && stroke.contains("g") && stroke.contains("b")) {
                        clr->window.stroke.Value.x = stroke["r"].get<int>() / 255.0f;
                        clr->window.stroke.Value.y = stroke["g"].get<int>() / 255.0f;
                        clr->window.stroke.Value.z = stroke["b"].get<int>() / 255.0f;
                        if (stroke.contains("a")) {
                            clr->window.stroke.Value.w = stroke["a"].get<int>() / 255.0f;
                        }
                    }
                }
            }

            // Import widgets colors
            if (theme_json.contains("widgets")) {
                auto& widgets = theme_json["widgets"];
                
                if (widgets.contains("stroke_two")) {
                    auto& stroke_two = widgets["stroke_two"];
                    if (stroke_two.contains("r") && stroke_two.contains("g") && stroke_two.contains("b")) {
                        clr->widgets.stroke_two.Value.x = stroke_two["r"].get<int>() / 255.0f;
                        clr->widgets.stroke_two.Value.y = stroke_two["g"].get<int>() / 255.0f;
                        clr->widgets.stroke_two.Value.z = stroke_two["b"].get<int>() / 255.0f;
                        if (stroke_two.contains("a")) {
                            clr->widgets.stroke_two.Value.w = stroke_two["a"].get<int>() / 255.0f;
                        }
                    }
                }

                if (widgets.contains("text")) {
                    auto& text = widgets["text"];
                    if (text.contains("r") && text.contains("g") && text.contains("b")) {
                        clr->widgets.text.Value.x = text["r"].get<int>() / 255.0f;
                        clr->widgets.text.Value.y = text["g"].get<int>() / 255.0f;
                        clr->widgets.text.Value.z = text["b"].get<int>() / 255.0f;
                        if (text.contains("a")) {
                            clr->widgets.text.Value.w = text["a"].get<int>() / 255.0f;
                        }
                    }
                }

                if (widgets.contains("text_inactive")) {
                    auto& text_inactive = widgets["text_inactive"];
                    if (text_inactive.contains("r") && text_inactive.contains("g") && text_inactive.contains("b")) {
                        clr->widgets.text_inactive.Value.x = text_inactive["r"].get<int>() / 255.0f;
                        clr->widgets.text_inactive.Value.y = text_inactive["g"].get<int>() / 255.0f;
                        clr->widgets.text_inactive.Value.z = text_inactive["b"].get<int>() / 255.0f;
                        if (text_inactive.contains("a")) {
                            clr->widgets.text_inactive.Value.w = text_inactive["a"].get<int>() / 255.0f;
                        }
                    }
                }
            }

            file.close();
            std::cout << "[THEME] Successfully imported theme: " << name << "\n";
            Notifications::Success("Theme '" + name + "' imported successfully!");
            return true;
        }
        catch (const json::parse_error& e) {
            std::cout << "[THEME] Failed to parse theme file: " << e.what() << "\n";
            file.close();
            Notifications::Error("Failed to parse theme file!");
            return false;
        }
        catch (const std::exception& e) {
            std::cout << "[THEME] Failed to import theme: " << e.what() << "\n";
            file.close();
            Notifications::Error("Failed to import theme!");
            return false;
        }
    }

    std::cout << "[THEME] Failed to open theme file: " << name << "\n";
    Notifications::Error("Theme file '" + name + "' not found!");
    return false;
}

std::vector<std::string> ThemeManager::get_theme_list() {
    return theme_files;
}

std::string ThemeManager::get_theme_directory() const {
    return theme_directory;
}

