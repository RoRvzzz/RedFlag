#define NOMINMAX
#include "meleemanager.h"
#include "../globals.h"
#include "../driver/driver.h"
#include "../offsets.h"
#include "../../drawing/overlay/overlay.h"
#include <algorithm>
#include <cctype>
#include <thread>
#include <Windows.h>

MeleeManager::MeleeManager() {
    Initialize();
}

MeleeManager::~MeleeManager() {
}

void MeleeManager::Initialize() {
    // Initialize default melee
    default_melee = "Dagger";
    last_known_valid_melee = default_melee;
    state.user_selected_melee = default_melee;
    
    // Get Roblox instances
    GetPlayerData();
    GetMeleesFolder();
    
    // Collect all melees
    if (melees_folder.address != 0) {
        auto children = melees_folder.get_children();
        for (const auto& child : children) {
            std::string name = child.get_name();
            if (!name.empty()) {
                all_melees.push_back(name);
            }
        }
        std::sort(all_melees.begin(), all_melees.end());
    }
    
    // Initialize filtered list
    state.filtered_list = all_melees;
    
    // Get current melee value and set selected index
    std::string current_melee = GetMeleeValue();
    if (IsValidMelee(current_melee)) {
        last_known_valid_melee = current_melee;
        state.user_selected_melee = current_melee;
        
        // Find index in list
        auto it = std::find(all_melees.begin(), all_melees.end(), current_melee);
        if (it != all_melees.end()) {
            globals::misc::melee_selected_index = static_cast<int>(std::distance(all_melees.begin(), it));
        }
    } else {
        last_known_valid_melee = default_melee;
        state.user_selected_melee = default_melee;
        globals::misc::melee_selected_index = 0;
    }
}

int MeleeManager::GetSelectedIndex() const {
    return globals::misc::melee_selected_index;
}

void MeleeManager::SetSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(all_melees.size())) {
        globals::misc::melee_selected_index = index;
        std::string selected_melee = all_melees[index];
        SetSelectedMelee(selected_melee);
    }
}

void MeleeManager::SetSelectedMelee(const std::string& melee) {
    if (melee.empty()) {
        return;
    }
    
    state.user_selected_melee = melee;
    
    // Refresh player data before writing
    GetPlayerData();
    
    // Write the melee value
    SetMeleeValue(melee);
    
    // Update last known valid melee
    last_known_valid_melee = melee;
}

bool MeleeManager::GetPlayerData() {
    if (globals::instances::localplayer.address == 0) {
        return false;
    }
    
    try {
        player_data = globals::instances::localplayer.findfirstchild("Data");
        if (player_data.address != 0) {
            melee_value = player_data.findfirstchild("Melee");
            return melee_value.address != 0;
        }
    } catch (...) {
        return false;
    }
    
    return false;
}

bool MeleeManager::GetMeleesFolder() {
    try {
        roblox::instance replicated_storage = globals::instances::datamodel.read_service("ReplicatedStorage");
        if (replicated_storage.address != 0) {
            melees_folder = replicated_storage.findfirstchild("Melees");
            return melees_folder.address != 0;
        }
    } catch (...) {
        return false;
    }
    
    return false;
}

std::string MeleeManager::GetMeleeValue() {
    if (melee_value.address == 0) {
        if (!GetPlayerData()) {
            return "";
        }
    }
    
    try {
        // Read StringValue - use the instance read_string method pattern
        // StringValue.Value is typically at offset 0x80
        uintptr_t value_ptr = read<uintptr_t>(melee_value.address + 0x80);
        if (value_ptr != 0) {
            int length = read<int>(value_ptr + 0x18);
            if (length > 0 && length < 256) {
                char buffer[256] = {0};
                HANDLE current_handle = mem::roblox_h.load();
                if (current_handle) {
                    SIZE_T bytes_read = 0;
                    if (length >= 16) {
                        uintptr_t data_ptr = read<uintptr_t>(value_ptr);
                        if (data_ptr != 0) {
                            ReadProcessMemory(current_handle, (LPCVOID)data_ptr, buffer, length, &bytes_read);
                        }
                    } else {
                        ReadProcessMemory(current_handle, (LPCVOID)value_ptr, buffer, length, &bytes_read);
                    }
                    if (bytes_read > 0) {
                        return std::string(buffer, bytes_read);
                    }
                }
            }
        }
    } catch (...) {
        return "";
    }
    
    return "";
}

void MeleeManager::SetMeleeValue(const std::string& value) {
    if (melee_value.address == 0) {
        if (!GetPlayerData()) {
            return;
        }
    }
    
    if (melee_value.address == 0) {
        return;
    }
    
    try {
        // Match the reading pattern: read pointer at offset 0x80, then write to that pointer
        uintptr_t value_ptr_address = melee_value.address + 0x80;
        uintptr_t value_ptr = read<uintptr_t>(value_ptr_address);
        
        if (value_ptr != 0) {
            // Write the string to the pointer location (same as reading pattern)
            bool success = write_string(value_ptr, value);
            
            if (success) {
                state.user_selected_melee = value;
                last_known_valid_melee = value;
                return;
            }
        }
        
        // Fallback: try direct write to offset 0x80 (in case it's not a pointer)
        bool success = write_string(value_ptr_address, value);
        
        if (!success) {
            // Last fallback: try standard Value offset
            uintptr_t value_address = melee_value.address + offsets::Value;
            success = write_string(value_address, value);
        }
        
        // Update state after successful write
        if (success) {
            state.user_selected_melee = value;
            last_known_valid_melee = value;
        }
    } catch (...) {
        // Error writing
    }
}

std::string MeleeManager::GetGunText() {
    try {
        if (globals::instances::localplayer.address == 0) {
            return "";
        }
        
        roblox::instance player_gui = globals::instances::localplayer.findfirstchild("PlayerGui");
        if (player_gui.address == 0) {
            return "";
        }
        
        roblox::instance gui_interface = player_gui.findfirstchild("GUI_Interface");
        if (gui_interface.address == 0) {
            return "";
        }
        
        roblox::instance vitals = gui_interface.findfirstchild("Vitals");
        if (vitals.address == 0) {
            return "";
        }
        
        roblox::instance ammo = vitals.findfirstchild("Ammo");
        if (ammo.address == 0) {
            return "";
        }
        
        roblox::instance gun = ammo.findfirstchild("Gun");
        if (gun.address == 0) {
            return "";
        }
        
        // Read TextLabel text - TextLabel.Text is typically at offset 0x138
        uintptr_t text_ptr = read<uintptr_t>(gun.address + 0x138);
        if (text_ptr != 0) {
            int length = read<int>(text_ptr + 0x18);
            if (length > 0 && length < 256) {
                char buffer[256] = {0};
                HANDLE current_handle = mem::roblox_h.load();
                if (current_handle) {
                    SIZE_T bytes_read = 0;
                    if (length >= 16) {
                        uintptr_t data_ptr = read<uintptr_t>(text_ptr);
                        if (data_ptr != 0) {
                            ReadProcessMemory(current_handle, (LPCVOID)data_ptr, buffer, length, &bytes_read);
                        }
                    } else {
                        ReadProcessMemory(current_handle, (LPCVOID)text_ptr, buffer, length, &bytes_read);
                    }
                    if (bytes_read > 0) {
                        return std::string(buffer, bytes_read);
                    }
                }
            }
        }
    } catch (...) {
        return "";
    }
    
    return "";
}

bool MeleeManager::IsValidMelee(const std::string& melee) {
    if (melee.empty()) {
        return false;
    }
    
    // Check if contains at least one letter
    for (char c : melee) {
        if (std::isalpha(c)) {
            return true;
        }
    }
    
    return false;
}

void MeleeManager::Update() {
    if (!globals::misc::melee_changer) {
        return;
    }
    
    // Sync swing fix state
    state.swing_fix_enabled = globals::misc::melee_swing_fix;
    
    // Update Roblox instances periodically
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() > 1000) {
        GetPlayerData();
        GetMeleesFolder();
        last_update = now;
    }
    
    // Only do crash protection, don't override user selection
    // The crash protection should only trigger if the value becomes invalid (game cleared it)
    std::string current_val = GetMeleeValue();
    if (!IsValidMelee(current_val)) {
        // Invalid value detected - game cleared it, restore to user selection
        if (IsValidMelee(state.user_selected_melee)) {
            SetMeleeValue(state.user_selected_melee);
        } else {
            std::string safe = IsValidMelee(last_known_valid_melee) ? last_known_valid_melee : default_melee;
            SetMeleeValue(safe);
            state.user_selected_melee = safe;
        }
    } else {
        // Value is valid, update last known
        last_known_valid_melee = current_val;
    }
    
    // Update swing fix
    if (state.swing_fix_enabled) {
        UpdateSwingFix();
    }
}

void MeleeManager::UpdateSwingFix() {
    std::string gun_text = GetGunText();
    
    if (gun_text.empty()) {
        return;
    }
    
    // Check A: Golden Knife
    if (gun_text.find("Golden Knife") != std::string::npos) {
        // Do nothing - let golden knife exist
        return;
    }
    
    // Check B: Standard Knife
    if (gun_text == "Knife") {
        // Force default dagger temporarily
        if (GetMeleeValue() != default_melee && !state.swing_fix_debounce) {
            state.swing_fix_debounce = true;
            state.swing_fix_time = std::chrono::steady_clock::now();
            
            // Set to default after delay
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(420));
                SetMeleeValue(default_melee);
                state.swing_fix_debounce = false;
            }).detach();
        }
    } else {
        // Check C: Holding a Gun - revert to user selection
        if (GetMeleeValue() != state.user_selected_melee) {
            SetMeleeValue(state.user_selected_melee);
        }
    }
}

bool MeleeManager::IsMouseOver(const ImVec2& pos, const ImVec2& size) {
    if (!state.menu_open) {
        return false;
    }
    
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    return mouse_pos.x >= pos.x && mouse_pos.x <= pos.x + size.x &&
           mouse_pos.y >= pos.y && mouse_pos.y <= pos.y + size.y;
}

void MeleeManager::UpdateFilter() {
    if (state.search_text.empty()) {
        state.filtered_list = all_melees;
    } else {
        state.filtered_list.clear();
        std::string search_lower = state.search_text;
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
        
        for (const auto& name : all_melees) {
            std::string name_lower = name;
            std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
            
            if (name_lower.find(search_lower) != std::string::npos) {
                state.filtered_list.push_back(name);
            }
        }
    }
    
    state.scroll_offset = 0;
}

void MeleeManager::HandleTyping() {
    if (!state.search_focused || !state.menu_open) {
        return;
    }
    
    if (state.debounce > 0) {
        state.debounce--;
        return;
    }
    
    bool changed = false;
    ImGuiIO& io = ImGui::GetIO();
    
    // Handle backspace
    if (io.KeysDown[ImGuiKey_Backspace] && !state.search_text.empty()) {
        state.search_text.pop_back();
        changed = true;
    }
    
    // Handle character input
    for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        unsigned int c = io.InputQueueCharacters[i];
        if (c >= 32 && c < 127) { // Printable ASCII
            state.search_text += static_cast<char>(c);
            changed = true;
        }
    }
    
    if (changed) {
        state.debounce = 5;
        UpdateFilter();
    }
}

void MeleeManager::Render() {
    // Removed - UI is now in main menu
}

void MeleeManager::DrawUI() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    if (!draw_list) {
        return;
    }
    
    ImVec2 P = cfg.pos;
    ImVec2 S = cfg.size;
    
    // Main Window Background
    draw_list->AddRectFilled(P, ImVec2(P.x + S.x, P.y + S.y), cfg.colors.bg);
    draw_list->AddRect(P - ImVec2(1, 1), ImVec2(P.x + S.x + 1, P.y + S.y + 1), cfg.colors.border2, 0.0f, 0, 3.0f);
    draw_list->AddRect(P, ImVec2(P.x + S.x, P.y + S.y), cfg.colors.border1);
    
    // Gradient Bar
    float seg_width = S.x / 4.0f;
    ImU32 gradient_colors[4] = {
        IM_COL32(55, 175, 225, 255),
        IM_COL32(200, 80, 225, 255),
        IM_COL32(225, 225, 60, 255),
        IM_COL32(165, 205, 65, 255)
    };
    for (int i = 0; i < 4; i++) {
        draw_list->AddRectFilled(
            ImVec2(P.x + i * seg_width, P.y),
            ImVec2(P.x + (i + 1) * seg_width, P.y + 2),
            gradient_colors[i]
        );
    }
    
    // Left Tab Area
    float inner_pad = 12.0f;
    float tab_width = 80.0f;
    ImVec2 tab_pos = ImVec2(P.x + inner_pad, P.y + inner_pad + 10);
    ImVec2 tab_size = ImVec2(tab_width, S.y - (inner_pad * 2) - 10);
    
    draw_list->AddRectFilled(tab_pos, ImVec2(tab_pos.x + tab_size.x, tab_pos.y + tab_size.y), cfg.colors.bg_dark);
    draw_list->AddRect(tab_pos, ImVec2(tab_pos.x + tab_size.x, tab_pos.y + tab_size.y), cfg.colors.border2);
    
    // Tab Icon & Text
    float tab_y_start = tab_pos.y + 15;
    float icon_height = 20.0f;
    draw_list->AddRectFilled(
        ImVec2(tab_pos.x, tab_y_start),
        ImVec2(tab_pos.x + 2, tab_y_start + icon_height),
        cfg.colors.accent
    );
    
    // Draw "Melee" text
    draw_list->AddText(ImVec2(tab_pos.x + 10, tab_y_start + 2), cfg.colors.text, "Melee");
    
    // SwingFix Checkbox
    float box_size = 10.0f;
    float box_y = tab_pos.y + tab_size.y - 20;
    float box_x = tab_pos.x + 5;
    
    // Draw "SwingFix" text
    draw_list->AddText(ImVec2(box_x + box_size + 6, box_y - 2), 
                       state.swing_fix_enabled ? cfg.colors.text : cfg.colors.text_dim, 
                       "SwingFix");
    
    draw_list->AddRect(
        ImVec2(box_x, box_y),
        ImVec2(box_x + box_size, box_y + box_size),
        cfg.colors.border1
    );
    
    if (state.swing_fix_enabled) {
        draw_list->AddRectFilled(
            ImVec2(box_x + 2, box_y + 2),
            ImVec2(box_x + box_size - 2, box_y + box_size - 2),
            cfg.colors.accent
        );
    }
    
    // Right Groupbox
    float gb_x = inner_pad + tab_width + inner_pad;
    float gb_w = S.x - gb_x - inner_pad;
    float gb_h = S.y - (inner_pad * 2) - 10;
    ImVec2 gb_pos = ImVec2(P.x + gb_x, P.y + inner_pad + 10);
    
    draw_list->AddRect(gb_pos, ImVec2(gb_pos.x + gb_w, gb_pos.y + gb_h), cfg.colors.border1);
    draw_list->AddRectFilled(
        ImVec2(gb_pos.x + 10, gb_pos.y - 2),
        ImVec2(gb_pos.x + 100, gb_pos.y + 4),
        cfg.colors.bg
    );
    draw_list->AddText(ImVec2(gb_pos.x + 15, gb_pos.y - 7), cfg.colors.text, "Melee Selection");
    
    // Search Bar
    float search_h = 22.0f;
    float list_pad = 12.0f;
    float search_y = gb_pos.y + gb_h - search_h - list_pad;
    
    ImVec2 search_pos = ImVec2(gb_pos.x + list_pad, search_y);
    ImVec2 search_size = ImVec2(gb_w - (list_pad * 2), search_h);
    
    draw_list->AddRectFilled(search_pos, ImVec2(search_pos.x + search_size.x, search_pos.y + search_size.y), cfg.colors.bg_dark);
    draw_list->AddRect(search_pos, ImVec2(search_pos.x + search_size.x, search_pos.y + search_size.y), cfg.colors.border1);
    
    // Search text
    std::string search_display = state.search_focused ? (state.search_text + "|") : 
                                  (state.search_text.empty() ? "Search..." : state.search_text);
    ImU32 search_color = state.search_focused || !state.search_text.empty() ? cfg.colors.text : cfg.colors.text_dim;
    draw_list->AddText(search_pos + ImVec2(5, 4), search_color, search_display.c_str());
    
    // List Area
    float list_top = gb_pos.y + list_pad + 10;
    float available_h = search_y - list_top - 8;
    float list_w = gb_w - (list_pad * 2);
    ImVec2 list_pos = ImVec2(gb_pos.x + list_pad, list_top);
    
    draw_list->AddRectFilled(list_pos, ImVec2(list_pos.x + list_w, list_pos.y + available_h), cfg.colors.bg_dark);
    draw_list->AddRect(
        list_pos - ImVec2(1, 1),
        ImVec2(list_pos.x + list_w + 1, list_pos.y + available_h + 1),
        cfg.colors.border2
    );
    
    // Scrollbar
    float scroll_w = 10.0f;
    float scroll_x = list_pos.x + list_w - scroll_w - 2;
    float scroll_y = list_pos.y + 2;
    float scroll_h = available_h - 4;
    float btn_size = 10.0f;
    
    draw_list->AddRectFilled(
        ImVec2(scroll_x, scroll_y),
        ImVec2(scroll_x + scroll_w, scroll_y + btn_size),
        cfg.colors.border1
    );
    
    draw_list->AddRectFilled(
        ImVec2(scroll_x, scroll_y + scroll_h - btn_size),
        ImVec2(scroll_x + scroll_w, scroll_y + scroll_h),
        cfg.colors.border1
    );
    
    draw_list->AddRectFilled(
        ImVec2(scroll_x, scroll_y + btn_size),
        ImVec2(scroll_x + scroll_w, scroll_y + scroll_h - btn_size),
        IM_COL32(15, 15, 15, 255)
    );
    
    int total_items = static_cast<int>(state.filtered_list.size());
    int max_scroll = std::max(0, total_items - cfg.list.max_visible);
    float track_h = scroll_h - (btn_size * 2);
    float thumb_ratio = std::min(1.0f, static_cast<float>(cfg.list.max_visible) / std::max(1.0f, static_cast<float>(total_items)));
    float thumb_h = std::max(8.0f, track_h * thumb_ratio);
    float scroll_ratio = max_scroll > 0 ? static_cast<float>(state.scroll_offset) / max_scroll : 0.0f;
    
    draw_list->AddRectFilled(
        ImVec2(scroll_x + 1, scroll_y + btn_size + (scroll_ratio * (track_h - thumb_h))),
        ImVec2(scroll_x + scroll_w - 1, scroll_y + btn_size + (scroll_ratio * (track_h - thumb_h)) + thumb_h),
        cfg.colors.border1
    );
    
    // List Items
    for (int i = 0; i < cfg.list.max_visible; i++) {
        int index = state.scroll_offset + i;
        if (index >= 0 && index < static_cast<int>(state.filtered_list.size())) {
            std::string name = state.filtered_list[index];
            float row_y = list_pos.y + 4 + (i * cfg.list.item_height);
            float row_x = list_pos.x + 4;
            
            ImVec2 item_pos = ImVec2(row_x, row_y);
            ImVec2 item_size = ImVec2(list_w - scroll_w - 8, cfg.list.item_height);
            
            ImU32 text_color = cfg.colors.text;
            ImU32 bg_color = cfg.colors.bg_dark;
            
            if (name == state.user_selected_melee) {
                text_color = cfg.colors.accent;
                bg_color = IM_COL32(35, 35, 35, 255);
            } else if (IsMouseOver(item_pos, item_size)) {
                bg_color = IM_COL32(30, 30, 30, 255);
            }
            
            draw_list->AddRectFilled(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y), bg_color);
            draw_list->AddText(item_pos + ImVec2(4, 2), text_color, name.c_str());
        }
    }
    
    // Handle input
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool clicked = mouse_down && !state.last_mouse_down;
    
    // Window dragging
    if (clicked && IsMouseOver(P, ImVec2(S.x, 30))) {
        state.dragging = true;
        state.drag_start = mouse_pos;
        state.drag_offset = ImVec2(P.x - mouse_pos.x, P.y - mouse_pos.y);
        state.search_focused = false;
    }
    
    if (state.dragging && mouse_down) {
        cfg.pos = ImVec2(mouse_pos.x + state.drag_offset.x, mouse_pos.y + state.drag_offset.y);
    } else {
        state.dragging = false;
    }
    
    // Search focus
    if (clicked) {
        state.search_focused = IsMouseOver(search_pos, search_size);
    }
    
    // Checkbox click
    if (clicked && IsMouseOver(ImVec2(box_x, box_y), ImVec2(60, 15))) {
        state.swing_fix_enabled = !state.swing_fix_enabled;
    }
    
    // Scrolling & Selection
    if (mouse_down) {
        if (state.scrolling || (clicked && IsMouseOver(ImVec2(scroll_x, scroll_y + btn_size), ImVec2(scroll_w, track_h)))) {
            state.scrolling = true;
            float rel_y = std::clamp(mouse_pos.y - (scroll_y + btn_size), 0.0f, track_h);
            float pct = rel_y / track_h;
            state.scroll_offset = static_cast<int>(pct * max_scroll + 0.5f);
        }
    } else {
        state.scrolling = false;
    }
    
    if (clicked) {
        // Scroll up/down buttons
        if (IsMouseOver(ImVec2(scroll_x, scroll_y), ImVec2(scroll_w, btn_size))) {
            if (state.scroll_offset > 0) {
                state.scroll_offset--;
            }
        } else if (IsMouseOver(ImVec2(scroll_x, scroll_y + scroll_h - btn_size), ImVec2(scroll_w, btn_size))) {
            if (state.scroll_offset < max_scroll) {
                state.scroll_offset++;
            }
        } else if (IsMouseOver(list_pos, ImVec2(list_w - scroll_w - 15, available_h))) {
            // Item selection
            float rel_y = mouse_pos.y - (list_pos.y + 4);
            int index = static_cast<int>(rel_y / cfg.list.item_height);
            int real_index = state.scroll_offset + index;
            if (real_index >= 0 && real_index < static_cast<int>(state.filtered_list.size())) {
                state.user_selected_melee = state.filtered_list[real_index];
                SetMeleeValue(state.user_selected_melee);
            }
        }
    }
    
    state.last_mouse_down = mouse_down;
}

