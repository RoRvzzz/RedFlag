#pragma once
#include "../util/classes/classes.h"
#include "../drawing/overlay/keybind/keybind.h"
#include <unordered_map>
namespace globals
{
	inline bool focused;
	inline bool firstreceived = false;
	inline bool unattach = false;
	inline bool handlingtp = false;
	inline std::vector<roblox::wall> walls;
	namespace bools {
		inline bool autokill, kill;
		inline std::string name;
		inline roblox::player entity;

	}

	// Render-loop guard to avoid memory reads while drawing
	inline bool in_render_loop = false;

	// Cached teammate decisions to avoid reads during rendering
	inline std::unordered_map<uint64_t, bool> teammate_cache;
	// Debounce state: require consecutive identical verdicts before committing
	inline std::unordered_map<uint64_t, bool> teammate_last_verdict;
	inline std::unordered_map<uint64_t, int> teammate_verdict_streak;
	namespace instances {
		inline std::vector<std::string> whitelist;
		inline std::vector<std::string> blacklist;
		inline roblox::instance visualengine;
		inline roblox::instance datamodel;
		inline roblox::instance workspace;
		inline roblox::instance players;
		inline roblox::player lp;
		inline roblox::instance lighting;
		inline roblox::camera camera;
		inline roblox::instance localplayer;
		inline std::string gamename = "Universal";
        inline std::string username = "Aimquette";
		inline std::vector<roblox::player> cachedplayers;
		inline roblox::player cachedtarget;
		inline roblox::player cachedlasttarget;
		inline roblox::instance aim;
		inline uintptr_t mouseservice;
		inline ImDrawList* draw;
		// esp_font removed
	}
	namespace combat {
		inline bool rapidfire;
		inline bool autostomp;
		inline bool animation_enable = false;
		inline int animationtype = 0;
		inline int run_animation = 0; // Default to Default
		inline int walk_animation = 0;
		inline int jump_animation = 0;
		inline int fall_animation = 0;
		inline int idle_animation = 0;
		inline int climb_animation = 0;
		inline int swim_animation = 0;
				inline bool aimbot = false;
		inline bool stickyaim = true;
		inline bool unlockondeath = false; // camlock: drop lock when target dies
		inline int aimbottype = 0;
		inline keybind aimbotkeybind("aimbotkeybind");
				inline bool usefov = false;
		inline bool drawfov = false;
		inline float fovsize = 50;
		inline float fovcolor[4] = {1,1,1,1};
				inline bool smoothing = false;
		inline float smoothingx = 5;
		inline float smoothingy = 5;
		inline int smoothingstyle = 1; // 0=None(raw), 1=Linear, 2..10 easing styles
		inline bool custom_easing = false;
		// Custom easing control points (normalized 0-1, default balanced curve)
		inline float easing_point1_x = 0.33f;  // First control point X (more neutral position)
		inline float easing_point1_y = 0.33f;  // First control point Y
		inline float easing_point2_x = 0.67f;  // Second control point X (more neutral position)
		inline float easing_point2_y = 0.67f;  // Second control point Y
		// Sensitivity controls
		inline bool sensitivity_enabled = true; // master toggle for sensitivity
		inline float cam_sensitivity = 1.0f; // camlock sensitivity multiplier
		inline bool camlock_shake = false; // enable shake for camlock
		inline float camlock_shake_x = 2.0f; // X-axis shake intensity
		inline float camlock_shake_y = 2.0f; // Y-axis shake intensity
		inline float camlock_shake_z = 2.0f; // Z-axis shake intensity
		inline float mouse_sensitivity = 1.0f; // mouselock sensitivity multiplier
				inline bool predictions = false;
		inline float predictionsx = 5;
		inline float predictionsy = 5;
		inline bool deadzone = false;
		inline float deadzonex = 0.0f;
		inline float deadzoney = 0.0f;
				inline bool autoswitch = false;
		inline bool teamcheck = false;
		inline bool grabbedcheck = false;
		inline bool knockcheck = false;
		inline bool rangecheck = false;
		inline bool healthcheck = false;
		inline bool wallcheck = false;
		inline bool knifecheck = false;
		inline bool arsenal_flick_fix = false;
		inline std::vector<int>* flags = new std::vector<int>{ 0, 0, 0, 0, 0,0,0};
		inline float range = 1000;
		inline float healththreshhold = 10;
		inline bool crew_check = false; // Skip players in same roblox group/crew
						inline int aimpart = 0; // 0 = Head, 1 = Upper Torso, 2 = Lower Torso, 3 = HumanoidRootPart, 4 = Left Hand, 5 = Right Hand, 6 = Left Foot, 7 = Right Foot, 8 = Closest Part, 9 = Random Part, 10 = Closest Point
		inline std::vector<int>* ignore_parts_camera = new std::vector<int>(8, 0); // Ignore parts for camera aimbot: Head, Upper Torso, Lower Torso, HRP, Left Hand, Right Hand, Left Foot, Right Foot
		inline std::vector<int>* ignore_parts_mouse = new std::vector<int>(8, 0); // Ignore parts for mouse aimbot: Head, Upper Torso, Lower Torso, HRP, Left Hand, Right Hand, Left Foot, Right Foot
		inline bool silentaim = false;
		inline bool stickyaimsilent = true;
		inline bool connect_to_aimbot = false; // Connect silent aim to aimbot target
		inline bool aimbot_locked = false; // Track if aimbot is currently locked onto a target
		inline roblox::player aimbot_current_target; // Current target that aimbot is locked onto
		inline int hitchance = 100;
		inline keybind silentaimkeybind("silentaimkeybind");
		inline int closestpartsilent = 0; // 0 = Off, 1 = Closest Part, 2 = Closest Point
		inline int silentaimpart = 0; // 0 = Head, 1 = Upper Torso, 2 = Lower Torso, 3 = HumanoidRootPart, 4 = Left Hand, 5 = Right Hand, 6 = Left Foot, 7 = Right Foot, 8 = Closest Part, 9 = Random Part
		inline int mouselockpart = 0; // 0 = Head, 1 = Upper Torso, 2 = Lower Torso, 3 = HumanoidRootPart, 4 = Left Hand, 5 = Right Hand, 6 = Left Foot, 7 = Right Foot, 8 = Closest Part, 9 = Random Part
		inline bool nosleep_aimbot = false; // No sleep aimbot for memory and camera types only
		inline int camlockpart = 0; // 0 = Head, 1 = Upper Torso, 2 = Lower Torso, 3 = HumanoidRootPart, 4 = Left Hand, 5 = Right Hand, 6 = Left Foot, 7 = Right Foot, 8 = Closest Part, 9 = Random Part
		inline bool airpart_enabled = false; // Enable airpart feature
		inline int airpart = 0; // 0 = Head, 1 = Upper Torso, 2 = Lower Torso, 3 = HumanoidRootPart, 4 = Left Hand, 5 = Right Hand, 6 = Left Foot, 7 = Right Foot, 8 = Closest Part, 9 = Random Part
		inline int target_method = 0; // 0 = Closest To Mouse, 1 = Closest To Camera
		inline bool usesfov = false;
		inline bool drawsfov = false;
		inline float sfovsize = 50;
		inline float sfovcolor[4] = { 1,1,1,1 };
		inline bool silentpredictions = false;
		inline float silentpredictionsx = 5;
		inline float silentpredictionsy = 5;
		inline bool spoofmouse = true;
				inline bool orbit = false;
				inline std::vector<int> orbittype = std::vector<int>(3, 0);
		inline float orbitspeed = 8;
		inline float orbitrange = 3;
		inline float orbitheight = 1;
		inline bool drawradiusring = false;
		inline keybind orbitkeybind("orbitkeybind                 ");
	//	inline std::vector<int>* orbittype;
                
				inline bool triggerbot = false;
				inline bool antiaim = false;
				inline bool underground_antiaim = false;
		inline float delay = 0;
		inline float releasedelay = 0;
		inline bool triggerbotrange = false;
		inline float triggerbotrangevalue = 50;
		inline keybind triggerbotkeybind("triggerbotkeybind");
		


	}
	namespace visuals {
				inline bool visuals = true;
		inline bool boxes = false;
		inline bool lockedindicator = false;

		inline bool glowesp = false;
		inline int boxtype = 0;
		inline bool health = false;
		inline bool healthbar = false;
		inline bool armorbar = false;
		inline bool healthtext = false;
		inline bool name = false;
		inline int nametype = 0;
		inline bool toolesp = false;
		inline bool distance = false;
		inline bool flags = false;
		inline bool skeletons = false;

		inline bool selfesp = false;
		inline bool maxdistance_enabled = false;
		inline float maxdistance = 1000.0f;
		inline bool chinahat = false;
		inline bool chinahat_target_only = false;
		inline float chinahat_color[4] = {1.0f, 0.843f, 0.0f, 1.0f};
		inline bool localplayer = false;
		

		
		inline bool predictionsdot = false;

		// Radar
		inline bool radar = false;
		inline float radar_size = 160.0f; // square side in pixels
		inline float radar_range = 250.0f; // meters
		inline float radar_pos[2] = { 20.0f, 20.0f }; // top-left corner
		inline bool radar_rotate = true;
		inline bool radar_show_distance = true;
		inline bool radar_outline = true;
		inline float radar_background[4] = { 0.06f, 0.06f, 0.06f, 0.6f };
		inline float radar_border[4] = { 0.0f, 0.0f, 0.0f, 0.8f };
		inline float radar_dot[4] = { 1.0f, 0.3f, 0.3f, 1.0f };

        inline float boxcolors[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
		        inline float boxfillcolor[4] = { 0.2,0.2,0.2,0.3 };
        inline float box_gradient_color1[4] = { 0.8f, 0.2f, 0.8f, 0.5f }; // First gradient color
        inline float box_gradient_color2[4] = { 0.2f, 0.8f, 0.8f, 0.5f }; // Second gradient color
        inline float glowcolor[4] =  { 0.396f, 0.420f, 0.722f, 1.0f};
        inline bool box_gradient = false;
        inline bool box_gradient_rotation = false; // Rotating gradient direction
        inline float box_gradient_rotation_speed = 0.25f; // revolutions per second
		inline float lockedcolor[4] =  { 0.396f, 0.420f, 0.722f, 1.0f };
		inline float healthbarcolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
		inline float healthbarcolor1[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
        inline float namecolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
        inline float toolespcolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
                inline float distancecolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
        inline float sonarcolor[4] = { 1.0f, 1.0f, 1.0f, 0.8f };
        inline float sonar_dot_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

        inline float skeletonscolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
        inline bool sonar = false;
        inline float sonar_range = 50.0f;
        inline float sonar_thickness = 1.0f;
        inline bool sonar_show_distance = false;
        
        // Team display
        inline bool teams = false;
        inline float teamscolor[3] = { 0.3647f, 0.4471f, 0.4549f };
        inline std::vector<int>* team_overlay_flags = new std::vector<int>{ 0, 0 };
        
        // Fog controls
        inline bool fog = false;
        inline float fog_start = 0.0f;
        inline float fog_end = 1000.0f;
        inline float fog_color[4] = { 0.396f, 0.420f, 0.722f, 1.0f }; // Default gray fog color
        inline bool rainbow_fog = false; // Rainbow fog animation
        inline float rainbow_fog_speed = 1.0f; // Speed of rainbow animation
        inline bool fog_glow = false; // Fog glow effect
        inline float fog_glow_intensity = 1.0f; // Fog glow intensity
        inline float fog_glow_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Fog glow color
        
        // Lighting modifications
        inline bool lighting_modifications = false;
        inline float lighting_brightness = 1.0f; // 0.0 to 10.0
        inline float lighting_contrast = 1.0f; // 0.0 to 2.0
        inline float lighting_color_correction[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // Color correction tint
        inline bool lighting_shadows = true;
        inline float lighting_ambient[3] = { 0.5f, 0.5f, 0.5f }; // Ambient lighting color
        
        // Workspace features
        inline bool workspace_viewer = false;
        inline bool workspace_show_position = false;
        inline bool workspace_show_size = false;
        inline bool workspace_show_velocity = false;
        inline bool workspace_transparency_modifier = false;
        inline float workspace_transparency = 0.0f; // 0.0 to 1.0
        inline bool workspace_cancollide_modifier = false;
        inline bool workspace_cancollide_value = true;
        inline bool workspace_anchored_modifier = false;
        inline bool workspace_anchored_value = false;
    


				inline bool fortniteindicator = false;
		inline bool hittracer = false;
		inline bool trail = false;
		inline float trail_duration = 3.0f; // Trail duration in seconds
		inline float trail_color[4] = { 0.396f, 0.420f, 0.722f, 0.8f }; // Trail color (RGBA)
		inline float trail_thickness = 2.0f; // Trail line thickness
		inline bool hitbubble = false;
		inline bool targetskeleton = false;


		// Tracers
		inline bool tracers = false;
		inline int tracerstype = 0; // 0 Off, 1 Normal, 2 Spiderweb
		inline bool tracers_outline = false;
		inline bool tracers_glow = false;
		inline float tracerscolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
		

		inline float flagscolor[4] = { 0.396f, 0.420f, 0.722f, 1.0f };
		inline float flags_font_size = 11.0f;
				inline float flags_offset_x = 5.0f;

		// ESP distance limit removed
		inline bool enemycheck = true;
		inline bool friendlycheck = true;
		inline bool teamcheck = false;
		inline bool rangecheck = false;
		inline float range = 1000;

		
		// Crosshair
		inline bool crosshair_enabled = false;
		inline float crosshair_color[4] = {0.396f, 0.420f, 0.722f, 1.0f};
		inline float crosshair_size = 10.0f;
		inline float crosshair_gap = 5.0f;
		inline bool crosshair_gapTween = false;
		inline float crosshair_gapSpeed = 1.0f;
		inline float crosshair_thickness = 1.0f;
		inline int crosshair_styleIdx = 0;
		inline float crosshair_baseSpeed = 100.0f;
		inline float crosshair_fadeDuration = 1.0f;
			inline std::vector<int>* box_overlay_flags = new std::vector<int>{ 0, 0, 0 };
		inline std::vector<int>* name_overlay_flags = new std::vector<int>{ 0, 0 };
		inline std::vector<int>* flag_overlay_flags = new std::vector<int>{ 0, 0 };
		inline std::vector<int>* healthbar_overlay_flags = new std::vector<int>{ 0, 0, 0 };
		inline std::vector<int>* tool_overlay_flags = new std::vector<int>{ 0, 0 };
		inline std::vector<int>* distance_overlay_flags = new std::vector<int>{ 0, 0 };
		inline std::vector<int>* skeleton_overlay_flags = new std::vector<int>{ 0, 0 };
		
		// Chams
		inline bool chams = false;
		inline bool chams_ignorez = false;
		inline float chamscolor[4] = { 0.396f, 0.420f, 0.722f, 0.5f };
		inline float chams_ignorez_color[4] = { 1.0f, 0.0f, 0.0f, 0.3f };
		inline std::vector<int>* chams_overlay_flags = new std::vector<int>{ 0, 0, 0 };

		
		// Sensor
		




	}
	namespace misc {
				inline bool speed = false;
		inline int speedtype = 0;
		inline float speedvalue = 16;
		inline keybind speedkeybind("speedkeybind");
		
		// NPC Check feature (for Workspace.Bots)
		inline bool npc_check = false;
		// 360 Spin bind
		inline bool spin360 = false;
		// UI scale 1..10; mapped internally to a high deg/s for fast spins
		inline float spin360speed = 5.0f; // UI scale (1..10)
		inline keybind spin360keybind("spin360keybind");
				inline bool jumppower = false;
		inline float jumpowervalue = 50;
		inline keybind jumppowerkeybind("jumppowerkeybind");
                inline bool voidhide = false; // hidden from UI
        inline keybind voidhidebind("voidehidkeybind"); // kept for compat
		inline bool headless = false;
		inline bool flight = false;
		inline int flighttype = 0;
		inline float flightvalue = 16;
		inline keybind flightkeybind("flightkeybind");
		inline bool hipheight = false;
		inline  float hipheightvalue = 16;
		inline  bool rapidfire = false;
		inline bool autoarmor = false;
		inline bool autoreload = false;
                inline bool autostomp = false; // hidden from UI
		inline bool antistomp = false;
		inline bool bikefly = false;
		inline keybind stompkeybind("stompkeybind");

		// Animation system
		inline bool animation = false;
		inline int animationtype = 0; // 0=zombie, 1=ninja, 2=robot, 3=zombie2, 4=levitation, 5=stylish, 6=cartoony, 7=superhero, 8=elder, 9=toy, 10=oldschool
		inline bool animations_stored = false;
		inline std::unordered_map<std::string, std::string> original_animations; // Store original animation IDs
		
		// Animation system 2
		inline bool animation2 = false;
		inline int animationtype2 = 0; // 0=zombie, 1=ninja, 2=robot, 3=zombie2, 4=levitation, 5=stylish, 6=cartoony, 7=superhero, 8=elder, 9=toy, 10=oldschool
		inline bool animations_stored2 = false;
		inline std::unordered_map<std::string, std::string> original_animations2; // Store original animation IDs for second changer
		
		// Enhanced movement
		inline bool cframe = false;
		inline int cframespeed = 16;
		inline keybind cframekeybind("cframekeybind");
		
		// Anti-aim
		inline bool antiaim = false;

		// Desync functionality
		inline bool desync = false;
		inline keybind desynckeybind("desynckeybind");
		inline bool desync_active = false;
		inline float desync_timer = 0.0f;
		inline const float desync_max_time = 15.0f; // Fixed at 15 seconds
		
		// Desync visualizer
		inline bool desync_visualizer = false;
		inline float desync_viz_color[4] = { 1.0f, 0.0f, 0.0f, 0.8f }; // Red with transparency
		inline const float desync_viz_range = 3.0f; // Fixed range
		inline const float desync_viz_thickness = 2.0f; // Fixed thickness
		inline Vector3 desync_activation_pos = Vector3(0, 0, 0); // Position where desync was activated
		
		// Spam TP to target
		inline bool spam_tp = false;
		inline std::string roblox_path = "";
		inline std::string firewall_rule_name = "Calendar";

        		inline bool spectate = false; // hidden from UI
		inline keybind spectatebind("spectatebind"); // kept for compat

                inline bool vsync = true;
                inline int max_fps = 175;
            inline int keybindsstyle;
        inline bool targethud = false; // Reverted to user-toggleable
        inline bool playerlist = true; // Always on
        inline bool streamproof = false;
        inline bool allow_raycasting = false;
        inline bool keybinds = false;
        inline bool explorer = false;
        inline bool spotify = false;
        inline bool override_overlay_fps = false;
        inline float overlay_fps = 60.0f;
        inline bool colors = false;
        inline bool hide_topbar = true;
        inline bool watermark = true;
        inline std::vector<int>* watermarkstuff = new std::vector<int>{ 1, 1, 0 };
        

	}



	// Helper function for team checking - works with custom models
	inline bool is_teammate(const roblox::player& player) {
		// Get local player's team
		roblox::instance localPlayerTeam = globals::instances::lp.team;
		// Compare teams - return true if same team
		if (localPlayerTeam.address != 0 && player.team.address != 0) {
			if (localPlayerTeam.address == player.team.address) {
				return true; // Same team
			}
		}
		return false;
	}

	// Helper function for grabbed checking - works with custom models
	inline bool is_grabbed(const roblox::player& player) {
		// Use player.bodyeffects if available (works for both regular players and custom models)
		auto bodyEffects = player.bodyeffects;
		if (!bodyEffects.address) {
			// Fallback: try to find BodyEffects from instance (for custom models)
			bodyEffects = player.instance.findfirstchild("BodyEffects");
		}
		if (bodyEffects.address) {
			try {
				auto grabbedFlag = bodyEffects.findfirstchild("Grabbed");
				if (grabbedFlag.address && grabbedFlag.read_bool_value()) {
					return true;
				}
			} catch (...) { }
		}
		
		// Also check main for grabber/grabbed (for compatibility)
		if (is_valid_address(player.main.address)) {
			auto grabber = player.main.findfirstchild("Grabber");
			if (is_valid_address(grabber.address)) {
				return true;
			}
			auto grabbed = player.main.findfirstchild("Grabbed");
			if (is_valid_address(grabbed.address)) {
				return true;
			}
		}
		
		return false;
	}




	

}