#pragma once
#include "../../util/globals.h"
#include "../../util/classes/classes.h"
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <utility>
#include <algorithm>
#include <unordered_map>

// Trail point data structure
struct TrailPoint {
    Vector3 position;
    std::chrono::steady_clock::time_point timestamp;
    
    TrailPoint(const Vector3& pos) : position(pos), timestamp(std::chrono::steady_clock::now()) {}
};

// Enhanced player data cache with LOD integration
struct CachedPlayerData {
    roblox::player player;
    Vector3 position;
    Vector2 top_left;
    Vector2 bottom_right;
    float distance;
    std::string name;
    std::string tool_name;
    float margin;
    float health;
    bool valid;
    // PF TextLabel.TextColor3 if available
    bool hasTextColor = false;
    int textColorR = 0;
    int textColorG = 0;
    int textColorB = 0;
    // Trail data
    std::vector<TrailPoint> trail_points;
};



// Main ESP data container with thread safety
struct ESPData {
    std::vector<CachedPlayerData> cachedPlayers;

    Vector3 localPos;
    std::mutex cachedmutex;

    // Performance metrics (optional)
    std::chrono::high_resolution_clock::time_point lastUpdate;
    size_t frameCount = 0;
};

namespace visuals {
    extern ESPData espData;
    extern std::thread updateThread;
    extern bool threadRunning;
    // Core functions
    void run();
    void updateESP();
    void startThread();
    void stopThread();

    // Text rendering with FreeType support
    void draw_text_with_shadow_enhanced(float font_size, const ImVec2& position, ImColor color, const char* text);
    
    // Crosshair
    void renderCrosshair();
}

// Forward declarations for DirectX
struct ID3D11Device;
struct ID3D11DeviceContext;


namespace utils {
    bool boxes(roblox::player player);
    void CacheChamsPoly(); // Cache chams polygons for overlay rendering
}

// Chams polygon structure for overlay rendering
struct Polygon_t {
    std::vector<ImVec2> vertices;
    float depth;
};

void DrawChinaHat(ImDrawList* draw, const Vector3& head_pos, ImColor color);

namespace esp_helpers {
    bool on_screen(const Vector2& pos);
}

// Chams structures
struct c_mesh_draw_primitive {
    void* vtable;
    void* m_material;
    void* m_material2;
    ImColor m_tint_color;
    
    template<typename T>
    T* get_scene_object() {
        // This would need to be implemented based on your engine structure
        // Typically this would read from an offset in the mesh structure
        return nullptr;
    }
};

struct c_mesh_primitive_output_buffer {
    c_mesh_draw_primitive* m_out;
    int m_max_output_primitives;
    int m_start_primitive;
};

struct c_scene_object {
    uintptr_t address;
};

struct c_scene_animatable_object : public c_scene_object {
    // Inherits from c_scene_object
    // Additional animatable-specific members would go here
};

// Chams class
namespace chams {
    class chams {
    public:
        static chams& instance() {
            static chams inst;
            return inst;
        }
        
        bool render(void* __this, c_scene_animatable_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf);
        void override_material(c_mesh_draw_primitive* mesh, void* material, const ImColor& color);
        
    private:
        bool is_valid_player(c_scene_animatable_object* object);
    };
    
    inline chams& get_chams() {
        return chams::instance();
    }
}

// Hook function declaration - integrate this with your hook system
// Example usage:
// void __fastcall hooked_animatable_generateprimitives(void* __this, c_scene_object* object, void* a3, c_mesh_primitive_output_buffer* render_buf);
    
// Performance configuration
namespace perf_config {
    constexpr auto UPDATE_INTERVAL_HIGH = std::chrono::microseconds(2500);   // High detail updates
    constexpr auto UPDATE_INTERVAL_MEDIUM = std::chrono::microseconds(5000); // Medium detail updates  
    constexpr auto UPDATE_INTERVAL_LOW = std::chrono::microseconds(10000);   // Low detail updates
    constexpr float MAX_RENDER_DISTANCE = 500.0f;
    constexpr float MAX_OFFSCREEN_ARROW_DISTANCE = 300.0f;
    constexpr size_t MAX_CACHED_PLAYERS = 64; // Limit for performance
}