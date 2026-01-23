#pragma once


#include <set>

#ifndef GLOBALS_H
#define GLOBALS_H

extern float g_Time;
extern float g_DeltaTime;

#endif // GLOBALS_H
#include <vector>
#include <cstdint>
#include <algorithm>
#include <iterator>

#include "../rbx/main.h"
#include "../rbx/overlay/ckey/keybind.hpp"
 inline float test;
 inline float test2;
 inline float test3;
struct c_part {
    uint64_t self; // Address or unique identifier for the part
    // Additional fields can be added as needed
};

// Class for managing custom ESP
class custom_esp_t {
public:
    std::vector<RBX::Instance> parts; // Publicly accessible parts vector

    // Iterator for beginning of parts
    auto start() {
        return parts.begin();
    }

    // Iterator for end of parts
    auto end() {
        return parts.end();
    }

    // Const iterators for read-only access
    auto start() const {
        return parts.cbegin();
    }

    auto end() const {
        return parts.cend();
    }
};




struct globals
{
    static int crosshair_origin;
    static float chamsGlowSize;
    static bool snapline;
    static bool chamsglow;
    static float crosshair_spinspeed;
    static bool pulse_effect;
    static RBX::Vector3 waypoint;
    static bool waypoint_enabled;
    static bool crosshair_outline;
    static float arrowDistance;
    static float tracer_thickness;
    static float arrowSize;
    static float hit_chance;
    static float deadzoneX;
    static  float deadzoneY;
    static bool usedeadzone;
    static int image_type;
    static bool Hit_Notifications;
    static bool knife_CHeck;
    static bool flag_esp;
    static bool TeamVisual;
    static bool HealthBasedColor;
    static bool boxGlow;
    static bool jumppowertoggle;
    static bool forcekick;
    static bool allow_tearing;
    static bool enable_health_glow;
    static int fps_cap;
    static float health_y;
    static int fov_segmants;
    static float health_x;
    static float chams_opacity;
    static bool autotype;
    static bool speed_enabled;
    static bool rapidfire;
    static bool autoreload;
	static bool antistomp;
    static float crosshair_color[3];
    static float flagcolor[3];
    static int menu_glow;
    static bool non_zero_check;
    static bool tracer_esp;
    static bool HeadDotEsp;
  
    static int HeadDotMode; // 0 = off, 1 = filled, 2 = outline, 3 = filled+outline
    static float HeadDotSize ;
    static float HeadDotOffset;
    static int HeadDotSegments;
    static float TracerColor[3];
 
    static bool linkTarget;
    static float skeletonThickness;
    static bool health_bar_text;
    static bool silentaim;
    static int glow_opacity;
    static float glow_size;
    static bool HeavyOptimize;
    static bool skeletonOutline;
    static ImVec4 ImGuiColors[ImGuiCol_COUNT];
    static   int crosshair_gap;
    static   int crosshair_size;
    static bool crosshair;
    static bool fill_box_render;
    static bool menufinishedfading;
    static float alpha;
    static  int crosshair_speed;
    static bool OptimizeCache;
    static  int selected_player;
    static CKeybind fly_bind;
    static CKeybind triggerBind;
    static CKeybind noclipbind;
    static int flightmode;
    static CKeybind EspKeybind;
    static CKeybind CframeBind;
    static int* Menu_bind;
    custom_esp_t custom_esp;
    static int fonttype;
    static float BoxRounding;
    static float BoxThickness  ;
    static float BoxFadeDistance ;

    static  int HealthBarSide  ; // 0 = Left, 1 = Right, 2 = Top, 3 = Bottom

    static  bool FadeThroughWall  ;
    static  float WallAlpha  ;

    static   bool ScaleByDistance  ;
    static  bool DrawClosestOnly  ;

    static  int TracerOrigin ; // 0 = Screen Center, 1 = Bottom Center, 2 = Mouse

    static  float GlowIntensity ;
    static bool tool_outline;
    static bool fasttools;
    //   static bool HeavyOptimize;
    static bool distancecheck;
    static bool offscreen_Check;
    static bool spectate_target;
    static bool onlylocal;
    static CKeybind aimbot_bind;
    static CKeybind free_aim_bind;
    static CKeybind silent_Aim_Bind;
    static CKeybind WalkSpeed_Bind;
    static CKeybind JumpPower_Bind;
    static CKeybind HipHeight_Bind;
    static bool triggerbot_enabled;
    static bool triggerbot_team_check;
    static float triggerbot_fov; 

    static std::string localplayername;
    static bool autoparry;
    static bool skeleton_esp;
    static RBX::Instance  game;
    static RBX::Instance  DataModelPTR;
    static std::string team;
    static RBX::WorkSpace  game1;
    static RBX::Instance visualengine;
    static RBX::Instance players;
    static RBX::Instance workspace;
    static RBX::Instance lighting;
    static RBX::Instance camera;
    static RBX::PlayerInstance target;
    static RBX::Instance aim;
    static bool onlytarget;
    static bool chams;
    static bool FrameChams;
    static bool WireFrameChams;
    static bool PlayerGlow;
    static bool FrameOutline;
    static std::vector<RBX::WorkSpaceInstance>  cachedInstances;
    static float camera_fov;
    static std::uint64_t mouse_service;
    static bool fill_box;
    static RBX::PlayerInstance localplayer;
    static std::vector<RBX::PlayerInstance> cached_players;
    static bool flame_bar;
    // OTHER STUFF
    static int triggerbot_delay;
    static bool fill_chams;
    static bool threadcrash;
    static bool high_cpu_usage;
    static int mostFreq;
    static int circleSegs;
    static int verticalSegs;
    static bool teambasedcolor;
    static HWND window_handle;
    static bool localplayercheck;
    // AIMBOT
    static bool tool_esp;
    static bool noclip;
    static bool aimbot;
    static bool rescan;
    static bool draw_aimbot_fov;
    static float aimbot_fov_size;
    static bool fly;
    static float max_aimbot_distance;
    static int threadtime;
    static int threadrestarttime;
    static int aimbot_easing_style;

    static int aimbot_part;
    static int aimbot_type;
    static int aimbot_mode;
    static bool aimbot_sticky;
    static bool closest_part;

    static bool camera_prediction;
    static float camera_prediction_x;
    static float camera_prediction_y;
    static float camera_prediction_z;

    static float mouse_sensitivity;
    static float mouse_smoothness;

    static float smoothness_camera;

    static bool resolver;
    static std::vector<bool> aimbot_checks;

    static bool shake;
    static float shake_x;
    static float shake_y;

    static float shake_time_interval;
    static float shake_amount;
    static float shake_smoothness;
    static int cframespeed;
    static bool vehicle_esp;
    static bool cash_esp;
    static bool cashier_esp;
    static bool soccer_ball_esp;
    static bool dropped_items;
    // FREE AIM

    static bool free_aim;
    static bool free_aim_sticky;
    static bool free_aim_is_in_fov;
    static bool free_aim_draw_fov;
    static std::vector<bool> silent_aim_checks;
    static bool jail_esp;
    static bool free_aim_resolver;

    static float max_free_aim_distance;
    static float free_aim_fov;


    static int free_aim_part;
    static bool free_aim_closest_part;
    static bool cframe;
    static float  BoxOutlineColor[3];
    static bool free_aim_prediction;
    static float free_aim_prediction_x;
    static float free_aim_prediction_y;
    static bool trigger_bot_prediction;
    static float trigger_bot_prediction_x;
    static float trigger_bot_prediction_y;
    // TRIGGERBOT

    static bool triggerbot;
    static bool triggerbot_sticky;
    static bool draw_triggerbot_fov;
    static std::vector<bool> triggerbot_checks;

    static float max_triggerbot_distance;
    static float triggerbot_radius;

  
    static CKeybind keybind_set;
    static CKeybind keybind_teleport;
    static CKeybind keybind_clear;
    static bool tp_on_respawn;

    // ESP

    static bool esp;

    static float max_render_distance;

    static bool box_esp;
    static int box_type;
    static float healthBar_Color[3];
    static float skeleton_Color[3];
    static float color_3[3];
    static float box_color[3];
    static float color_2[3];

    static float color_1[3];
    static float color_5[3];
    static float color_4[3];
    static float HighLight_color[3];
    static bool health_bar;
    static bool highlight;
    static bool shield_bar;

    static bool name_esp;
    static bool name_outline;
    static int name_type;
    static float name_color[3];

    static bool distance_esp;
    static bool distance_outline;
    static float distance_color[3];
    static float teamNameColor[3];
    static float FovColor[3];
    static std::vector<bool> esp_checks;

    // MOVEMENTS

    static bool nojumpcooldown;

    static float walkspeed_amount;
    static float JumpPower;
    static float HipHeight;
    // GAME SETTINGS
    static int fly_speed;
    static int_fast64_t game_id;

    // OVERLAY SETTINGS

    static bool force_projection_update;
    static bool highcpuusageesp;
    static bool vsync;
    static bool streamproof;

    static int font_type;

    // CHEAT SETTINGS
    static bool chams_outline;
    static bool debug_info;
    static bool use_class_names;
};