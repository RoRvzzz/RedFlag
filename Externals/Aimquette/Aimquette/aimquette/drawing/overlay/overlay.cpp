#include "overlay.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx11.h"

// Removed: Text editor include (was only used for the Lua tab)
#include <dwmapi.h>
#include <numeric>
#include <shellapi.h>

#include <filesystem>
#include <thread>
#include <chrono>
#include <bitset>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include "../../util/classes/classes.h"

using namespace roblox;
#include <cmath>
#include <unordered_set>
#include <vector>
#include <cctype>
#include <string>

#ifdef min
#undef min
#endif
#include <stack>
#include "../../util/notification/notification.h"
#ifdef max
#undef max
#endif
#include <Psapi.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include "../imgui/backends/imgui_impl_win32.h"
#include "../imgui/backends/imgui_impl_dx11.h"
#include "../imgui/misc/freetype/imgui_freetype.h"
#include "../imgui/addons/imgui_addons.h"
#include <dwmapi.h>
#include "../../util/globals.h"
#include "keybind/keybind.h"
#include "../../features/visuals/visuals.h"
#include "../../util/config/configsystem.h"
#include "../../features/combat/modules/dahood/autostuff/auto.h"
#include "../../features/hook.h"
#include "../framework/settings/functions.h"
#include "../framework/data/fonts.h"
#include "../framework/settings/variables.h"

// Waypoint system removed
// Minimal stubs to satisfy references in disabled waypoint UI/render functions
struct WaypointStub {
    std::string name;
    math::Vector3 position;
    bool visible;
    WaypointStub(const std::string& n, const math::Vector3& p) : name(n), position(p), visible(true) {}
};
static std::vector<WaypointStub> waypoints;
static bool show_waypoint_panel = false;
static char new_waypoint_name[128] = "";

// Roblox Instance Explorer functionality (like Dex)
static instance selected_instance;
static std::unordered_set<uint64_t> expanded_instances;
static std::unordered_map<uint64_t, std::vector<instance>> instance_cache;
static std::unordered_map<uint64_t, std::string> instance_name_cache;
static std::unordered_map<uint64_t, std::string> instance_class_cache;
static std::unordered_map<uint64_t, std::string> instance_path_cache;
static std::unordered_map<uint64_t, bool> instance_children_loaded; // Track if children are loaded
static char search_filter[256] = "";
static std::vector<instance> search_results;
static bool show_search_results = false;
static bool cache_initialized = false;
static auto last_cache_refresh = std::chrono::steady_clock::now();
static auto last_tree_render = std::chrono::steady_clock::now();

// Filter options
static bool show_only_parts = false;
static bool show_only_scripts = false;
static bool search_by_path = false; // Add search by path option

// Double-click detection
static std::string last_clicked_item = "";
static auto last_click_time = std::chrono::steady_clock::now();
static const auto double_click_duration = std::chrono::milliseconds(300);



// Performance optimization constants
static const int CACHE_REFRESH_INTERVAL_SECONDS = 10; // Increased from 2 to 10 seconds
static const int MAX_CACHE_SIZE = 10000; // Limit cache size to prevent memory issues

// Forward declarations
static void safe_teleport_to(const math::Vector3& rawTarget);
static std::string getInstanceName(const instance& instance);
static std::string getInstanceClassName(const instance& instance);
static std::string getInstanceDisplayName(const instance& instance);
static std::string getInstancePath(const instance& instance);
static std::string getInstanceFullPath(const instance& instance);
static void searchInstances(const instance& instance, const std::string& query);
static void renderInstanceTree(const instance& instance, int depth);
void render_explorer();

// Helper functions for instance explorer

static void cacheInstance(const instance& instance, const std::string& path) {
    if (instance.address == 0) return;

    try {
        std::string name = instance.get_name();
        std::string class_name = instance.get_class_name();

        // Apply filters
        if (show_only_parts && class_name != "Part" && class_name != "BasePart" && class_name != "Model") {
            return; // Skip non-parts
        }

        if (show_only_scripts && class_name != "Script" && class_name != "LocalScript" && class_name != "ModuleScript") {
            return; // Skip non-scripts
        }

        instance_name_cache[instance.address] = name;
        instance_class_cache[instance.address] = class_name;
        instance_path_cache[instance.address] = path;

        // Only cache immediate children, not recursively (lazy loading)
        std::vector<roblox::instance> children = instance.get_children();
        instance_cache[instance.address] = children;
        instance_children_loaded[instance.address] = true;

        // Limit cache size to prevent memory issues
        if (instance_cache.size() > MAX_CACHE_SIZE) {
            // Remove oldest entries
            auto it = instance_cache.begin();
            instance_cache.erase(it);
            instance_name_cache.erase(it->first);
            instance_class_cache.erase(it->first);
            instance_path_cache.erase(it->first);
            instance_children_loaded.erase(it->first);
        }
    }
    catch (...) {
        // Ignore errors for individual instances
    }
}

static void searchInstances(const instance& instance, const std::string& query) {
    if (instance.address == 0) return;

    try {
        std::string name = instance.get_name();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });

        // Search by name (default behavior)
        std::string lower_query = query;
        std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), [](unsigned char c) { return std::tolower(c); });
        
        if (name.find(lower_query) != std::string::npos) {
            search_results.push_back(instance);
        }
        
        // If path search is enabled, also search by path
        if (search_by_path) {
            std::string path = getInstancePath(instance);
            std::transform(path.begin(), path.end(), path.begin(), [](unsigned char c) { return std::tolower(c); });
            
            if (path.find(lower_query) != std::string::npos) {
                // Only add if not already in results
                bool already_found = false;
                for (const auto& existing : search_results) {
                    if (existing.address == instance.address) {
                        already_found = true;
                        break;
                    }
                }
                if (!already_found) {
                    search_results.push_back(instance);
                }
            }
        }

        // Search children
        auto children = instance.get_children();
        for (const auto& child : children) {
            searchInstances(child, query);
        }
    }
    catch (...) {
        // Ignore search errors
    }
}

static std::string getInstanceName(const instance& instance) {
    if (instance.address == 0) return "Invalid";

    auto it = instance_name_cache.find(instance.address);
    if (it != instance_name_cache.end()) {
        return it->second;
    }

    try {
        return instance.get_name();
    }
    catch (...) {
        return "Unknown";
    }
}

static std::string getInstanceClassName(const instance& instance) {
    if (instance.address == 0) return "Invalid";

    auto it = instance_class_cache.find(instance.address);
    if (it != instance_class_cache.end()) {
        return it->second;
    }

    try {
        return instance.get_class_name();
    }
    catch (...) {
        return "Unknown";
    }
}

static std::string getInstanceDisplayName(const instance& instance) {
    if (instance.address == 0) return "Invalid";

    std::string name = getInstanceName(instance);
    std::string class_name = getInstanceClassName(instance);

    return name + " [" + class_name + "]";
}

static std::string getInstancePath(const instance& instance) {
    if (instance.address == 0) return "Invalid";

    auto it = instance_path_cache.find(instance.address);
    if (it != instance_path_cache.end()) {
        return it->second;
    }

    // Build path manually if not cached - format matches Dex exactly
    try {
        std::string path = instance.get_name();
        roblox::instance parent = instance.read_parent();
        while (parent.address != 0) {
            std::string parent_name = parent.get_name();
            if (!parent_name.empty()) {
                path = parent_name + "." + path;
            }
            parent = parent.read_parent();
        }
        
        // Cache the result
        instance_path_cache[instance.address] = path;
        return path;
    }
    catch (...) {
        return "Unknown";
    }
}

// Enhanced path function that includes class names like Dex
static std::string getInstanceFullPath(const instance& instance) {
    if (instance.address == 0) return "Invalid";

    try {
        std::string path = instance.get_name() + " (" + instance.get_class_name() + ")";
        roblox::instance parent = instance.read_parent();
        while (parent.address != 0) {
            std::string parent_name = parent.get_name();
            std::string parent_class = parent.get_class_name();
            if (!parent_name.empty()) {
                path = parent_name + " (" + parent_class + ")." + path;
            }
            parent = parent.read_parent();
        }
        return path;
    }
    catch (...) {
        return "Unknown";
    }
}

static void renderInstanceTree(const instance& instance, int depth) {
    if (instance.address == 0) return;

    try {
        std::string name = getInstanceName(instance);
        std::string class_name = getInstanceClassName(instance);
        std::string display_name = name + " [" + class_name + "]";

        bool is_selected = (selected_instance.address == instance.address);
        bool has_children = false;
        bool is_expanded = expanded_instances.count(instance.address);

        // Check if we need to load children (lazy loading)
        if (instance_children_loaded.count(instance.address) == 0) {
            // Load children only when needed
            try {
                std::vector<roblox::instance> children = instance.get_children();
                instance_cache[instance.address] = children;
                instance_children_loaded[instance.address] = true;
                has_children = !children.empty();
            }
            catch (...) {
                has_children = false;
            }
        } else {
            has_children = !instance_cache[instance.address].empty();
        }

        ImGui::PushID(instance.address);

        // Indent based on depth
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + depth * 20.0f);

        // Expand/collapse arrow for instances with children
        if (has_children) {
            if (ImGui::ArrowButton(("##expand" + std::to_string(instance.address)).c_str(),
                is_expanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (is_expanded) {
                    expanded_instances.erase(instance.address);
                }
                else {
                    expanded_instances.insert(instance.address);
                }
            }
            ImGui::SameLine();
        }
        else {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f); // Align with expandable items
        }



        // Instance name
        if (ImGui::Selectable(display_name.c_str(), is_selected)) {
            selected_instance = instance;
        }
        
        // Quick copy path button (small and unobtrusive)
        ImGui::SameLine();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
        if (ImGui::Button(("##copy" + std::to_string(instance.address)).c_str(), ImVec2(16, 16))) {
            std::string path = getInstancePath(instance);
            ImGui::SetClipboardText(path.c_str());
        }
        
        ImGui::PopStyleVar();

        // Handle double-click
        if (ImGui::IsItemClicked()) {
            auto now = std::chrono::steady_clock::now();
            if (last_clicked_item == std::to_string(instance.address) &&
                (now - last_click_time) < double_click_duration) {
                // Double-click - expand/collapse
                if (has_children) {
                    if (is_expanded) {
                        expanded_instances.erase(instance.address);
                    }
                    else {
                        expanded_instances.insert(instance.address);
                    }
                }
                last_clicked_item = "";
            }
            else {
                selected_instance = instance;
                last_clicked_item = std::to_string(instance.address);
                last_click_time = now;
            }
        }

        // Right-click context menu
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            selected_instance = instance;
            ImGui::OpenPopup("InstanceContextMenu");
        }

        ImGui::PopID();

        // Render children if expanded (simplified approach)
        if (is_expanded && has_children) {
            // Only limit rendering for very large instance trees
            if (instance_cache[instance.address].size() > 500) {
                // For very large trees, show first 100 instances and a summary
                int max_to_show = 100;
                int total_children = instance_cache[instance.address].size();
                
                for (int i = 0; i < std::min(max_to_show, total_children); i++) {
                    renderInstanceTree(instance_cache[instance.address][i], depth + 1);
                }
                
                if (total_children > max_to_show) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "... (%d more instances)", total_children - max_to_show);
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Large instance tree - consider using search filters");
                }
            } else {
                // For normal sized trees, render all children
                for (const auto& child : instance_cache[instance.address]) {
                    renderInstanceTree(child, depth + 1);
                }
            }
        }

    }
    catch (...) {
        // Ignore rendering errors for individual instances
    }
}

void render_explorer() {
    // Explorer removed
    return;
    if (!globals::misc::explorer) return;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    float rounded = style.WindowRounding;
    style.WindowRounding = 0.0f;

    static ImVec2 explorer_pos = ImVec2(100, 100);
    static bool first_time = true;
    static bool isDragging = false;
    static ImVec2 dragDelta;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    if (first_time || !overlay::visible) {
        ImGui::SetNextWindowPos(explorer_pos, ImGuiCond_Always);
        first_time = false;
    }

    ImVec2 windowSize(800, 600);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::Begin("Explorer", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();

    if (overlay::visible) {
        explorer_pos = window_pos;
    }

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImRect headerRect(explorer_pos, ImVec2(explorer_pos.x + windowSize.x, explorer_pos.y + 30));
    ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();

    // Match theme style: shadow, background, accent lines, stroke border
    draw->rect(bg_draw, explorer_pos - ImVec2(1, 1), explorer_pos + windowSize + ImVec2(1, 1), draw->get_clr({0, 0, 0, 0.5f}));
    draw->rect_filled(draw_list, explorer_pos, explorer_pos + windowSize, draw->get_clr(clr->window.background_one));
    draw->line(draw_list, explorer_pos + ImVec2(1, 1), ImVec2(explorer_pos.x + windowSize.x - 1, explorer_pos.y + 1), draw->get_clr(clr->accent), 1);
    draw->line(draw_list, explorer_pos + ImVec2(1, 2), ImVec2(explorer_pos.x + windowSize.x - 1, explorer_pos.y + 2), draw->get_clr(clr->accent, 0.4f), 1);
    draw->rect(draw_list, explorer_pos, explorer_pos + windowSize, draw->get_clr(clr->window.stroke));

    if (ImGui::IsMouseClicked(0) && headerRect.Contains(mousePos) && overlay::visible) {
        isDragging = true;
        dragDelta = mousePos - explorer_pos;
    }
    if (isDragging && ImGui::IsMouseDown(0)) {
        explorer_pos = mousePos - dragDelta;
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        explorer_pos.x = ImClamp(explorer_pos.x, 0.0f, screenSize.x - windowSize.x);
        explorer_pos.y = ImClamp(explorer_pos.y, 0.0f, screenSize.y - windowSize.y);
    }
    else {
        isDragging = false;
    }

    // Header text
    draw_list->AddText(ImVec2(explorer_pos.x + 8, explorer_pos.y + 8), draw->get_clr(clr->widgets.text), "Roblox Instance Explorer");

    ImGui::SetCursorPos(ImVec2(8, 30));

    // Search and filter controls
    ImGui::InputTextWithHint("##SearchFilter", "Search instances...", search_filter, sizeof(search_filter));
    
    ImGui::SameLine();
    if (ImGui::Checkbox("Search by Path", &search_by_path)) {
        if (strlen(search_filter) > 0) {
            search_results.clear();
            show_search_results = false;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        cache_initialized = false;
        search_results.clear();
        show_search_results = false;
        selected_instance = instance();
        expanded_instances.clear();
        instance_children_loaded.clear();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Cache")) {
        instance_cache.clear();
        instance_name_cache.clear();
        instance_class_cache.clear();
        instance_path_cache.clear();
        instance_children_loaded.clear();
        cache_initialized = false;
        search_results.clear();
        show_search_results = false;
        selected_instance = instance();
        expanded_instances.clear();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Copy All Paths")) {
        std::string all_paths;
        for (const auto& [addr, children] : instance_cache) {
            try {
                roblox::instance inst(addr);
                if (inst.address != 0) {
                    std::string path = getInstancePath(inst);
                    all_paths += path + "\n";
                }
            }
            catch (...) {
            }
        }
        if (!all_paths.empty()) {
            ImGui::SetClipboardText(all_paths.c_str());
        }
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Show Only Parts", &show_only_parts)) {
        cache_initialized = false;
    }

    ImGui::SameLine();
    if (ImGui::Checkbox("Show Only Scripts", &show_only_scripts)) {
        cache_initialized = false;
    }

    ImGui::Separator();

    // Instance tree view and properties
    ImGui::Columns(2, nullptr, false);

    // Left column - Instance tree
    ImGui::BeginChild("InstanceTree", ImVec2(0, -30), true);

    try {
        // Get the DataModel (game)
        auto& datamodel = globals::instances::datamodel;
        if (datamodel.address == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DataModel not found. Make sure you're in a Roblox game.");
            ImGui::EndChild();
            ImGui::EndChild();
            return;
        }

        instance root_instance(datamodel.address);

        // Cache refresh logic - reduced frequency for better performance
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cache_refresh).count() >= CACHE_REFRESH_INTERVAL_SECONDS) {
            // Only clear caches if they're getting too large
            if (instance_cache.size() > MAX_CACHE_SIZE * 0.8) {
                instance_cache.clear();
                instance_name_cache.clear();
                instance_class_cache.clear();
                instance_path_cache.clear();
                instance_children_loaded.clear();
                cache_initialized = false;
            }
            last_cache_refresh = now;
        }

        // Initialize cache if needed
        if (!cache_initialized) {
            cacheInstance(root_instance, "");
            cache_initialized = true;
        }

        // Handle search
        if (strlen(search_filter) > 0) {
            if (!show_search_results) {
                search_results.clear();
                searchInstances(root_instance, search_filter);
                show_search_results = true;
            }

            // Display search results
            ImGui::Text("Search Results (%d):", (int)search_results.size());
            ImGui::Separator();

            for (const auto& instance : search_results) {
                if (instance.address == 0) continue;

                std::string display_name = getInstanceDisplayName(instance);
                bool is_selected = (selected_instance.address == instance.address);

                ImGui::PushID(instance.address);

                if (ImGui::Selectable(display_name.c_str(), is_selected)) {
                    selected_instance = instance;
                }

                // Handle double-click
                if (ImGui::IsItemClicked()) {
                    auto now = std::chrono::steady_clock::now();
                    if (last_clicked_item == std::to_string(instance.address) &&
                        (now - last_click_time) < double_click_duration) {
                        // Double-click - expand/collapse
                        if (expanded_instances.count(instance.address)) {
                            expanded_instances.erase(instance.address);
                        }
                        else {
                            expanded_instances.insert(instance.address);
                        }
                        last_clicked_item = "";
                    }
                    else {
                        selected_instance = instance;
                        last_clicked_item = std::to_string(instance.address);
                        last_click_time = now;
                    }
                }

                // Right-click context menu
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    selected_instance = instance;
                    ImGui::OpenPopup("InstanceContextMenu");
                }

                ImGui::PopID();
            }
        }
        else {
            show_search_results = false;
            // Display normal instance tree
            renderInstanceTree(root_instance, 0);
        }

    }
    catch (const std::exception& e) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: %s", e.what());
    }

    ImGui::EndChild();

    // Right column - Instance properties
    ImGui::NextColumn();
    ImGui::BeginChild("InstanceProperties", ImVec2(0, 0), true);

    ImGui::Text("Instance Properties");
    ImGui::Separator();

    if (selected_instance.address != 0) {
        try {
            std::string name = getInstanceName(selected_instance);
            std::string class_name = getInstanceClassName(selected_instance);
            std::string path = getInstancePath(selected_instance);

            ImGui::Text("Name: %s", name.c_str());
            ImGui::Text("Class: %s", class_name.c_str());
            
            // Enhanced path display with copy buttons
            ImGui::Text("Path:");
            ImGui::SameLine();
            
            // Style the copy buttons to be more compact and attractive
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
            if (ImGui::Button("Copy Path##CopyPathBtn", ImVec2(80, 20))) {
                ImGui::SetClipboardText(path.c_str());
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.6f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.7f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.1f, 0.5f, 1.0f));
            if (ImGui::Button("Copy Full Path##CopyFullPathBtn", ImVec2(120, 20))) {
                std::string full_path = getInstanceFullPath(selected_instance);
                ImGui::SetClipboardText(full_path.c_str());
            }
            ImGui::PopStyleColor(3);
            
            // Display the path with word wrapping
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x - 20);
            ImGui::TextWrapped("%s", path.c_str());
            ImGui::PopTextWrapPos();
            
            ImGui::Text("Address: 0x%llX", selected_instance.address);

            ImGui::Separator();

            // Show position if it's a BasePart
            if (class_name == "Part" || class_name == "BasePart" || class_name == "Model") {
                try {
                    math::Vector3 position = selected_instance.get_pos();
                    ImGui::Text("Position: X: %.2f, Y: %.2f, Z: %.2f", position.x, position.y, position.z);

                    if (ImGui::Button("Teleport To")) {
                        safe_teleport_to(position);
                    }
                }
                catch (...) {
                    ImGui::Text("Position: Not accessible");
                }
            }

            // Show other properties based on class
            if (class_name == "Part" || class_name == "BasePart") {
                try {
                    Vector3 size = selected_instance.get_part_size();
                    ImGui::Text("Size: X: %.2f, Y: %.2f, Z: %.2f", size.x, size.y, size.z);

                    bool can_collide = selected_instance.get_cancollide();
                    ImGui::Text("Can Collide: %s", can_collide ? "Yes" : "No");

                    if (ImGui::Button("Toggle Collision")) {
                        selected_instance.write_cancollide(!can_collide);
                    }
                }
                catch (...) {
                    ImGui::Text("Properties: Not accessible");
                }
            }

        }
        catch (...) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error reading instance properties");
        }
    }
    else {
        ImGui::Text("No instance selected");
    }

    ImGui::EndChild();

    // Context menu for instances
    if (ImGui::BeginPopup("InstanceContextMenu")) {
        if (ImGui::MenuItem("Copy Address")) {
            ImGui::SetClipboardText(std::to_string(selected_instance.address).c_str());
        }

        if (ImGui::MenuItem("Copy Name")) {
            std::string name = getInstanceName(selected_instance);
            ImGui::SetClipboardText(name.c_str());
        }

        if (ImGui::MenuItem("Copy Class")) {
            std::string class_name = getInstanceClassName(selected_instance);
            ImGui::SetClipboardText(class_name.c_str());
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Teleport To")) {
            try {
                math::Vector3 position = selected_instance.get_pos();
                if (position.x != 0 || position.y != 0 || position.z != 0) {
                    safe_teleport_to(position);
                }
            }
            catch (...) {
                // Ignore teleport errors
            }
        }

        if (ImGui::MenuItem("View Properties")) {
            // Could expand to show instance properties
        }

        if (ImGui::MenuItem("Copy Path")) {
            std::string path = getInstancePath(selected_instance);
            ImGui::SetClipboardText(path.c_str());
        }
        
        if (ImGui::MenuItem("Copy Full Path (with Classes)")) {
            std::string full_path = getInstanceFullPath(selected_instance);
            ImGui::SetClipboardText(full_path.c_str());
        }
        
        if (ImGui::MenuItem("Copy Path for Scripting")) {
            std::string path = getInstancePath(selected_instance);
            std::string script_path = "game." + path;
            ImGui::SetClipboardText(script_path.c_str());
        }

        if (ImGui::MenuItem("Find References")) {
            // Could implement reference finding
        }

        ImGui::EndPopup();
    }

    ImGui::EndChild();
    ImGui::EndChild();

    // Status bar with performance info
    ImGui::SetCursorPos(ImVec2(8, windowSize.y - 20));
    ImGui::Text("Selected: %s | Instances: %d | Expanded: %d | Cache: %d/%d",
        selected_instance.address == 0 ? "None" : getInstanceName(selected_instance).c_str(),
        (int)instance_cache.size(),
        (int)expanded_instances.size(),
        (int)instance_cache.size(),
        MAX_CACHE_SIZE);
    
    if (search_by_path) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), " | Path Search: ON");
    }

    style.WindowRounding = rounded;
    ImGui::End();
}





static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winhttp.lib")


bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

bool fullsc(HWND windowHandle);

void movewindow(HWND hw);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


static ConfigManager g_config_manager;

// Visual-only disabled scope: dims widgets but keeps them interactive
static std::vector<bool> g_visual_disabled_stack;
static inline void BeginVisualDisabled(bool disabled)
{
    (void)disabled;
    g_visual_disabled_stack.push_back(disabled);
}
static inline void EndVisualDisabled()
{
    if (!g_visual_disabled_stack.empty())
        g_visual_disabled_stack.pop_back();
}

// Configurable menu toggle keybind (defaults to Insert)
static keybind g_menu_toggle_keybind("menu_toggle_keybind");
static bool g_menu_custom_bind_enabled = false;
static const int g_menu_default_key = VK_INSERT;

bool isAutoFunctionActivez() {
    return globals::bools::kill || globals::bools::autokill;
}

// Safer teleport utility: interpolate position via CFrame to reduce detections/kicks
static void safe_teleport_to(const math::Vector3& rawTarget)
{
    if (globals::handlingtp) return;

    instance hrp = globals::instances::lp.hrp;
    if (hrp.address == 0) return;

    math::Vector3 target = rawTarget;
    target.y += 0.5f; // small lift to avoid ground clipping

    CFrame currentCFrame = hrp.read_cframe();
    math::Vector3 start = currentCFrame.position;

    float dx = target.x - start.x;
    float dy = target.y - start.y;
    float dz = target.z - start.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    int steps = (int)std::min(20.0f, std::max(1.0f, distance / 25.0f));
    for (int i = 1; i <= steps; ++i)
    {
        if (globals::handlingtp) return;
        float t = (float)i / (float)steps;
        math::Vector3 pos{ start.x + dx * t, start.y + dy * t, start.z + dz * t };
        currentCFrame.position = pos;
        hrp.write_cframe(currentCFrame);
        hrp.write_velocity({ 0.0f, 0.0f, 0.0f });
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
    }

    currentCFrame.position = target;
    hrp.write_cframe(currentCFrame);
    hrp.write_velocity({ 0.0f, 0.0f, 0.0f });
}

// Only show Target HUD when there is a valid cached target
static inline bool shouldTargetHudBeActive() {
    return (globals::instances::cachedtarget.head.address != 0) ||
        (globals::instances::cachedlasttarget.head.address != 0);
}

// Force rescan function
void force_rescan() {
    // Clear cached players to force a fresh scan
    globals::instances::cachedplayers.clear();
    
    // Force immediate player cache refresh
    if (is_valid_address(globals::instances::datamodel.address)) {
        globals::instances::players = globals::instances::datamodel.read_service("Players");
    }
    
    // Clear target cache
    globals::instances::cachedtarget = {};
    globals::instances::cachedlasttarget = {};
    
    // Force immediate update - the cache will update faster now with reduced intervals
}

// Waypoint rendering removed
void render_waypoints() {
    return;

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    auto view_matrix = globals::instances::visualengine.GetViewMatrix();
    math::Vector3 local_pos = globals::instances::lp.hrp.get_pos();

    for (auto& waypoint : waypoints) {
        if (!waypoint.visible) continue;

        // Convert 3D position to 2D screen coordinates
        math::Vector3 waypoint_pos = waypoint.position;
        math::Vector2 screen_pos = roblox::instance::worldtoscreen(waypoint_pos);

        if (screen_pos.x != -1.0f && screen_pos.y != -1.0f) {
            // Calculate distance
            float distance = (waypoint_pos - local_pos).magnitude();

            // Color for waypoint visuals
            ImU32 waypoint_color = IM_COL32(0, 255, 255, 255); // Turquoise

            // Draw 3D circle effect around the waypoint (like sonar)
            float circle_radius = 4.0f;
            const int num_segments = 64; // Increased segments for smoother circle
            std::vector<ImVec2> screen_points;
            screen_points.reserve(num_segments + 1);

            // Get screen dimensions for bounds checking
            auto screen_dimensions = globals::instances::visualengine.get_dimensions();

            // Create 3D circle points around the waypoint position
            for (int i = 0; i <= num_segments; i++) {
                float angle = (2.0f * M_PI * i) / num_segments;

                // Create 3D circle point around the waypoint
                math::Vector3 circle_point_3d(
                    waypoint_pos.x + cos(angle) * circle_radius,
                    waypoint_pos.y, // Keep at waypoint height
                    waypoint_pos.z + sin(angle) * circle_radius
                );

                math::Vector2 circle_screen_pos = roblox::instance::worldtoscreen(circle_point_3d);


                // More lenient bounds checking to maintain circle integrity
                if (circle_screen_pos.x != -1.0f && circle_screen_pos.y != -1.0f &&
                    std::isfinite(circle_screen_pos.x) && std::isfinite(circle_screen_pos.y) &&
                    circle_screen_pos.x > -2000 && circle_screen_pos.x < screen_dimensions.x + 2000 &&
                    circle_screen_pos.y > -2000 && circle_screen_pos.y < screen_dimensions.y + 2000) {
                    screen_points.push_back(ImVec2(circle_screen_pos.x, circle_screen_pos.y));
                }
            }

            // Draw the 3D circle by connecting all points with improved gap handling
            if (screen_points.size() >= 12) { // Increased minimum points for better circle
                for (size_t i = 0; i < screen_points.size() - 1; i++) {
                    // Calculate distance between consecutive points
                    float dx = screen_points[i + 1].x - screen_points[i].x;
                    float dy = screen_points[i + 1].y - screen_points[i].y;
                    if (distance < 800.0f) {
                        draw_list->AddLine(
                            screen_points[i],
                            screen_points[i + 1],
                            waypoint_color,
                            2.0f
                        );
                    }
                }

                // Close the circle with improved gap handling
                if (screen_points.size() > 2) {
                    float dx = screen_points.front().x - screen_points.back().x;
                    float dy = screen_points.front().y - screen_points.back().y;
                    float distance = sqrtf(dx * dx + dy * dy);

                    if (distance < 800.0f) {
                        draw_list->AddLine(
                            screen_points.back(),
                            screen_points.front(),
                            waypoint_color,
                            2.0f
                        );
                    }
                }
            }

            // Draw waypoint icon at the 3D circle center (outer ring + inner dot)
            {
                float base_radius = 6.0f;
                // Slight size attenuation with distance
                float scaled = base_radius * (1.0f - std::min(distance, 500.0f) / 1000.0f);
                float r = std::max(3.0f, scaled);
                ImU32 ring_col = ImGui::GetColorU32(ImGuiCol_SliderGrab);
                ImU32 dot_col = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
                draw_list->AddCircle(ImVec2(screen_pos.x, screen_pos.y), r, ring_col, 24, 2.0f);
                draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), r * 0.45f, dot_col, 24);
            }

            // Draw distance near the base of the 3D line
            std::string distance_text = "[" + std::to_string((int)distance) + "m]";
            ImVec2 distance_pos(screen_pos.x - 20, screen_pos.y + 20);
            draw_list->AddText(distance_pos, waypoint_color, distance_text.c_str());
        }
    }
}

void render_desync_timer() {
    if (!globals::misc::targethud) return; // Only show when HUD is enabled
    if (!globals::misc::desync_active) return; // Only show when desync is active
    
    // Create a compact timer UI similar to the image
    static ImVec2 timer_pos = ImVec2(static_cast<float>(GetSystemMetrics(SM_CXSCREEN)) / 2.0f - 100.0f, 50.0f);
    
    ImGui::SetNextWindowPos(timer_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(200, 60), ImGuiCond_Always);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground;
    
    ImGui::Begin("DesyncTimer", nullptr, window_flags);
    
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    
    // Dark gray background with red border
    ImU32 bg_color = IM_COL32(47, 48, 46, 255); // Dark gray
    ImU32 border_color = IM_COL32(255, 0, 0, 255); // Red border
    ImU32 text_color = IM_COL32(255, 255, 255, 255); // White text
    ImU32 text_outline_color = IM_COL32(0, 0, 0, 255); // Black outline for text
    
    // Draw background
    draw->AddRectFilled(window_pos, window_pos + window_size, bg_color, 6.0f);
    
    // Draw red outline around the entire rectangle
    draw->AddRect(window_pos, window_pos + window_size, border_color, 6.0f, 0, 3.0f);
    
    // Calculate progress (0.0 to 1.0)
    float progress = globals::misc::desync_timer / globals::misc::desync_max_time;
    progress = std::min(progress, 1.0f);
    
    // Draw progress bar (squared corners)
    ImVec2 bar_start = ImVec2(window_pos.x + 10, window_pos.y + 35);
    ImVec2 bar_end = ImVec2(window_pos.x + window_size.x - 10, window_pos.y + 45);
    ImVec2 bar_fill_end = ImVec2(bar_start.x + (bar_end.x - bar_start.x) * progress, bar_end.y);
    
    // Progress bar background (squared)
    draw->AddRectFilled(bar_start, bar_end, IM_COL32(60, 60, 60, 255), 0.0f);
    // Progress bar fill - RED (squared)
    if (progress > 0.0f) {
        draw->AddRectFilled(bar_start, bar_fill_end, IM_COL32(255, 0, 0, 255), 0.0f);
    }
    
    // Text: "desync : false : 0.0s"
    std::string status_text = "desync : " + std::string(globals::misc::desync_active ? "true" : "false") + " : " + 
                             std::to_string(static_cast<int>(globals::misc::desync_timer)) + "." + 
                             std::to_string(static_cast<int>((globals::misc::desync_timer - static_cast<int>(globals::misc::desync_timer)) * 10)) + "s";
    
    // Center the text horizontally
    ImVec2 text_size = ImGui::CalcTextSize(status_text.c_str());
    ImVec2 text_pos = ImVec2(window_pos.x + (window_size.x - text_size.x) / 2.0f, window_pos.y + 10);
    
    // Draw text with outline (draw outline first, then text on top)
    draw->AddText(ImVec2(text_pos.x - 1, text_pos.y - 1), text_outline_color, status_text.c_str());
    draw->AddText(ImVec2(text_pos.x + 1, text_pos.y - 1), text_outline_color, status_text.c_str());
    draw->AddText(ImVec2(text_pos.x - 1, text_pos.y + 1), text_outline_color, status_text.c_str());
    draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), text_outline_color, status_text.c_str());
    draw->AddText(text_pos, text_color, status_text.c_str());
    
    ImGui::End();
}

void render_watermark() {
    if (!globals::misc::watermark) return;

    static ImVec2 watermark_pos = ImVec2(0, 0);
    static bool first_time = true;
    static bool was_menu_visible = false;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    float rounded = style.WindowRounding;
    style.WindowRounding = 4.0f; // Match theme rounding

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_s(&local_time, &time_t);

    char time_str[64];
    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &local_time);
    char date_str[64];
    std::strftime(date_str, sizeof(date_str), "%d-%m-%Y", &local_time);

    ImGuiIO& io = ImGui::GetIO();
    int fps = (int)(1.0f / io.DeltaTime);

    std::string watermark_text = "Aimquette";
    std::vector<std::string> info_parts;

    if (first_time && globals::misc::watermarkstuff != nullptr) {
        (*globals::misc::watermarkstuff)[0] = 1;
        (*globals::misc::watermarkstuff)[1] = 1;
    }

    if (globals::misc::watermarkstuff != nullptr) {
        if ((*globals::misc::watermarkstuff)[1] == 1) {
            // Get the actual player name instead of the hardcoded username
            std::string player_name = globals::instances::localplayer.get_name();
            if (player_name.empty()) {
                player_name = globals::instances::username; // Fallback to username if player name is empty
            }
            info_parts.push_back(player_name);
        }
        if ((*globals::misc::watermarkstuff)[2] == 1) {
            info_parts.push_back(std::string(date_str));
        }
        if ((*globals::misc::watermarkstuff)[0] == 1) {
            info_parts.push_back("FPS: " + std::to_string(fps));
        }
    }

    if (!info_parts.empty()) {
        watermark_text += " | ";
        for (size_t i = 0; i < info_parts.size(); i++) {
            if (i > 0) watermark_text += " | ";
            watermark_text += info_parts[i];
        }
    }

    ImVec2 text_size = ImGui::CalcTextSize(watermark_text.c_str());
    float padding_x = 8.0f;
    float padding_y = 6.0f;
    float total_width = text_size.x + (padding_x * 2);
    float total_height = text_size.y + (padding_y * 2);

    // Center watermark at top of screen - use fixed center point to prevent shaking
    float top_padding = 10.0f;
    static float saved_center_x = io.DisplaySize.x / 2.0f;
    
    // Detect menu state change (closed -> open or open -> closed)
    bool menu_state_changed = (was_menu_visible != overlay::visible);
    was_menu_visible = overlay::visible;
    
    // Update saved center only when display size changes
    if (saved_center_x != io.DisplaySize.x / 2.0f) {
        saved_center_x = io.DisplaySize.x / 2.0f;
    }
    
    // Make watermark stable like when menu is open - use Once condition to prevent shaking
    if (!overlay::visible) {
        // Menu is closed - center the watermark when state changes, then keep stable
        if (first_time || menu_state_changed || (watermark_pos.x < 100.0f && watermark_pos.y < 100.0f)) {
            // First time, menu just closed, or at corner - center it once
            ImVec2 centered_pos = ImVec2(saved_center_x - total_width / 2.0f, top_padding);
            ImGui::SetNextWindowPos(centered_pos, ImGuiCond_Always);
            watermark_pos = centered_pos;
            first_time = false;
        } else {
            // Already centered - use Once with fixed position to keep it stable
            // Don't recalculate X position when width changes - keep it fixed
            ImGui::SetNextWindowPos(watermark_pos, ImGuiCond_Once);
        }
    }
    else {
        // Menu is open - allow dragging, but center on first time or if at corner
        if (first_time || menu_state_changed || (watermark_pos.x < 100.0f && watermark_pos.y < 100.0f)) {
            // First time, menu just opened, or at corner position - center it once
            ImVec2 centered_pos = ImVec2(saved_center_x - total_width / 2.0f, top_padding);
            ImGui::SetNextWindowPos(centered_pos, ImGuiCond_Always);
            watermark_pos = centered_pos;
            first_time = false;
        } else {
            // Use saved position (allows dragging) - Once keeps it stable
            ImGui::SetNextWindowPos(watermark_pos, ImGuiCond_Once);
        }
    }

    ImGui::SetNextWindowSize(ImVec2(total_width, total_height), ImGuiCond_Always);
    ImGui::Begin("Watermark", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    // Only save position when menu is visible and watermark is not at corner
    if (overlay::visible && !(window_pos.x < 100.0f && window_pos.y < 100.0f)) {
        watermark_pos = window_pos;
    }

    // Match topbar style exactly: shadow, background, two accent lines, stroke border
    ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();
    
    // 1. Draw background shadow (black with 0.5 alpha, offset by 1px) - matches topbar
    draw->rect(bg_draw, window_pos - ImVec2(1, 1), window_pos + window_size + ImVec2(1, 1), draw->get_clr({0, 0, 0, 0.5f}));

    // 2. Draw main background (background_one) - matches topbar
    draw->rect_filled(draw_list, window_pos, window_pos + window_size, draw->get_clr(clr->window.background_one));

    // 3. Draw first accent line (full accent color) - matches topbar
    draw->line(draw_list, window_pos + ImVec2(1, 1), ImVec2(window_pos.x + window_size.x - 1, window_pos.y + 1), draw->get_clr(clr->accent), 1);

    // 4. Draw second accent line (accent with 0.4 alpha) - matches topbar
    draw->line(draw_list, window_pos + ImVec2(1, 2), ImVec2(window_pos.x + window_size.x - 1, window_pos.y + 2), draw->get_clr(clr->accent, 0.4f), 1);

    // 5. Draw stroke border - matches topbar
    draw->rect(draw_list, window_pos, window_pos + window_size, draw->get_clr(clr->window.stroke));

    // 6. Draw text - matches theme
    ImVec2 text_pos = ImVec2(
        window_pos.x + padding_x,
        window_pos.y + padding_y + 1
    );
    draw_list->AddText(
        text_pos,
        draw->get_clr(clr->widgets.text),
        watermark_text.c_str()
    );

    if (first_time) {
        first_time = false;
    }

    style.WindowRounding = rounded;
    ImGui::End();
}

void render_target_hud() {
    if (!globals::misc::targethud) return;
    if (!shouldTargetHudBeActive() && !overlay::visible) return;

    roblox::player target;
    bool hasTarget = false;

    if (overlay::visible) {
        target = globals::instances::lp;
        hasTarget = true;
    }
    else {
        if (globals::instances::cachedtarget.head.address != 0) {
            target = globals::instances::cachedtarget;
            hasTarget = true;
        }
        else if (globals::instances::cachedlasttarget.head.address != 0) {
            target = globals::instances::cachedlasttarget;
            hasTarget = true;
        }
    }

    if (!hasTarget || target.name.empty()) return;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    float rounded = style.WindowRounding;
    style.WindowRounding = 0.0f;

    static ImVec2 targethudpos = ImVec2(static_cast<float>(GetSystemMetrics(SM_CXSCREEN)) / 2.0f - 90.0f, static_cast<float>(GetSystemMetrics(SM_CYSCREEN)) / 2.0f + 120.0f);
    static bool first_time = true;
    static bool isDragging = false;
    static ImVec2 dragDelta;
    static float animatedHealth = 100.0f;
    static int lastHealth = 100;
    static float animationTimer = 0.0f;
    static float healthTextAlpha = 0.0f;
    static float healthTextTargetAlpha = 0.0f;
    static float healthTextLerpSpeed = 3.0f;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    if (first_time || !overlay::visible) {
        ImGui::SetNextWindowPos(targethudpos, ImGuiCond_Always);
        first_time = false;
    }

    int health = target.humanoid.read_health();
    int maxHealth = target.humanoid.read_maxhealth();

    if (maxHealth <= 0) maxHealth = 100;
    if (health < 0) health = 0;

    if (lastHealth != health) {
        if (health < lastHealth) {
            animationTimer = 1.0f;
            // Show health text when player gets hit (only if not at full health)
            if (health < maxHealth) {
                healthTextTargetAlpha = 1.0f;
            }
        }
        lastHealth = health;
    }

    // Hide health text when health is at full health
    if (health >= maxHealth) {
        healthTextTargetAlpha = 0.0f;
    }

    // Smooth lerp animation for health text alpha
    float deltaTime = ImGui::GetIO().DeltaTime;
    if (std::abs(healthTextAlpha - healthTextTargetAlpha) > 0.01f) {
        healthTextAlpha += (healthTextTargetAlpha - healthTextAlpha) * healthTextLerpSpeed * deltaTime;
    } else {
        healthTextAlpha = healthTextTargetAlpha;
    }

    float targetHealthPercentage = std::clamp(static_cast<float>(health) / maxHealth, 0.0f, 1.0f);
    float currentHealthPercentage = std::clamp(animatedHealth / maxHealth, 0.0f, 1.0f);

    if (animationTimer > 0.0f) {
        animationTimer = std::max(0.0f, animationTimer - ImGui::GetIO().DeltaTime);
    }

    if (std::abs(currentHealthPercentage - targetHealthPercentage) > 0.001f) {
        float animationSpeed = 1.0f * ImGui::GetIO().DeltaTime;
        if (targetHealthPercentage < currentHealthPercentage) {
            currentHealthPercentage = std::max(targetHealthPercentage, currentHealthPercentage - animationSpeed);
        }
        else {
            currentHealthPercentage = std::min(targetHealthPercentage, currentHealthPercentage + animationSpeed);
        }
        animatedHealth = currentHealthPercentage * maxHealth;
    }
    else {
        currentHealthPercentage = targetHealthPercentage;
    }

    const float PADDING = 5;
    float totalHeight = 50;
    ImVec2 windowSize(180, totalHeight);

    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::Begin("TargetHUD", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();

    if (overlay::visible) {
        targethudpos = window_pos;
    }

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImRect headerRect(targethudpos, targethudpos + windowSize);
    ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();

    ImU32 healthBarColor = draw->get_clr(clr->accent);
    if (animationTimer > 0.0f && health < lastHealth) {
        float flashIntensity = std::min(1.0f, animationTimer * 2.0f);
        // Flash effect using accent color with intensity
        ImColor accent = clr->accent;
        healthBarColor = IM_COL32(
            static_cast<int>(accent.Value.x * (0.5f + 0.5f * flashIntensity)),
            static_cast<int>(accent.Value.y * (0.5f + 0.5f * flashIntensity)),
            static_cast<int>(accent.Value.z * (0.5f + 0.5f * flashIntensity)),
            255
        );
    }

    if (ImGui::IsMouseClicked(0) && headerRect.Contains(mousePos) && overlay::visible) {
        isDragging = true;
        dragDelta = mousePos - targethudpos;
    }
    if (isDragging && ImGui::IsMouseDown(0)) {
        targethudpos = mousePos - dragDelta;
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        targethudpos.x = ImClamp(targethudpos.x, 0.0f, screenSize.x - windowSize.x);
        targethudpos.y = ImClamp(targethudpos.y, 0.0f, screenSize.y - totalHeight);
    }
    else {
        isDragging = false;
    }

    // Match theme style: shadow, background, accent lines, stroke border
    draw->rect(bg_draw, targethudpos - ImVec2(1, 1), targethudpos + windowSize + ImVec2(1, 1), draw->get_clr({0, 0, 0, 0.5f}));
    draw->rect_filled(draw_list, targethudpos, targethudpos + windowSize, draw->get_clr(clr->window.background_one));
    draw->line(draw_list, targethudpos + ImVec2(1, 1), ImVec2(targethudpos.x + windowSize.x - 1, targethudpos.y + 1), draw->get_clr(clr->accent), 1);
    draw->line(draw_list, targethudpos + ImVec2(1, 2), ImVec2(targethudpos.x + windowSize.x - 1, targethudpos.y + 2), draw->get_clr(clr->accent, 0.4f), 1);
    draw->rect(draw_list, targethudpos, targethudpos + windowSize, draw->get_clr(clr->window.stroke));

    int barwidth = static_cast<int>(windowSize.x - 60);
    int healthbarwidth = static_cast<int>(currentHealthPercentage * barwidth);

    ImVec2 healthBarBg_start = ImVec2(targethudpos.x + 55, targethudpos.y + 35);
    ImVec2 healthBarBg_end = ImVec2(targethudpos.x + 55 + barwidth, targethudpos.y + 39);

    draw->rect_filled(draw_list, healthBarBg_start, healthBarBg_end, draw->get_clr(clr->window.background_two));
    
    if (healthbarwidth > 0) {
        draw->rect_filled(draw_list, healthBarBg_start, ImVec2(targethudpos.x + 55 + healthbarwidth, targethudpos.y + 39), healthBarColor);
    }
    
    draw->rect(draw_list, healthBarBg_start, healthBarBg_end, draw->get_clr(clr->window.stroke));

    std::string healthText = std::to_string(health) + " / " + std::to_string(maxHealth);
    ImVec2 textSize = ImGui::CalcTextSize(healthText.c_str());

    float textX = targethudpos.x + 55 + (barwidth - textSize.x) / 2;
    float textY = targethudpos.y + 35 + (4 - textSize.y) / 2;

    auto* avatar_mgr = overlay::get_avatar_manager();
    if (avatar_mgr) {
        ImTextureID avatar_texture = avatar_mgr->getAvatarTexture(target.userid.address);

        if (avatar_texture) {
            draw_list->AddImage(
                avatar_texture,
                targethudpos + ImVec2(5, 5),
                targethudpos + ImVec2(45, 45)
            );
        }
        else {
            draw->rect_filled(draw_list, targethudpos + ImVec2(5, 5), targethudpos + ImVec2(45, 45), draw->get_clr(clr->window.background_two));
            draw_list->AddText(
                targethudpos + ImVec2(25 - ImGui::CalcTextSize("IMG").x / 2, 25 - ImGui::CalcTextSize("IMG").y / 2),
                draw->get_clr(clr->widgets.text_inactive),
                "IMG"
            );
        }
    }
    else {
        draw->rect_filled(draw_list, targethudpos + ImVec2(5, 5), targethudpos + ImVec2(45, 45), draw->get_clr(clr->window.background_two));
        draw_list->AddText(
            targethudpos + ImVec2(25 - ImGui::CalcTextSize("IMG").x / 2, 25 - ImGui::CalcTextSize("IMG").y / 2),
            draw->get_clr(clr->widgets.text_inactive),
            "IMG"
        );
    }

    draw_list->AddText(
        ImVec2(textX + 1, textY + 1),
        draw->get_clr({0, 0, 0, 0.7f}),
        healthText.c_str()
    );

    draw_list->AddText(
        ImVec2(textX, textY),
        draw->get_clr(clr->widgets.text),
        healthText.c_str()
    );

    std::string display_name = target.name.length() > 16 ? target.name.substr(0, 13) + "..." : target.name;
    draw_list->AddText(
        ImVec2(targethudpos.x + 55, targethudpos.y + 8),
        draw->get_clr(clr->widgets.text),
        display_name.c_str()
    );

    std::string tool_name = "No Tool";
    try {
        auto tool = target.instance.read_service("Tool");
        if (tool.address) {
            std::string tool_str = tool.get_name();
            if (!tool_str.empty() && tool_str != "nil") {
                tool_name = tool_str.length() > 15 ? tool_str.substr(0, 12) + "..." : tool_str;
            }
        }
    }
    catch (...) {
        tool_name = "Unknown";
    }

    draw_list->AddText(
        ImVec2(targethudpos.x + 55, targethudpos.y + 21),
        draw->get_clr(clr->widgets.text_inactive),
        tool_name.c_str()
    );

    style.WindowRounding = rounded;
    ImGui::End();
}



void overlay::initialize_avatar_system() {
    if (g_pd3dDevice && !avatar_manager) {
        avatar_manager = std::make_unique<AvatarManager>(g_pd3dDevice, g_pd3dDeviceContext);

    }
}

void overlay::update_avatars() {
    // Performance optimization: Reduce avatar update frequency
    static auto last_update = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

    // Only update avatars every 2 seconds instead of every frame
    if (elapsed.count() < 2000) {
        return;
    }

    if (avatar_manager) {
        avatar_manager->update();
    }

    last_update = now;
}

AvatarManager* overlay::get_avatar_manager() {
    return avatar_manager.get();
}

void overlay::cleanup_avatar_system() {
    if (avatar_manager) {
        avatar_manager.reset();
    }
}


static ImFont* g_panelFont = nullptr; // Smooth font for panel-only text

// Waypoint panel removed
void render_waypoint_panel() {
    return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Waypoints", &show_waypoint_panel, window_flags);
    if (g_panelFont) ImGui::PushFont(g_panelFont);
    // Smaller controls in this panel
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::SetWindowFontScale(0.90f);
    // Waypoints loading removed

    // Header
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    ImU32 bg_color = IM_COL32(15, 15, 15, 200);
    ImU32 text_color = IM_COL32(255, 255, 255, 255);
    ImU32 top_line_color = ImGui::GetColorU32(ImGuiCol_SliderGrab);

    draw->AddRectFilled(window_pos, ImVec2(window_pos.x + window_size.x, window_pos.y + 2.0f), top_line_color, 0.0f);
    draw->AddText(ImVec2(window_pos.x + 10.0f, window_pos.y + 8.0f), text_color, "Waypoints");

    ImGui::SetCursorPos(ImVec2(10.0f, 30.0f));

    ImGui::Separator();

    // Add new waypoint section
    ImGui::Text("Add New Waypoint:");
    ImGui::InputTextWithHint("##waypointname", "Waypoint name...", new_waypoint_name, sizeof(new_waypoint_name));

    // Button on next line to ensure it fits within the panel
    if (ImGui::Button("Add Current Position", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) {
        if (strlen(new_waypoint_name) > 0) {
            math::Vector3 current_pos = globals::instances::lp.hrp.get_pos();
            waypoints.emplace_back(std::string(new_waypoint_name), current_pos);
            memset(new_waypoint_name, 0, sizeof(new_waypoint_name));
        }
    }

    ImGui::Separator();

    // Waypoint list
    ImGui::Text("Saved Waypoints (%d):", (int)waypoints.size());

    if (waypoints.empty()) {
        ImGui::TextDisabled("No waypoints saved");
    }
    else {
        for (size_t i = 0; i < waypoints.size(); i++) {
            auto& waypoint = waypoints[i];
            ImGui::PushID(static_cast<int>(i));

            // Icon + Waypoint name and visibility toggle
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 icon_pos = ImGui::GetCursorScreenPos();
                float outer_r = 6.0f; // icon radius
                ImU32 accent = ImGui::GetColorU32(ImGuiCol_SliderGrab);
                ImU32 accent_fill = ImGui::GetColorU32(ImGuiCol_SliderGrabActive);
                // Outer ring
                dl->AddCircle(ImVec2(icon_pos.x + outer_r, icon_pos.y + outer_r + 2.0f), outer_r, accent, 20, 1.5f);
                // Inner dot
                dl->AddCircleFilled(ImVec2(icon_pos.x + outer_r, icon_pos.y + outer_r + 2.0f), outer_r * 0.45f, accent_fill, 20);
                // Advance cursor to leave space for icon
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + outer_r * 2.0f + 8.0f);
            }

            ImGui::Checkbox(("##visible" + std::to_string(i)).c_str(), &waypoint.visible);
            ImGui::SameLine();
            ImGui::Text("%s", waypoint.name.c_str());

            // Position info
            ImGui::SameLine();
            ImGui::TextDisabled("(%.1f, %.1f, %.1f)", waypoint.position.x, waypoint.position.y, waypoint.position.z);

            // Action buttons
            if (ImGui::Button("Teleport")) {
                safe_teleport_to(waypoint.position);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                waypoints.erase(waypoints.begin() + i);
                i--; // Adjust index after deletion
            }

            ImGui::PopID();
        }
    }

    ImGui::Separator();

    // Clear all button
    if (!waypoints.empty() && ImGui::Button("Clear All Waypoints")) {
        waypoints.clear();
    }

    // Restore styles
    ImGui::SetWindowFontScale(1.0f);
    if (g_panelFont) ImGui::PopFont();
    ImGui::PopStyleVar(2);
    ImGui::End();
}

void render_player_list() {
    if (!globals::misc::playerlist) return;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;
    float rounded = style.WindowRounding;
    style.WindowRounding = 0.0f;

    static ImVec2 playerlist_pos = ImVec2(500, 10);
    static bool first_time = true;
    static bool isDragging = false;
    static ImVec2 dragDelta;
    static int selected_player = -1;
    static float side_panel_animation = 0.0f;
    static std::vector<std::string> status_options = { "Enemy", "Friendly", "Neutral", "Client" };
    static std::vector<int> player_status;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    if (first_time || !overlay::visible) {
        ImGui::SetNextWindowPos(playerlist_pos, ImGuiCond_Always);
        first_time = false;
    }

    std::vector<roblox::player> players;
    if (globals::instances::cachedplayers.size() > 0) {
        players = globals::instances::cachedplayers;
    }

    if (player_status.size() != players.size()) {
        player_status.resize(players.size(), 2);

        for (size_t i = 0; i < players.size(); i++) {
            auto& player = players[i];

            if (player.name == globals::instances::lp.name) {
                player_status[i] = 3;
            }
            else if (std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), player.name) != globals::instances::whitelist.end()) {
                player_status[i] = 1;
            }
            else if (std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), player.name) != globals::instances::blacklist.end()) {
                player_status[i] = 0;
            }
        }
    }

    auto isWhitelisted = [](const std::string& name) {
        return std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), name) != globals::instances::whitelist.end();
        };

    auto isBlacklisted = [](const std::string& name) {
        return std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), name) != globals::instances::blacklist.end();
        };

    auto addToWhitelist = [](const std::string& name) {
        if (std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), name) == globals::instances::whitelist.end()) {
            globals::instances::whitelist.push_back(name);
        }
        auto it = std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), name);
        if (it != globals::instances::blacklist.end()) {
            globals::instances::blacklist.erase(it);
        }
        };

    auto addToBlacklist = [](const std::string& name) {
        if (std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), name) == globals::instances::blacklist.end()) {
            globals::instances::blacklist.push_back(name);
        }
        auto it = std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), name);
        if (it != globals::instances::whitelist.end()) {
            globals::instances::whitelist.erase(it);
        }
        };

    auto removeFromLists = [](const std::string& name) {
        auto it = std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), name);
        if (it != globals::instances::whitelist.end()) {
            globals::instances::whitelist.erase(it);
        }
        it = std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), name);
        if (it != globals::instances::blacklist.end()) {
            globals::instances::blacklist.erase(it);
        }
        };

    float content_width = 320.0f;
    float padding = 8.0f;
    float header_height = 30.0f;
    float player_item_height = 35.0f;
    float max_height = 500.0f;
    float side_panel_width = 300.0f;

    float target_animation = (selected_player >= 0) ? 1.0f : 0.0f;
    side_panel_animation += (target_animation - side_panel_animation) * 0.15f;
    float animated_side_width = side_panel_width * side_panel_animation;

    float total_width = content_width;
    float total_height = header_height + max_height + padding;

    ImGui::SetNextWindowSize(ImVec2(total_width + animated_side_width, total_height), ImGuiCond_Always);
    ImGui::Begin("PlayerList", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();

    if (overlay::visible) {
        playerlist_pos = window_pos;
    }

    ImVec2 mousePos = ImGui::GetIO().MousePos;
    ImRect headerRect(playerlist_pos, ImVec2(playerlist_pos.x + total_width + animated_side_width, playerlist_pos.y + header_height));
    ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();
    ImU32 text_color = draw->get_clr(clr->widgets.text);
    ImU32 enemy_color = IM_COL32(255, 100, 100, 255);
    ImU32 friendly_color = IM_COL32(100, 255, 100, 255);
    ImU32 neutral_color = draw->get_clr(clr->widgets.text_inactive);
    ImU32 client_color = draw->get_clr(clr->accent);

    // Match theme style: shadow, background, accent lines, stroke border
    draw->rect(bg_draw, window_pos - ImVec2(1, 1), ImVec2(window_pos.x + total_width + animated_side_width + 1, window_pos.y + total_height + 1), draw->get_clr({0, 0, 0, 0.5f}));
    draw->rect_filled(draw_list, window_pos, ImVec2(window_pos.x + total_width + animated_side_width, window_pos.y + total_height), draw->get_clr(clr->window.background_one));
    draw->line(draw_list, window_pos + ImVec2(1, 1), ImVec2(window_pos.x + total_width + animated_side_width - 1, window_pos.y + 1), draw->get_clr(clr->accent), 1);
    draw->line(draw_list, window_pos + ImVec2(1, 2), ImVec2(window_pos.x + total_width + animated_side_width - 1, window_pos.y + 2), draw->get_clr(clr->accent, 0.4f), 1);
    draw->rect(draw_list, window_pos, ImVec2(window_pos.x + total_width + animated_side_width, window_pos.y + total_height), draw->get_clr(clr->window.stroke));
    
    if (ImGui::IsMouseClicked(0) && headerRect.Contains(mousePos) && overlay::visible) {
        isDragging = true;
        dragDelta = mousePos - playerlist_pos;
    }
    if (isDragging && ImGui::IsMouseDown(0)) {
        playerlist_pos = mousePos - dragDelta;
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        playerlist_pos.x = ImClamp(playerlist_pos.x, 0.0f, screenSize.x - total_width - animated_side_width);
        playerlist_pos.y = ImClamp(playerlist_pos.y, 0.0f, screenSize.y - total_height);
    }
    else {
        isDragging = false;
    }

    draw_list->AddText(ImVec2(window_pos.x + padding, window_pos.y + 8), text_color, "Players");

    ImGui::SetCursorPos(ImVec2(padding, header_height));

    if (ImGui::BeginChild("PlayerListContent", ImVec2(total_width - padding * 2, max_height), true, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
        if (players.empty()) {
            ImGui::Text("No players found");
        }
        else {
            for (size_t i = 0; i < players.size(); i++) {
                auto& player = players[i];
                ImGui::PushID(static_cast<int>(i));

                bool is_selected = (selected_player == static_cast<int>(i));
                bool is_client = (player.name == globals::instances::lp.name);

                // Draw selection background
                if (is_selected) {
                    ImVec2 item_min = ImGui::GetCursorScreenPos();
                    ImVec2 item_max = ImVec2(item_min.x + total_width - padding * 2, item_min.y + player_item_height);
                    draw_list->AddRectFilled(item_min, item_max, draw->get_clr(clr->accent, 0.3f));
                }

                // Use Selectable for cleaner layout
                std::string display_name = player.name.length() > 18 ? player.name.substr(0, 15) + "..." : player.name;
                
                ImU32 status_color = neutral_color;
                std::string status_text = "Neutral";

                if (is_client) {
                    player_status[i] = 3;
                    status_color = client_color;
                    status_text = "Client";
                }
                else if (i < player_status.size()) {
                    switch (player_status[i]) {
                    case 0: status_color = enemy_color; status_text = "Enemy"; break;
                    case 1: status_color = friendly_color; status_text = "Friendly"; break;
                    case 2: status_color = neutral_color; status_text = "Neutral"; break;
                    case 3: status_color = client_color; status_text = "Client"; break;
                    }
                }

                // Create display text with status
                std::string full_text = display_name + " [" + status_text + "]";
                
                if (ImGui::Selectable(full_text.c_str(), is_selected, 0, ImVec2(0, player_item_height))) {
                    if (!is_client) {
                        selected_player = (selected_player == static_cast<int>(i)) ? -1 : static_cast<int>(i);
                    }
                }

                // Draw status color indicator and ID on the right
                ImVec2 item_min = ImGui::GetItemRectMin();
                ImVec2 item_max = ImGui::GetItemRectMax();
                
                // Status color dot
                ImVec2 dot_pos = ImVec2(item_max.x - 80, item_min.y + player_item_height / 2.0f);
                draw_list->AddCircleFilled(dot_pos, 4.0f, status_color);
                
                // ID text
                std::string id_text = std::to_string(player.userid.address);
                ImVec2 id_text_size = ImGui::CalcTextSize(id_text.c_str());
                draw_list->AddText(ImVec2(item_max.x - id_text_size.x - 10, item_min.y + (player_item_height - id_text_size.y) / 2.0f),
                    draw->get_clr(clr->widgets.text_inactive), id_text.c_str());

                ImGui::PopID();
            }
        }
    }
    ImGui::EndChild();

    static bool spectating = false;
    if (side_panel_animation > 0.01f && selected_player >= 0 && selected_player < static_cast<int>(players.size())) {
        auto& selected = players[selected_player];
        bool is_client = (selected.name == globals::instances::lp.name);

        float panel_x = window_pos.x + total_width;
        float panel_y = window_pos.y;

        draw->rect_filled(draw_list, ImVec2(panel_x, panel_y), ImVec2(panel_x + animated_side_width, panel_y + total_height),
            draw->get_clr(clr->window.background_two));
        draw->rect(draw_list, ImVec2(panel_x, panel_y), ImVec2(panel_x + animated_side_width, panel_y + total_height),
            draw->get_clr(clr->window.stroke));

        if (side_panel_animation > 0.8f && !is_client) {
            float pad = 15.0f;
            float y = panel_y + 25.0f;

            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            
            // Player name
            ImGui::Text("%s", selected.name.c_str());
            y += 25.0f;
            
            // Position
            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::TextColored(clr->widgets.text_inactive, "Position:");
            y += 20.0f;
            Vector3 pos = selected.hrp.get_pos();

            int health = selected.humanoid.read_health();
            int maxhealth = selected.humanoid.read_maxhealth();

            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::Text("X: %.0f, Y: %.0f, Z: %.0f", pos.x, pos.y, pos.z);
            y += 25.0f;
            
            // Health
            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::TextColored(clr->widgets.text_inactive, "Health:");
            y += 20.0f;
            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::Text("%d / %d", health, maxhealth);
            y += 25.0f;
            
            // Status
            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::TextColored(clr->widgets.text_inactive, "Status:");
            y += 20.0f;
            std::string status_display = selected_player < static_cast<int>(player_status.size()) ?
                status_options[player_status[selected_player]] : "Neutral";
            ImGui::SetCursorScreenPos(ImVec2(panel_x + pad, y));
            ImGui::Text("%s", status_display.c_str());
            y += 30.0f;

            float btn_y = y;
            float btn_w = (animated_side_width - pad * 3) * 0.5f;
            float btn_h = 22.0f;
            float btn_spacing = 6.0f;

            ImVec2 mouse_pos = ImGui::GetMousePos();
            bool mouse_clicked = ImGui::IsMouseClicked(0);

            auto draw_button = [&](ImVec2 pos, ImVec2 size, const char* text, bool& clicked) {
                bool hovered = mouse_pos.x >= pos.x && mouse_pos.x <= pos.x + size.x &&
                    mouse_pos.y >= pos.y && mouse_pos.y <= pos.y + size.y;

                ImU32 btn_color = draw->get_clr(clr->window.background_two);
                if (hovered) {
                    btn_color = draw->get_clr(clr->window.background_one);
                    if (mouse_clicked) {
                        clicked = true;
                        btn_color = draw->get_clr(clr->accent, 0.3f);
                    }
                }

                draw->rect_filled(draw_list, pos, ImVec2(pos.x + size.x, pos.y + size.y), btn_color);
                draw->rect(draw_list, pos, ImVec2(pos.x + size.x, pos.y + size.y), draw->get_clr(clr->window.stroke));

                ImVec2 text_size = ImGui::CalcTextSize(text);
                ImVec2 text_pos = ImVec2(pos.x + size.x * 0.5f - text_size.x * 0.5f,
                    pos.y + size.y * 0.5f - text_size.y * 0.5f);
                draw_list->AddText(text_pos, text_color, text);
                };

            bool spectate_clicked = false, kill_clicked = false, auto_kill_clicked = false,
                teleport_clicked = false;
            static std::string cool = "Spectate";
            if (spectating)
                cool = "Unspectate";
            else
                cool = "Spectate";

            draw_button(ImVec2(panel_x + pad, btn_y), ImVec2(btn_w, btn_h), cool.c_str(), spectate_clicked);
            draw_button(ImVec2(panel_x + pad + btn_w + pad, btn_y), ImVec2(btn_w, btn_h), "Kill", kill_clicked);

            draw_button(ImVec2(panel_x + pad, btn_y + btn_h + btn_spacing), ImVec2(btn_w, btn_h), "Auto Kill", auto_kill_clicked);
            draw_button(ImVec2(panel_x + pad, btn_y + (btn_h + btn_spacing) * 2),
                ImVec2(animated_side_width - pad * 2, btn_h), "Teleport", teleport_clicked);

            float combo_y = btn_y + (btn_h + btn_spacing) * 2;
            ImVec2 combo_pos = ImVec2(panel_x + pad, combo_y);
            ImVec2 combo_size = ImVec2(animated_side_width - pad * 2, btn_h);

            bool combo_hovered = mouse_pos.x >= combo_pos.x && mouse_pos.x <= combo_pos.x + combo_size.x &&
                mouse_pos.y >= combo_pos.y && mouse_pos.y <= combo_pos.y + combo_size.y;

            static bool combo_open = false;
            if (combo_hovered && mouse_clicked) {
                combo_open = !combo_open;
            }

            ImU32 combo_color = combo_hovered ? draw->get_clr(clr->window.background_one) : draw->get_clr(clr->window.background_two);
            draw->rect_filled(draw_list, combo_pos, ImVec2(combo_pos.x + combo_size.x, combo_pos.y + combo_size.y), combo_color);
            draw->rect(draw_list, combo_pos, ImVec2(combo_pos.x + combo_size.x, combo_pos.y + combo_size.y), draw->get_clr(clr->window.stroke));

            std::string combo_text = selected_player < static_cast<int>(player_status.size()) ?
                status_options[player_status[selected_player]] : "Neutral";
            ImVec2 combo_text_pos = ImVec2(combo_pos.x + 8, combo_pos.y + combo_size.y * 0.5f - ImGui::CalcTextSize(combo_text.c_str()).y * 0.5f);
            draw_list->AddText(combo_text_pos, text_color, combo_text.c_str());

            draw_list->AddText(ImVec2(combo_pos.x + combo_size.x - 15, combo_text_pos.y), text_color, "v");

            if (combo_open) {
                float dropdown_y = combo_pos.y + combo_size.y + 2;
                for (int i = 0; i < 3; i++) {
                    ImVec2 item_pos = ImVec2(combo_pos.x, dropdown_y + i * btn_h);
                    bool item_hovered = mouse_pos.x >= item_pos.x && mouse_pos.x <= item_pos.x + combo_size.x &&
                        mouse_pos.y >= item_pos.y && mouse_pos.y <= item_pos.y + btn_h;

                    ImU32 item_color = item_hovered ? draw->get_clr(clr->window.background_one) : draw->get_clr(clr->window.background_two);
                    draw->rect_filled(draw_list, item_pos, ImVec2(item_pos.x + combo_size.x, item_pos.y + btn_h), item_color);
                    draw->rect(draw_list, item_pos, ImVec2(item_pos.x + combo_size.x, item_pos.y + btn_h), draw->get_clr(clr->window.stroke));

                    ImVec2 item_text_pos = ImVec2(item_pos.x + 8, item_pos.y + btn_h * 0.5f - ImGui::CalcTextSize(status_options[i].c_str()).y * 0.5f);
                    draw_list->AddText(item_text_pos, text_color, status_options[i].c_str());

                    if (item_hovered && mouse_clicked && selected_player >= 0 && selected_player < static_cast<int>(player_status.size())) {
                        int old_status = player_status[selected_player];
                        player_status[selected_player] = i;

                        if (i == 0) {
                            addToBlacklist(selected.name);
                        }
                        else if (i == 1) {
                            addToWhitelist(selected.name);
                        }
                        else if (i == 2) {
                            removeFromLists(selected.name);
                        }

                        combo_open = false;
                    }
                }
            }

            if (mouse_clicked && !combo_hovered && combo_open) {
                combo_open = false;
            }

            if (spectate_clicked) {
                roblox::instance cam;
                if (!spectating) {
                    spectating = true;
                    cam.spectate(selected.hrp.address);
                }
                else {
                    spectating = false;
                    cam.unspectate();
                }
            }
            if (kill_clicked) {
                globals::bools::name = selected.name;
                globals::bools::entity = selected;
                globals::bools::kill = true;
            }
            if (auto_kill_clicked) {
                globals::bools::name = selected.name;
                globals::bools::entity = selected;
                globals::bools::autokill = true;
            }
            if (teleport_clicked) {
                globals::instances::lp.hrp.write_position(selected.hrp.get_pos());
            }
        }
        else if (is_client && side_panel_animation > 0.8f) {
            float pad = 20.0f;
            float info_x = panel_x + pad;
            float y = panel_y + 25.0f;

            draw_list->AddText(ImVec2(info_x, y), client_color, "This is you!");
            draw_list->AddText(ImVec2(info_x, y + 25), text_color, selected.name.c_str());
            draw_list->AddText(ImVec2(info_x, y + 50), draw->get_clr(clr->widgets.text_inactive), "Status: Client");
        }
    }

    style.WindowRounding = rounded;
    ImGui::End();
}

// Player list tab removed
void render_player_list_tab() {
    // Player list tab functionality has been removed
}


static std::string get_target_process_name()
{
    HWND target = FindWindowA(nullptr, "Roblox");
    if (target == nullptr)
        target = GetForegroundWindow();
    if (target == nullptr)
        return std::string("unknown");

    DWORD pid = 0;
    GetWindowThreadProcessId(target, &pid);
    if (pid == 0)
        return std::string("unknown");

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc)
        return std::string("unknown");

    wchar_t wbuf[1024];
    DWORD size = static_cast<DWORD>(std::size(wbuf));
    std::string result = "unknown";
    if (QueryFullProcessImageNameW(hProc, 0, wbuf, &size))
    {
        std::wstring full(wbuf);
        size_t pos = full.find_last_of(L"\\/");
        std::wstring fname = (pos == std::wstring::npos) ? full : full.substr(pos + 1);
        // Convert to UTF-8
        int needed = WideCharToMultiByte(CP_UTF8, 0, fname.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (needed > 0)
        {
            std::string utf8(needed - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, fname.c_str(), -1, utf8.data(), needed, nullptr, nullptr);
            result = utf8;
        }
    }
    CloseHandle(hProc);
    return result;
}


// Keybind list removed
static inline void render_keybind_list() {
    if (!globals::misc::keybinds) return;

    ImGuiContext& g = *GImGui;
    ImGuiStyle& style = g.Style;

    float rounded;
    rounded = style.WindowRounding;
    style.WindowRounding = 4.0f; // Match theme rounding



    static ImVec2 keybind_pos = ImVec2(5.0f, static_cast<float>(GetSystemMetrics(SM_CYSCREEN)) / 2.0f - 10.0f);
    static bool first_time = true;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (!overlay::visible) {
        window_flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove;
    }

    if (first_time || (!overlay::visible)) {
        ImGui::SetNextWindowPos(keybind_pos, ImGuiCond_Always);
        first_time = false;
    }

    std::vector<std::pair<std::string, std::string>> active_keybinds;

    if (globals::combat::aimbot && globals::combat::aimbotkeybind.enabled) {
        active_keybinds.push_back({ "Aimbot", globals::combat::aimbotkeybind.get_key_name() });
    }

    if (globals::combat::silentaim && globals::combat::silentaimkeybind.enabled) {
        active_keybinds.push_back({ "Silent Aim", globals::combat::silentaimkeybind.get_key_name() });
    }

    if (globals::misc::desync && globals::misc::desynckeybind.enabled) {
        active_keybinds.push_back({ "Desync", globals::misc::desynckeybind.get_key_name() });
    }

    if (globals::misc::speed && globals::misc::speedkeybind.enabled) {
        active_keybinds.push_back({ "Speed", globals::misc::speedkeybind.get_key_name() });
    }

    if (globals::misc::jumppower && globals::misc::jumppowerkeybind.enabled) {
        active_keybinds.push_back({ "Jump Power", globals::misc::jumppowerkeybind.get_key_name() });
    }

    if (globals::misc::flight && globals::misc::flightkeybind.enabled) {
        active_keybinds.push_back({ "Flight", globals::misc::flightkeybind.get_key_name() });
    }



    if (globals::misc::keybindsstyle == 1) {
        struct KeybindInfo {
            std::string name;
            keybind* bind;
            bool* enabled;
        };

        std::vector<KeybindInfo> all_keybinds = {
            {"Aimbot", &globals::combat::aimbotkeybind, &globals::combat::aimbot},
            {"Silent Aim", &globals::combat::silentaimkeybind, &globals::combat::silentaim},
            {"Desync", &globals::misc::desynckeybind, &globals::misc::desync},
            {"Speed", &globals::misc::speedkeybind, &globals::misc::speed},
            {"Jump Power", &globals::misc::jumppowerkeybind, &globals::misc::jumppower},
            {"Flight", &globals::misc::flightkeybind, &globals::misc::flight}
        };

        active_keybinds.clear();
        for (const auto& info : all_keybinds) {
            if (*info.enabled) {
                active_keybinds.push_back({ info.name, info.bind->get_key_name() });
            }
        }
    }

    ImVec2 title_size = ImGui::CalcTextSize("Keybinds");
    float content_width = title_size.x;

    for (const auto& bind : active_keybinds) {
        std::string full_text = bind.first + ": " + bind.second;
        ImVec2 text_size = ImGui::CalcTextSize(full_text.c_str());
        content_width = std::max(content_width, text_size.x);
    }

    float padding_x = 3.0f;
    float padding_y = 3.0f;
    float line_spacing = ImGui::GetTextLineHeight() + 2.0f;

    float total_width = content_width + (padding_x * 2) + 1.0f;
    float total_height = padding_y * 2 + title_size.y + 2;

    if (!active_keybinds.empty()) {
        total_height += active_keybinds.size() * line_spacing;
    }

    ImGui::SetNextWindowSize(ImVec2(total_width, total_height), ImGuiCond_Always);

    ImGui::Begin("Keybinds", nullptr, window_flags);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImDrawList* bg_draw = ImGui::GetBackgroundDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    if (overlay::visible) {
        keybind_pos = window_pos;
    }

    // Draw background shadow (black with 0.5 alpha, offset by 1px) - matches watermark
    draw->rect(bg_draw, window_pos - ImVec2(1, 1), window_pos + window_size + ImVec2(1, 1), draw->get_clr({0, 0, 0, 0.5f}));

    // Draw main background (background_one) - matches watermark
    draw->rect_filled(draw_list, window_pos, window_pos + window_size, draw->get_clr(clr->window.background_one));

    // Draw first accent line (full accent color) - matches watermark
    draw->line(draw_list, window_pos + ImVec2(1, 1), ImVec2(window_pos.x + window_size.x - 1, window_pos.y + 1), draw->get_clr(clr->accent), 1);

    // Draw second accent line (accent with 0.4 alpha) - matches watermark
    draw->line(draw_list, window_pos + ImVec2(1, 2), ImVec2(window_pos.x + window_size.x - 1, window_pos.y + 2), draw->get_clr(clr->accent, 0.4f), 1);

    // Draw stroke border - matches watermark
    draw->rect(draw_list, window_pos, window_pos + window_size, draw->get_clr(clr->window.stroke));

    // Calculate title position to center it horizontally
    // Use the actual content width (window size minus any borders)
    float title_x = window_pos.x + window_size.x / 2.0f - title_size.x / 2.0f;
    ImVec2 title_pos = ImVec2(
        title_x,
        window_pos.y + padding_y
    );

    // Draw title using theme colors
    draw->text_outline(draw_list, var->font.tahoma, var->font.tahoma->FontSize, title_pos, draw->get_clr(clr->widgets.text), "Keybinds");

    if (!active_keybinds.empty()) {
        // Draw separator line between title and keybinds
        float separator_y = title_pos.y + title_size.y + 4.0f;
        draw->line(draw_list, 
            ImVec2(window_pos.x + padding_x, separator_y), 
            ImVec2(window_pos.x + window_size.x - padding_x, separator_y), 
            draw->get_clr(clr->window.stroke), 
            1.0f);
        
        float current_y = separator_y + 4.0f;

        std::sort(active_keybinds.begin(), active_keybinds.end(),
            [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) {
                std::string full_a = a.first + ": " + a.second;
                std::string full_b = b.first + ": " + b.second;
                return full_a.length() > full_b.length();
            });

        for (const auto& bind : active_keybinds) {
            std::string full_text = bind.first + ": " + bind.second;
            
            // Calculate text size to center it
            ImVec2 text_size = ImGui::CalcTextSize(full_text.c_str());
            float text_x = window_pos.x + (window_size.x - text_size.x) / 2.0f; // Center horizontally
            ImVec2 keybind_pos = ImVec2(text_x, current_y);

            // Draw keybind text using theme colors
            if (globals::misc::keybindsstyle == 1) {
                bool is_active = false;
                if (bind.first == "Aimbot") is_active = globals::combat::aimbotkeybind.enabled;
                else if (bind.first == "Silent Aim") is_active = globals::combat::silentaimkeybind.enabled;
                else if (bind.first == "Desync") is_active = globals::misc::desynckeybind.enabled;
                else if (bind.first == "Speed") is_active = globals::misc::speedkeybind.enabled;
                else if (bind.first == "Jump Power") is_active = globals::misc::jumppowerkeybind.enabled;
                else if (bind.first == "Flight") is_active = globals::misc::flightkeybind.enabled;

                draw->text_outline(draw_list, var->font.tahoma, var->font.tahoma->FontSize, keybind_pos, draw->get_clr(is_active ? clr->accent : clr->widgets.text), full_text.c_str());
            }
            else {
                draw->text_outline(draw_list, var->font.tahoma, var->font.tahoma->FontSize, keybind_pos, draw->get_clr(clr->widgets.text), full_text.c_str());
            }

            current_y += line_spacing;
        }
    }
    style.WindowRounding = rounded;
    ImGui::End();
}



bool Bind(keybind* keybind, const ImVec2& size_arg = ImVec2(0, 0), bool clicked = false, ImGuiButtonFlags flags = 0) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(keybind->get_name().c_str());
    char popup_name[128];
    snprintf(popup_name, sizeof(popup_name), "%s##%08X", keybind->get_name().c_str(), id);
    const ImVec2 label_size = ImGui::CalcTextSize(keybind->get_name().c_str(), NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) &&
        style.FramePadding.y < window->DC.CurrLineTextBaseOffset)
        pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;

    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    if (g.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
        flags |= ImGuiButtonFlags_PressedOnRelease;

    bool hovered, held;
    bool Pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

    bool value_changed = false;
    int key = keybind->key;

    auto io = ImGui::GetIO();

    std::string name = keybind->get_key_name();

    if (keybind->waiting_for_input) {
        name = "[Waiting]";
    }


    if (ImGui::GetIO().MouseClicked[0] && hovered)
    {
        if (g.ActiveId == id)
        {
            keybind->waiting_for_input = true;
        }
    }
    else if (ImGui::GetIO().MouseClicked[1] && hovered)
    {
        ImGui::OpenPopup(popup_name);
    }
    else if (ImGui::GetIO().MouseClicked[0] && !hovered)
    {
        if (g.ActiveId == id)
            ImGui::ClearActiveID();
    }

    if (keybind->waiting_for_input)
    {
        if (ImGui::GetIO().MouseClicked[0] && !hovered)
        {
            keybind->key = VK_LBUTTON;
            ImGui::ClearActiveID();
            keybind->waiting_for_input = false;
        }
        else
        {
            if (keybind->set_key())
            {
                ImGui::ClearActiveID();
                keybind->waiting_for_input = false;
            }
        }
    }

    ImVec4 bgColor = ImVec4(254.0f / 255.0f, 208.0f / 255.0f, 2.0f / 255.0f, 1.0f);
    ImVec4 borderColor = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
    ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    window->DrawList->AddText(bb.Min + ImVec2(size_arg.x / 2 - ImGui::CalcTextSize(name.c_str()).x / 2, size_arg.y / 2 - ImGui::CalcTextSize(name.c_str()).y / 2), ImGui::GetColorU32(textColor), name.c_str());

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;

    if (ImGui::BeginPopup(popup_name))
    {
        {


            if (ImGui::Selectable("Hold", keybind->type == keybind::HOLD)) keybind->type = keybind::HOLD;
            if (ImGui::Selectable("Toggle", keybind->type == keybind::TOGGLE)) keybind->type = keybind::TOGGLE;
            if (ImGui::Selectable("Always", keybind->type == keybind::ALWAYS)) keybind->type = keybind::ALWAYS;

        }
        ImGui::EndPopup();

    }

    return Pressed;
}

void draw_shadowed_text(const char* label) {
    ImGuiContext& g = *GImGui;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    const ImGuiStyle& style = g.Style;
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    float HeaderHeight = ImGui::GetFontSize() + style.WindowPadding.y * 2 + style.ChildBorderSize * 2;
    pos.y = pos.y - 4;

    pDrawList->AddText(pos + style.WindowPadding + ImVec2(0, style.ChildBorderSize * 2) + ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_TitleBg), label);
    pDrawList->AddText(pos + style.WindowPadding + ImVec2(0, style.ChildBorderSize * 2) + ImVec2(-1, -1), ImGui::GetColorU32(ImGuiCol_TitleBg), label);
    pDrawList->AddText(pos + style.WindowPadding + ImVec2(0, style.ChildBorderSize * 2) + ImVec2(1, -1), ImGui::GetColorU32(ImGuiCol_TitleBg), label);
    pDrawList->AddText(pos + style.WindowPadding + ImVec2(0, style.ChildBorderSize * 2) + ImVec2(-1, 1), ImGui::GetColorU32(ImGuiCol_TitleBg), label);
    pDrawList->AddText(pos + style.WindowPadding + ImVec2(0, style.ChildBorderSize * 2), ImGui::GetColorU32(ImGuiCol_Text), label);

    ImGui::SetCursorPosY(HeaderHeight - style.WindowPadding.y + 2);
}

void overlay::load_interface()
{

    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"Aimquette", WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), nullptr, nullptr, wc.hInstance, nullptr);

    wc.cbClsExtra = NULL;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.cbWndExtra = NULL;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = L"base";
    wc.lpszMenuName = nullptr;
    wc.style = CS_VREDRAW | CS_HREDRAW;

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margin);



    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;



    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 0.0f;               // Square combo/input frames (eto-like)
    style.GrabMinSize = 6.0f;                 // Make sliders smaller (default is 12.0f)
    style.FrameBorderSize = 0.0f;             // No border
    style.ItemSpacing = ImVec2(6.0f, 4.0f);   // Slightly tighter spacing like eto
    style.ItemInnerSpacing = ImVec2(5.0f, 3.0f);
    style.FramePadding = ImVec2(6.0f, 3.0f);  // Smaller widgets globally
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.35f, 0.35f, 0.35f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    // Inputs/combos: neutral idle, purple hover/active (matching image theme)
    colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);           // Idle
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);    // Hover (purple)
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);     // Active (purple)
    colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.25f, 0.45f, 1.00f);         // Hide tick (same as fill)
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.18f, 0.35f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    // Checkbox check color: purple accent (#656BB8 = 101, 107, 184)
    colors[ImGuiCol_CheckMark] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    // Slider grab: purple accent
    colors[ImGuiCol_SliderGrab] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.47f, 0.78f, 1.00f);
    // Buttons/headers: purple theme
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.18f, 0.30f, 1.00f);            // Button base (darker purple)
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.28f, 0.45f, 1.00f);     // Hover purple
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.32f, 0.55f, 1.00f);      // Active purple
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.28f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.32f, 0.55f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.396f, 0.420f, 0.722f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.396f, 0.420f, 0.722f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.396f, 0.420f, 0.722f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.396f, 0.420f, 0.722f, 0.95f);
    // Sub tab colors: purple theme
    colors[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.32f, 0.50f, 0.85f);
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.13f, 0.22f, 0.90f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.25f, 0.22f, 0.38f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.12f, 0.10f, 0.18f, 0.95f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.22f, 0.20f, 0.32f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.396f, 0.420f, 0.722f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_WindowShadow] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);





    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    // Load Smallest Pixel-7 font if available (original) and a smooth font for panels
    {
        ImFontConfig cfg; cfg.OversampleH = 1; cfg.OversampleV = 1; cfg.PixelSnapH = true;
        io.Fonts->AddFontDefault();
        io.Fonts->AddFontFromFileTTF("resources/fonts/smallest_pixel-7.ttf", 13.0f, &cfg, io.Fonts->GetGlyphRangesDefault());

        // Smooth TTF font for panel (system Segoe UI if available)
        ImFontConfig smoothCfg; smoothCfg.OversampleH = 3; smoothCfg.OversampleV = 3; smoothCfg.PixelSnapH = false;
        const char* segoePath = "C:\\Windows\\Fonts\\segoeui.ttf";
        try {
            if (std::filesystem::exists(segoePath)) {
                g_panelFont = io.Fonts->AddFontFromFileTTF(segoePath, 14.0f, &smoothCfg, io.Fonts->GetGlyphRangesDefault());
            }
        }
        catch (...) { /* ignore */ }

        // Initialize framework fonts
        {
            ImFontConfig frameworkCfg;
            frameworkCfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_MonoHinting | ImGuiFreeTypeBuilderFlags_MonoHinting;
            frameworkCfg.FontDataOwnedByAtlas = false;
            
            var->font.icons[0] = io.Fonts->AddFontFromMemoryTTF(section_icons_hex, sizeof section_icons_hex, 15.f, &frameworkCfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.icons[1] = io.Fonts->AddFontFromMemoryTTF(icons_hex, sizeof icons_hex, 5.f, &frameworkCfg, io.Fonts->GetGlyphRangesCyrillic());
            var->font.tahoma = io.Fonts->AddFontFromMemoryTTF(tahoma_hex, sizeof tahoma_hex, 13.f, &frameworkCfg, io.Fonts->GetGlyphRangesCyrillic());
        }

        io.Fonts->Build();
    }

    // Dynamic font loader: scan resources/fonts and build a list for UI selection
    static std::vector<std::string> g_fontFiles;
    static int g_selectedFont = 0; // 0 = default, 1..N map to g_fontFiles entries
    static bool g_pendingFontReload = false;
    g_fontFiles.clear();
    try {
        std::string fontDir = "resources/fonts";
        if (std::filesystem::exists(fontDir)) {
            for (const auto& entry : std::filesystem::directory_iterator(fontDir)) {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                for (auto& c : ext) c = (char)tolower((unsigned char)c);
                if (ext == ".ttf" || ext == ".otf") {
                    g_fontFiles.push_back(entry.path().string());
                }
            }
        }
    }
    catch (...) {}

    ImVec4 clear_color = ImVec4(0.f, 0.f, 0.f, 0.f);

    // (removed ESP font override)

    bool done = false;

    initialize_avatar_system();

    // Start animation changer thread
    std::thread animation_thread([]() {
        hooks::animation();
    });
    animation_thread.detach();

    while (done == false)
    {
        if (!globals::firstreceived)return;


        auto avatar_mgr = overlay::get_avatar_manager();
        for (roblox::player entity : globals::instances::cachedplayers) {

            if (avatar_mgr) {
                if (!entity.pictureDownloaded) {
                    // Team check - skip teammates if teamcheck is enabled
                    if (globals::is_teammate(entity)) {
                        continue; // Skip teammate
                    }

                    // Performance optimization: Limit avatar requests per frame
                    static int avatar_requests_this_frame = 0;
                    static auto last_avatar_reset = std::chrono::steady_clock::now();
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_avatar_reset);

                    if (elapsed.count() >= 100) { // Reset counter every 100ms
                        avatar_requests_this_frame = 0;
                        last_avatar_reset = now;
                    }

                    if (avatar_requests_this_frame < 3) { // Max 3 avatar requests per 100ms
                        avatar_mgr->requestAvatar(entity.userid.address);
                        avatar_requests_this_frame++;
                    }
                }
                else {
                    continue;
                }

            }
            else {
                break;
            }
        }


        static HWND robloxWindow = FindWindowA(0, "Roblox");
        robloxWindow = FindWindowA(0, "Roblox");

        // Periodic Roblox process check (every 5 seconds)
        static auto lastRobloxCheck = std::chrono::steady_clock::now();
        auto currentTime = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastRobloxCheck).count() >= 5) {
            lastRobloxCheck = currentTime;

            // Check if RobloxPlayerBeta.exe is actually running
            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            bool robloxRunning = false;

            if (snapshot != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe32;
                pe32.dwSize = sizeof(PROCESSENTRY32);

                if (Process32First(snapshot, &pe32)) {
                    do {
                        if (strcmp(pe32.szExeFile, "RobloxPlayerBeta.exe") == 0) {
                            robloxRunning = true;
                            break;
                        }
                    } while (Process32Next(snapshot, &pe32));
                }
                CloseHandle(snapshot);
            }

            if (!robloxRunning) {
                // Roblox is not running, close the application
                std::cout << "[OVERLAY] Roblox not detected, closing application..." << std::endl;
                Sleep(1000); // Give user time to see the message
                exit(0);
            }
        }

        update_avatars();

        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                done = true;
                break;
            }
        }

        if (done == true)
        {
            break;
        }

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        movewindow(hwnd);







        // Legacy targeting flags reset removed - flags are now controlled by checkboxes in GUI





        if (FindWindowA(0, "Roblox") && (GetForegroundWindow() == FindWindowA(0, "Roblox") || GetForegroundWindow() == hwnd)) {
            globals::focused = true;
        }
        else {
            globals::focused = false;
        }
        if (FindWindowA(0, "Roblox") && (GetForegroundWindow() == FindWindowA(0, "Roblox") || GetForegroundWindow() == hwnd))
        {


            static bool firsssssssssssssssss = true;
            if (globals::focused && firsssssssssssssssss) {
                overlay::visible = true;
                firsssssssssssssssss = false;
            }

            // Menu key handling with toggle/hold mode support
            static bool last_menu_key_state = false;
            static DWORD last_menu_key_time = 0;
            
            // Check if any keybind widget is being edited - if so, ignore menu key
            ImGuiContext& g = *GImGui;
            bool keybind_being_edited = (g.ActiveId != 0);
            
            // Only process menu key if not editing a keybind
            if (!keybind_being_edited && var->gui.menu_key != 0) {
                bool current_key_state = (GetAsyncKeyState(var->gui.menu_key) & 0x8000) != 0;
                bool key_just_pressed = current_key_state && !last_menu_key_state;
                
                // Add a small cooldown to prevent rapid toggling
                DWORD current_time = GetTickCount64();
                bool cooldown_passed = (current_time - last_menu_key_time) > 200; // 200ms cooldown
                
                if (var->gui.menu_key_mode == 0) {
                    // Toggle mode - menu toggles on key press
                    if (key_just_pressed && cooldown_passed) {
                        var->gui.menu_opened = !var->gui.menu_opened;
                        last_menu_key_time = current_time;
                    }
                } else if (var->gui.menu_key_mode == 1) {
                    // Hold mode - menu is open while key is held
                    var->gui.menu_opened = current_key_state;
                    if (key_just_pressed) {
                        last_menu_key_time = current_time;
                    }
                }
                
                last_menu_key_state = current_key_state;
            } else {
                // Reset key state tracking if we're editing a keybind
                last_menu_key_state = false;
            }
            
            // Sync overlay visibility with menu opened state
            overlay::visible = var->gui.menu_opened;

            static float ui_alpha = 0.0f;
            const float fade_speed_per_sec = 8.0f; // higher = faster fade
            float target_alpha = overlay::visible ? 1.0f : 0.0f;
            float dt_fade = ImGui::GetIO().DeltaTime;
            if (ui_alpha < target_alpha) ui_alpha = ImMin(target_alpha, ui_alpha + dt_fade * fade_speed_per_sec);
            else if (ui_alpha > target_alpha) ui_alpha = ImMax(target_alpha, ui_alpha - dt_fade * fade_speed_per_sec);

            if (overlay::visible || ui_alpha > 0.01f)
            {


                // Render user's features menu (all features are in gui->render())
                gui->render();

            }
            
            if (globals::combat::drawfov) {
                POINT cursor_pos;
                GetCursorPos(&cursor_pos);
                ScreenToClient(robloxWindow, &cursor_pos);
                ImVec2 mousepos = ImVec2(static_cast<float>(static_cast<int>(cursor_pos.x)), static_cast<float>(static_cast<int>(cursor_pos.y)));
                ImDrawList* drawbglist = ImGui::GetBackgroundDrawList();
                drawbglist->AddCircle(mousepos, globals::combat::fovsize - 1, IM_COL32(0, 0, 0, 255));
                drawbglist->AddCircle(mousepos, globals::combat::fovsize, ImGui::ColorConvertFloat4ToU32(ImVec4(globals::combat::fovcolor[0], globals::combat::fovcolor[1], globals::combat::fovcolor[2], globals::combat::fovcolor[3])));
                drawbglist->AddCircle(mousepos, globals::combat::fovsize + 1, IM_COL32(0, 0, 0, 255));
            }
            if (globals::combat::drawsfov) {
                POINT cursor_pos;
                GetCursorPos(&cursor_pos);
                ScreenToClient(robloxWindow, &cursor_pos);
                ImVec2 mousepos = ImVec2(static_cast<float>(static_cast<int>(cursor_pos.x)), static_cast<float>(static_cast<int>(cursor_pos.y)));
                ImDrawList* drawbglist = ImGui::GetBackgroundDrawList();
                drawbglist->AddCircle(mousepos, globals::combat::sfovsize - 1, IM_COL32(0, 0, 0, 255));
                drawbglist->AddCircle(mousepos, globals::combat::sfovsize, ImGui::ColorConvertFloat4ToU32(ImVec4(globals::combat::sfovcolor[0], globals::combat::sfovcolor[1], globals::combat::sfovcolor[2], globals::combat::sfovcolor[3])));
                drawbglist->AddCircle(mousepos, globals::combat::sfovsize + 1, IM_COL32(0, 0, 0, 255));
            }

            // All features are rendered through gui->render() above
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            overlay::visible = false;
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin("esp", NULL,
            ImGuiWindowFlags_NoBackground
            |
            ImGuiWindowFlags_NoResize
            |
            ImGuiWindowFlags_NoCollapse
            |
            ImGuiWindowFlags_NoTitleBar
            |
            ImGuiWindowFlags_NoInputs
            |
            ImGuiWindowFlags_NoMouseInputs
            |
            ImGuiWindowFlags_NoDecoration
            |
            ImGuiWindowFlags_NoMove); {

            globals::instances::draw = ImGui::GetBackgroundDrawList();
            // Snow background effect removed
            globals::in_render_loop = true;
            
            try {
                visuals::run();
            }
            catch (...) {
                printf("Overlay: visuals::run threw, skipping frame\n");
                globals::in_render_loop = false;
            }
            globals::in_render_loop = false;
            

            // No custom images

        }

        ImGui::End();

        // Render watermark and other overlay elements
        render_watermark();
        
        // Render keybind list if enabled
        if (globals::misc::keybinds) {
            render_keybind_list();
        }

        // All features are now rendered through gui->render() above
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present with optional FPS override
        if (globals::misc::override_overlay_fps && globals::misc::overlay_fps > 0.0f) {
            static double accumulator = 0.0;
            static LARGE_INTEGER freq = { 0 };
            static LARGE_INTEGER last = { 0 };
            if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
            LARGE_INTEGER now; QueryPerformanceCounter(&now);
            double dt = (last.QuadPart == 0) ? 0.0 : double(now.QuadPart - last.QuadPart) / double(freq.QuadPart);
            last = now;
            accumulator += dt;
            double target_dt = 1.0 / globals::misc::overlay_fps;
            if (accumulator >= target_dt) {
                g_pSwapChain->Present(0, 0);
                accumulator -= target_dt;
            }
        }
        else {
            g_pSwapChain->Present(globals::misc::vsync ? 1 : 0, 0);
        }

        if (overlay::visible) {
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        }
        else
        {
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        }

        if (globals::misc::streamproof)
        {
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        }
        else
        {
            SetWindowDisplayAffinity(hwnd, WDA_NONE);
        }



        Notifications::Update();
        Notifications::Render();





        // Safe font reload just before rendering to avoid hot-reload glitches
        if (g_pendingFontReload) {
            ImGuiIO& io2 = ImGui::GetIO();
            io2.Fonts->Clear();
            ImFontConfig cfg; cfg.OversampleH = 1; cfg.OversampleV = 1; cfg.PixelSnapH = true;
            if (g_selectedFont == 0) {
                io2.Fonts->AddFontDefault();
            }
            else if ((size_t)g_selectedFont <= g_fontFiles.size()) {
                io2.Fonts->AddFontDefault();
                const std::string& selPath = g_fontFiles[(size_t)g_selectedFont - 1];
                io2.Fonts->AddFontFromFileTTF(selPath.c_str(), 13.0f, &cfg, io2.Fonts->GetGlyphRangesDefault());
            }
            io2.Fonts->Build();
            // Recreate device objects to ensure new font atlas is uploaded
            ImGui_ImplDX11_InvalidateDeviceObjects();
            ImGui_ImplDX11_CreateDeviceObjects();
            g_pendingFontReload = false;
        }


        // Removed overlay loaded popup
        // Damage indicators removed

      /*  static LARGE_INTEGER frequency;
        static LARGE_INTEGER lastTime;
        static bool timeInitialized = false;

        if (!timeInitialized) {
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&lastTime);
            timeBeginPeriod(1);
            timeInitialized = true;
        }

        const double targetFrameTime = 1.0 / 165;

        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        double elapsedSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

        if (elapsedSeconds < targetFrameTime) {
            DWORD sleepMilliseconds = static_cast<DWORD>((targetFrameTime - elapsedSeconds) * 1000.0);
            if (sleepMilliseconds > 0) {
                Sleep(sleepMilliseconds);
            }
        }

        do {
            QueryPerformanceCounter(&currentTime);
            elapsedSeconds = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
        } while (elapsedSeconds < targetFrameTime);

        lastTime = currentTime;*/

        if (overlay::visible) {
            // Overlay visibility logic here
        }
    }
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanup_avatar_system();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

bool fullsc(HWND windowHandle)
{
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    if (GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
    {
        RECT rect;
        if (GetWindowRect(windowHandle, &rect))
        {
            return rect.left == monitorInfo.rcMonitor.left
                && rect.right == monitorInfo.rcMonitor.right
                && rect.top == monitorInfo.rcMonitor.top
                && rect.bottom == monitorInfo.rcMonitor.bottom;
        }
    }
    return false;
}

void movewindow(HWND hw) {
    HWND target = FindWindowA(0, "Roblox");
    HWND foregroundWindow = GetForegroundWindow();

    if (target != foregroundWindow && hw != foregroundWindow)
    {
        MoveWindow(hw, 0, 0, 0, 0, true);
    }
    else
    {
        RECT rect;
        GetWindowRect(target, &rect);

        int rsize_x = rect.right - rect.left;
        int rsize_y = rect.bottom - rect.top;

        if (fullsc(target))
        {
            rsize_x += 16;
            rect.right -= 16;
        }
        else
        {
            rsize_y -= 39;
            rect.left += 8;
            rect.top += 31;
            rect.right -= 16;
        }
        rsize_x -= 16;
        MoveWindow(hw, rect.left, rect.top, rsize_x, rsize_y, TRUE);
    }
}
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();

    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}