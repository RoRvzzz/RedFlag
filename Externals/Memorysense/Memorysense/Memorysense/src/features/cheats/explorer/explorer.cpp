#include "explorer.h"
#include "ClassImages.h"
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../../../ext/imgui/imgui.h"
#include "../../../../src/main.h"

namespace memorysense::explorer {
    
    bool Explorer::first_time = true;
    void* Explorer::class_names_palette = nullptr;
    rbx::instance_t Explorer::selected_instance{};
    char Explorer::search_buffer[128] = "";
    int Explorer::right_tab = 0;
    
    // Lua dumping static variables
    std::unique_ptr<LuaDumper> Explorer::lua_dumper = nullptr;
    std::vector<LuaScript> Explorer::dumped_scripts;
    char Explorer::api_endpoint_buffer[256] = "http://api.plusgiant5.com";
    char Explorer::encryption_key_buffer[64] = "MemorySense2024";
    bool Explorer::compression_enabled = true;
    bool Explorer::auto_upload_enabled = false;
    
    // Decompiler window static variables
    bool Explorer::show_decompiler_window = false;
    std::string Explorer::decompiled_script = "";
    std::string Explorer::decompiled_script_name = "";
    
    void Explorer::render() {
        if (first_time) {
            initialize();
        }
        
        // Render decompiler window if open
        if (show_decompiler_window) {
            render_decompiler_window();
        }
        
        ImGui::PushFont(var->font.tahoma);

        gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
        gui->begin_group();
        {
            gui->push_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
            gui->text_field("Search by name or class...", search_buffer, IM_ARRAYSIZE(search_buffer));
            gui->pop_style_var();
        }
        gui->end_group();

        gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 30));
        gui->begin_def_child("ExplorerSplit", ImVec2(GetWindowWidth() - elements->content.window_padding.x * 2, GetContentRegionAvail().y - elements->content.window_padding.y * 2 - 30), 0, ImGuiWindowFlags_NoMove);
        {
            gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->content.spacing);

            float split_total_w = GetContentRegionAvail().x;
            float split_total_h = GetContentRegionAvail().y;
            float gap = GetStyle().ItemSpacing.x;
            float gap_v = GetStyle().ItemSpacing.y;
            float avail_no_gap = split_total_w - gap;
            float left_w = floorf(avail_no_gap * 0.5f);
            float right_w = floorf(avail_no_gap - left_w);
            
            // Calculate right side split heights
            float avail_no_gap_v = split_total_h - gap_v;
            float top_right_h = floorf(avail_no_gap_v * 0.5f);
            float bottom_right_h = floorf(avail_no_gap_v - top_right_h);

            // Left side - Explorer Tree (full height)
            gui->begin_child("Explorer Tree", 1, 1, ImVec2(left_w, split_total_h));
            {
                gui->push_style_var(ImGuiStyleVar_WindowPadding, elements->widgets.padding);
                gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->widgets.spacing);
                
                if (g_datamodel && g_datamodel->address != 0) {
                    bool has_filter = search_buffer[0] != '\0';
                    if (has_filter)
                        recursive_draw_filtered(*g_datamodel, "NULL");
                    else
                        recursive_draw(*g_datamodel, "NULL");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Datamodel not found!");
                }
                
                gui->pop_style_var(2);
            }
            gui->end_child();

            ImGui::SameLine(0.0f, gap);

            // Right side container
            ImGui::BeginGroup();
            {
                // Top right - Inspector
                gui->begin_child("Inspector", 1, 1, ImVec2(right_w, top_right_h));
                {
                    gui->push_style_var(ImGuiStyleVar_WindowPadding, elements->widgets.padding);
                    gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->widgets.spacing);

                    if (selected_instance.address != 0) {
                        std::string name = selected_instance.get_name();
                        std::string class_name = selected_instance.get_class_name();
                        
                        // Inspector content - always visible
                        ImGui::Text("Name: %s", name.c_str());
                        ImGui::Text("ClassName: %s", class_name.c_str());
                        ImGui::Text("Address: 0x%llX", static_cast<unsigned long long>(selected_instance.address));

                        if (_stricmp(class_name.c_str(), "Part") == 0 || _stricmp(class_name.c_str(), "MeshPart") == 0 || _stricmp(class_name.c_str(), "BasePart") == 0) {
                            ImGui::Spacing();
                            ImGui::Text("BasePart Properties:");
                            rbx::part_t part{}; part.address = selected_instance.address;
                            rbx::primitive_t prim = part.get_primitive();
                            auto pos = prim.get_position();
                            auto rot = prim.get_rotation();
                            auto siz = prim.get_size();
                            ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
                            ImGui::Text("Size: (%.2f, %.2f, %.2f)", siz.x, siz.y, siz.z);
                            ImGui::Text("Rotation:");
                            ImGui::Text("  [%.2f, %.2f, %.2f]", rot(0,0), rot(0,1), rot(0,2));
                            ImGui::Text("  [%.2f, %.2f, %.2f]", rot(1,0), rot(1,1), rot(1,2));
                            ImGui::Text("  [%.2f, %.2f, %.2f]", rot(2,0), rot(2,1), rot(2,2));
                        }
                        
                        ImGui::Spacing();
                        ImGui::Spacing();
                        if (ImGui::Button("Copy Address", ImVec2(120, 0))) {
                            char buf[32]; 
                            sprintf_s(buf, "0x%llX", static_cast<unsigned long long>(selected_instance.address)); 
                            ImGui::SetClipboardText(buf);
                        }
                        
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        // Lua Dumper button section
                        if (ImGui::Button("Decompile", ImVec2(120, 0))) {
                            // Check if selected instance is a script
                            if (_stricmp(class_name.c_str(), "LocalScript") == 0 || 
                                _stricmp(class_name.c_str(), "Script") == 0 || 
                                _stricmp(class_name.c_str(), "ModuleScript") == 0) {
                                
                                show_decompiler_window = true;
                                decompiled_script_name = name;
                                decompiled_script = "-- Decompiling " + name + "...\n-- This is a placeholder decompiled script\n\nlocal script = {}\n\nfunction script.init()\n    print('Script initialized')\nend\n\nreturn script";
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Dump Bytecode", ImVec2(120, 0))) {
                            // Dump bytecode logic
                        }
                        if (ImGui::Button("Copy All", ImVec2(120, 0))) {
                            // Copy all logic
                        }
                    } else {
                        ImGui::TextDisabled("Select an instance on the left.");
                    }
                    
                    gui->pop_style_var(2);
                }
                gui->end_child();
                
                ImGui::Dummy(ImVec2(0, gap_v));

                // Bottom right - Mess Around
                gui->begin_child("Mess Around", 1, 1, ImVec2(right_w, bottom_right_h));
                {
                    gui->push_style_var(ImGuiStyleVar_WindowPadding, elements->widgets.padding);
                    gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->widgets.spacing);
                    
                    if (selected_instance.address != 0) {
                        std::string name = selected_instance.get_name();
                        std::string class_name = selected_instance.get_class_name();
                        
                        // Mess Around - Value editing
                        std::string cname = class_name;
                        bool is_number_value = (_stricmp(cname.c_str(), "NumberValue") == 0) || (_stricmp(cname.c_str(), "IntValue") == 0);
                        bool is_bool_value = (_stricmp(cname.c_str(), "BoolValue") == 0);
                        
                        ImGui::Text("Mess Around - Value Editor");
                        ImGui::Separator();
                        ImGui::Spacing();
                        
                        if (!is_number_value && !is_bool_value) {
                            ImGui::Text("Select a NumberValue/IntValue/BoolValue to edit.");
                        } else {
                            ImGui::Text("Editing: %s [%s]", name.c_str(), class_name.c_str());
                            ImGui::Spacing();
                            
                            static int int_val = 0; 
                            static float float_val = 0.0f;
                            static bool bool_val = false;
                            
                            if (is_number_value) {
                                if (_stricmp(cname.c_str(), "IntValue") == 0) {
                                    ImGui::InputInt("Set Int Value", &int_val);
                                    ImGui::SameLine();
                                    if (ImGui::Button("Apply##int")) { 
                                        selected_instance.set_value<int>(int_val); 
                                    }
                                } else {
                                    ImGui::InputFloat("Set Float Value", &float_val);
                                    ImGui::SameLine();
                                    if (ImGui::Button("Apply##float")) { 
                                        selected_instance.set_value<float>(float_val); 
                                    }
                                }
                            }
                            
                            if (is_bool_value) {
                                ImGui::Checkbox("Set Bool Value", &bool_val);
                                ImGui::SameLine();
                                if (ImGui::Button("Apply##bool")) { 
                                    selected_instance.set_value<bool>(bool_val); 
                                }
                            }
                        }
                    } else {
                        ImGui::Text("Select an instance on the left.");
                    }
                    
                    gui->pop_style_var(2);
                }
                gui->end_child();
            }
            ImGui::EndGroup();

            gui->pop_style_var();
        }
        gui->end_def_child();

        ImGui::PopFont();
    }
    
    void Explorer::initialize() {
        first_time = false;
        lua_dumper = std::make_unique<LuaDumper>();
        lua_dumper->SetApiEndpoint(api_endpoint_buffer);
        lua_dumper->SetEncryptionKey(encryption_key_buffer);
        lua_dumper->SetCompressionEnabled(compression_enabled);
    }
    
    void Explorer::cleanup() {
        if (class_names_palette) {
            class_names_palette = nullptr;
        }
    }
    
    void Explorer::recursive_draw(rbx::instance_t instance, std::string parent_path) {
        if (instance.address == 0) return;
        
        std::string instance_path;
        std::string instance_name = instance.get_name();
        std::string instance_class = instance.get_class_name();
        
        if (parent_path == "NULL") {
            instance_path = "game:GetService(\"";
        } else {
            instance_path = (parent_path == "game:GetService(\"") ? 
                parent_path + instance_name + "\")" : 
                (!is_valid(instance_name)) ? 
                parent_path + "[\"" + instance_name + "\"]" : 
                parent_path + "." + instance_name;
        }
        
        auto children = instance.get_children<rbx::instance_t>();
        std::string unique_id = instance_name + " [" + instance_class + "]" + "##" + std::to_string(instance.address);
        
        std::string display_text = instance_name;
        if (!instance_class.empty() && _stricmp(instance_class.c_str(), "unknown") != 0) {
            display_text += " [" + instance_class + "]";
        }
        
        if (!children.empty()) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool open = ImGui::TreeNodeEx(unique_id.c_str(), flags, "%s", display_text.c_str());
            if (ImGui::IsItemClicked()) {
                selected_instance = instance;
            }
            if (open) {
                std::string popup_id = "Context Menu##" + std::to_string(instance.address);
                if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
                    ImGui::OpenPopup(popup_id.c_str());
                }
                
                if (ImGui::BeginPopup(popup_id.c_str())) {
                    if (ImGui::MenuItem("Copy Path")) {
                        ImGui::SetClipboardText(instance_path.c_str());
                    }
                    if (ImGui::MenuItem("Copy Address")) {
                        char buf[32];
                        sprintf_s(buf, "0x%llX", static_cast<unsigned long long>(instance.address));
                        ImGui::SetClipboardText(buf);
                    }
                    ImGui::EndPopup();
                }
                
                for (const auto& child : children) {
                    recursive_draw(child, instance_path);
                }
                ImGui::TreePop();
            }
        } else {
            ImGui::Selectable((display_text + "##" + std::to_string(instance.address)).c_str(), selected_instance.address == instance.address);
            if (ImGui::IsItemClicked()) {
                selected_instance = instance;
            }
            
            if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) {
                std::string popup_id = "Context Menu##" + std::to_string(instance.address);
                ImGui::OpenPopup(popup_id.c_str());
            }
            
            std::string popup_id = "Context Menu##" + std::to_string(instance.address);
            if (ImGui::BeginPopup(popup_id.c_str())) {
                if (ImGui::MenuItem("Copy Path")) {
                    ImGui::SetClipboardText(instance_path.c_str());
                }
                if (ImGui::MenuItem("Copy Address")) {
                    char buf[32];
                    sprintf_s(buf, "0x%llX", static_cast<unsigned long long>(instance.address));
                    ImGui::SetClipboardText(buf);
                }
                ImGui::EndPopup();
            }
        }
    }

    void Explorer::recursive_draw_filtered(rbx::instance_t instance, const std::string& parent_path) {
        if (instance.address == 0) return;
        std::string instance_name = instance.get_name();
        std::string instance_class = instance.get_class_name();
        std::string text = instance_name;
        if (!instance_class.empty() && _stricmp(instance_class.c_str(), "unknown") != 0) {
            text += " [" + instance_class + "]";
        }
        std::string lower = text; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        std::string query = search_buffer; std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        auto children = instance.get_children<rbx::instance_t>();
        bool match = query.empty() || lower.find(query) != std::string::npos;

        if (!children.empty()) {
            if (match) {
                if (ImGui::TreeNode((text + "##" + std::to_string(instance.address)).c_str())) {
                    for (const auto& child : children) recursive_draw_filtered(child, parent_path);
                    ImGui::TreePop();
                }
            } else {
                bool any_child_matches = false;
                for (const auto& child : children) {
                    std::string n = child.get_name();
                    std::string c = child.get_class_name();
                    std::string t = n + " [" + c + "]"; std::string l = t; std::transform(l.begin(), l.end(), l.begin(), ::tolower);
                    if (l.find(query) != std::string::npos) { any_child_matches = true; break; }
                }
                if (any_child_matches) {
                    if (ImGui::TreeNode((text + "##" + std::to_string(instance.address)).c_str())) {
                        for (const auto& child : children) recursive_draw_filtered(child, parent_path);
                        ImGui::TreePop();
                    }
                }
            }
        } else if (match) {
            ImGui::Selectable((text + "##" + std::to_string(instance.address)).c_str(), false);
        }
    }
    
    void Explorer::render_class_icon(const char* class_name, ImVec2 size) {
        ImGui::PushFont(var->font.icons[0]);
        
        static std::unordered_map<std::string, const char*> class_icon_map = {
            {"Default", "A"},
            {"Part", "B"},
            {"Model", "C"},
            {"Camera", "D"},
            {"Script", "E"},
            {"ImageFrame", "F"},
            {"Mesh", "G"},
            {"SpecialMesh", "G"},
            {"Humanoid", "H"},
            {"Texture", "I"},
            {"Sound", "J"},
            {"Players", "K"},
            {"SpotLight", "L"},
            {"SurfaceLight", "L"},
            {"RocketPropulsion", "M"},
            {"Tool", "N"},
            {"LocalScript", "O"},
            {"Workspace", "P"},
            {"Player", "Q"},
            {"Folder", "R"},
            {"Lighting", "S"},
            {"ReplicatedStorage", "T"},
            {"ServerStorage", "U"},
            {"StarterGui", "V"},
            {"StarterPack", "W"},
            {"StarterPlayer", "X"},
            {"SoundService", "Y"},
            {"TweenService", "Z"},
            {"RunService", "A"},
            {"UserInputService", "B"},
            {"ContextActionService", "C"},
            {"GuiService", "D"},
            {"Character", "E"},
            {"PlayerGui", "F"},
            {"PlayerScripts", "G"},
            {"Backpack", "H"}
        };
        
        const char* icon = "A";
        auto it = class_icon_map.find(class_name);
        if (it != class_icon_map.end()) {
            icon = it->second;
        }
        
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "%s", icon);
        ImGui::PopFont();
        ImGui::SameLine(0, 5);
    }
    
    bool Explorer::is_valid(const std::string& str) {
        if (str.empty()) return false;
        
        bool starts_with_number = std::isdigit(str[0]);
        bool contains_symbol = false;
        
        for (char ch : str) {
            if (!std::isalnum(ch) && ch != '_') {
                contains_symbol = true;
                break;
            }
        }
        
        return starts_with_number || contains_symbol;
    }

    void Explorer::render_lua_dumper_tab() {
        gui->begin_def_child("Lua Dumper", ImVec2(GetContentRegionAvail().x, GetContentRegionAvail().y), 0, ImGuiWindowFlags_NoMove);
        {
            // Settings section
            gui->begin_def_child("Settings", ImVec2(GetContentRegionAvail().x, 120), 0, ImGuiWindowFlags_NoMove);
            {
                draw->text_outline("Lua Script Dumper Settings");
                
                ImGui::InputText("API Endpoint", api_endpoint_buffer, sizeof(api_endpoint_buffer));
                ImGui::InputText("Encryption Key", encryption_key_buffer, sizeof(encryption_key_buffer));
                
                ImGui::Checkbox("Enable Compression", &compression_enabled);
                ImGui::SameLine();
                ImGui::Checkbox("Auto Upload", &auto_upload_enabled);
                
                if (gui->button("Update Settings", 3)) {
                    if (lua_dumper) {
                        lua_dumper->SetApiEndpoint(api_endpoint_buffer);
                        lua_dumper->SetEncryptionKey(encryption_key_buffer);
                        lua_dumper->SetCompressionEnabled(compression_enabled);
                    }
                }
            }
            gui->end_def_child();
            
            ImGui::Dummy(ImVec2(1, 5));
            
            // Current script section
            gui->begin_def_child("Current Script", ImVec2(GetContentRegionAvail().x, 100), 0, ImGuiWindowFlags_NoMove);
            {
                std::string name = selected_instance.get_name();
                std::string class_name = selected_instance.get_class_name();
                
                draw->text_outline((std::stringstream{} << "Selected: " << name << " [" << class_name << "]").str().c_str());
                draw->text_outline((std::stringstream{} << "Address: 0x" << std::hex << selected_instance.address).str().c_str());
                
                // Check if it's a Lua script
                bool is_lua_script = (_stricmp(class_name.c_str(), "Script") == 0 || 
                                    _stricmp(class_name.c_str(), "LocalScript") == 0 ||
                                    _stricmp(class_name.c_str(), "ModuleScript") == 0);
                
                if (is_lua_script) {
                    draw->text_outline("✓ This appears to be a Lua script");
                    if (gui->button("Dump Script", 3)) {
                        dump_current_script();
                    }
                } else {
                    draw->text_outline("⚠ This doesn't appear to be a Lua script");
                }
            }
            gui->end_def_child();
            
            ImGui::Dummy(ImVec2(1, 5));
            
            // Dumped scripts list
            render_script_list();
        }
        gui->end_def_child();
    }

    void Explorer::render_script_list() {
        gui->begin_def_child("Dumped Scripts", ImVec2(GetContentRegionAvail().x, GetContentRegionAvail().y), 0, ImGuiWindowFlags_NoMove);
        {
            draw->text_outline((std::stringstream{} << "Dumped Scripts (" << dumped_scripts.size() << ")").str().c_str());
            
            if (gui->button("Download All", 3)) {
                download_scripts();
            }
            ImGui::SameLine();
            if (gui->button("Clear List", 3)) {
                dumped_scripts.clear();
            }
            
            ImGui::Separator();
            
            if (dumped_scripts.empty()) {
                draw->text_outline("No scripts dumped yet. Select a script and click 'Dump Script'.");
            } else {
                for (size_t i = 0; i < dumped_scripts.size(); i++) {
                    const auto& script = dumped_scripts[i];
                    
                    std::string display_name = script.name + " [" + script.class_name + "]";
                    if (ImGui::Selectable(display_name.c_str(), false)) {
                        // Show script details
                    }
                    
                    ImGui::SameLine();
                    if (gui->button(("Upload##" + std::to_string(i)).c_str(), 2)) {
                        upload_script(script);
                    }
                }
            }
        }
        gui->end_def_child();
    }

    void Explorer::render_script_details(const LuaScript& script) {
        gui->begin_def_child("Script Details", ImVec2(GetContentRegionAvail().x, 200), 0, ImGuiWindowFlags_NoMove);
        {
            draw->text_outline((std::stringstream{} << "Name: " << script.name).str().c_str());
            draw->text_outline((std::stringstream{} << "Class: " << script.class_name).str().c_str());
            draw->text_outline((std::stringstream{} << "Address: 0x" << std::hex << script.address).str().c_str());
            draw->text_outline((std::stringstream{} << "Compiled: " << (script.is_compiled ? "Yes" : "No")).str().c_str());
            
            if (script.is_compiled) {
                draw->text_outline((std::stringstream{} << "Bytecode Size: " << script.bytecode.size() << " bytes").str().c_str());
            } else {
                draw->text_outline((std::stringstream{} << "Source Length: " << script.source.length() << " characters").str().c_str());
            }
            
            if (!script.error_message.empty()) {
                draw->text_outline((std::stringstream{} << "Error: " << script.error_message).str().c_str());
            }
        }
        gui->end_def_child();
    }

    void Explorer::dump_current_script() {
        if (!lua_dumper || selected_instance.address == 0) return;
        
        LuaScript script = lua_dumper->DumpScript(selected_instance.address);
        dumped_scripts.push_back(script);
        
        if (auto_upload_enabled) {
            upload_script(script);
        }
    }

    void Explorer::upload_script(const LuaScript& script) {
        if (!lua_dumper) return;
        
        bool success = lua_dumper->UploadScript(script, api_endpoint_buffer);
        if (success) {
            draw->text_outline("Script uploaded successfully!");
        } else {
            draw->text_outline("Failed to upload script!");
        }
    }

    void Explorer::download_scripts() {
        if (!lua_dumper) return;
        
        auto scripts = lua_dumper->DownloadScripts(api_endpoint_buffer);
        dumped_scripts.insert(dumped_scripts.end(), scripts.begin(), scripts.end());
    }
    
    void Explorer::render_decompiler_window() {
        // Get the main window position and size
        ImVec2 main_window_pos = ImGui::GetWindowPos();
        ImVec2 main_window_size = ImGui::GetWindowSize();
        
        // Calculate decompiler window position (right next to the explorer)
        ImVec2 decompiler_pos = ImVec2(main_window_pos.x + main_window_size.x + 10, main_window_pos.y);
        ImVec2 decompiler_size = ImVec2(600, main_window_size.y);
        
        ImGui::SetNextWindowPos(decompiler_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(decompiler_size, ImGuiCond_Always);
        
        ImGui::PushFont(var->font.tahoma);
        
        if (ImGui::Begin("Script Decompiler", &show_decompiler_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            // Titlebar
            ImGui::SetCursorPos(elements->content.window_padding);
            ImGui::PushFont(var->font.tahoma);
            ImGui::Text("Script Decompiler");
            ImGui::PopFont();
            
            ImGui::SetCursorPos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
            
            gui->push_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
            gui->begin_child("DecompilerContent", 1, 1, ImVec2(0, 0));
            {
                gui->push_style_var(ImGuiStyleVar_WindowPadding, elements->widgets.padding);
                gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->widgets.spacing);
                
                // Script name
                ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Script: %s", decompiled_script_name.c_str());
                ImGui::Separator();
                ImGui::Spacing();
                
                // Buttons
                if (ImGui::Button("Copy to Clipboard", ImVec2(150, 0))) {
                    ImGui::SetClipboardText(decompiled_script.c_str());
                }
                ImGui::SameLine();
                if (ImGui::Button("Save to File", ImVec2(150, 0))) {
                    // Save logic here
                }
                ImGui::SameLine();
                if (ImGui::Button("Close", ImVec2(80, 0))) {
                    show_decompiler_window = false;
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Decompiled script text area
                ImGui::BeginChild("ScriptContent", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
                {
                    ImGui::PushFont(var->font.tahoma);
                    ImGui::TextUnformatted(decompiled_script.c_str());
                    ImGui::PopFont();
                }
                ImGui::EndChild();
                
                gui->pop_style_var();
            }
            gui->end_child();
            gui->pop_style_var();
        }
        ImGui::End();
        
        ImGui::PopFont();
    }
    
}
