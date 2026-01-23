#pragma once
#include "../../../../ext/framework/settings/functions.h"
#include "../../../../src/sdk/sdk.h"
#include "lua_dumper.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace memorysense::explorer {
    class Explorer {
    public:
        static void render();
        static void initialize();
        static void cleanup();
        
    private:
        static bool first_time;
        static void* class_names_palette;
        static rbx::instance_t selected_instance;
        static char search_buffer[128];
        static int right_tab; // 0: Properties, 1: Value Editor, 2: Lua Dumper
        
        // Lua dumping
        static std::unique_ptr<LuaDumper> lua_dumper;
        static std::vector<LuaScript> dumped_scripts;
        static char api_endpoint_buffer[256];
        static char encryption_key_buffer[64];
        static bool compression_enabled;
        static bool auto_upload_enabled;
        
        // Decompiler window
        static bool show_decompiler_window;
        static std::string decompiled_script;
        static std::string decompiled_script_name;

        static void recursive_draw(rbx::instance_t instance, std::string parent_path);
        static void recursive_draw_filtered(rbx::instance_t instance, const std::string& parent_path);
        static void render_class_icon(const char* class_name, ImVec2 size = ImVec2(16, 16));
        static bool is_valid(const std::string& str);
        
        // Lua dumping UI
        static void render_lua_dumper_tab();
        static void render_script_list();
        static void render_script_details(const LuaScript& script);
        static void dump_current_script();
        static void upload_script(const LuaScript& script);
        static void download_scripts();
        
        // Decompiler window
        static void render_decompiler_window();
    };
}
