#pragma once
#define NOMINMAX
#include "../classes/classes.h"
#include "../../drawing/imgui/imgui.h"
#include <vector>
#include <string>
#include <chrono>

class MeleeManager {
public:
    MeleeManager();
    ~MeleeManager();

    void Initialize();
    void Update();
    void Render(); // Empty stub for compatibility
    
    // Get melee list for dropdown
    std::vector<std::string>& GetMeleeList() { return all_melees; }
    int GetSelectedIndex() const;
    void SetSelectedIndex(int index);
    void SetSelectedMelee(const std::string& melee);
    bool IsSwingFixEnabled() const { return state.swing_fix_enabled; }
    void SetSwingFixEnabled(bool enabled) { state.swing_fix_enabled = enabled; }
    
    // Public access to melee list for GUI
    std::vector<std::string> all_melees;

private:
    // Configuration
    struct Config {
        ImVec2 pos = ImVec2(200, 200);
        ImVec2 size = ImVec2(500, 350);
        
        struct Colors {
            ImU32 bg = IM_COL32(20, 20, 20, 255);
            ImU32 bg_dark = IM_COL32(12, 12, 12, 255);
            ImU32 border1 = IM_COL32(60, 60, 60, 255);
            ImU32 border2 = IM_COL32(0, 0, 0, 255);
            ImU32 accent = IM_COL32(165, 205, 65, 255);
            ImU32 text = IM_COL32(235, 235, 235, 255);
            ImU32 text_dim = IM_COL32(150, 150, 150, 255);
            ImU32 groupbox_bg = IM_COL32(23, 23, 23, 255);
        } colors;
        
        struct List {
            int max_visible = 13;
            float item_height = 18.0f;
        } list;
    } cfg;

    // State
    struct State {
        bool menu_open = true;
        bool last_f1 = false;
        
        bool dragging = false;
        ImVec2 drag_start = ImVec2(0, 0);
        ImVec2 drag_offset = ImVec2(0, 0);
        
        int scroll_offset = 0;
        std::vector<std::string> filtered_list;
        
        std::string search_text = "";
        bool search_focused = false;
        int debounce = 0;
        
        bool last_mouse_down = false;
        bool scrolling = false;
        
        bool swing_fix_enabled = true;
        std::string user_selected_melee = "";
        bool swing_fix_debounce = false;
        std::chrono::steady_clock::time_point swing_fix_time;
    } state;

    // Roblox instances
    roblox::instance player_data;
    roblox::instance melee_value;
    roblox::instance melees_folder;
    roblox::instance player_gui;
    
    // Data
    std::string default_melee = "Dagger";
    std::string last_known_valid_melee = "";

    // Helper functions
    bool IsMouseOver(const ImVec2& pos, const ImVec2& size);
    void SetMenuVisible(bool visible);
    void UpdateFilter();
    void HandleTyping();
    void DrawUI();
    void DrawMainWindow();
    void DrawGradientBar();
    void DrawTabArea();
    void DrawGroupbox();
    void DrawSearchBar();
    void DrawList();
    void DrawScrollbar();
    
    // Roblox access
    bool GetPlayerData();
    bool GetMeleesFolder();
    std::string GetMeleeValue();
    void SetMeleeValue(const std::string& value);
    std::string GetGunText();
    bool IsValidMelee(const std::string& melee);
    
    // Swing fix logic
    void UpdateSwingFix();
};

