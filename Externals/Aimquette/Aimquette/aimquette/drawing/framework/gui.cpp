#include "settings/functions.h"
#define NOMINMAX
#include <Windows.h>
#include "../../util/globals.h"
#include "../../util/config/configsystem.h"
#include "../../util/theme/thememanager.h"
#include "../../util/notification/notification.h"
#include "../../drawing/overlay/overlay.h"
#include "../../util/avatarmanager/avatarmanager.h"
// STB_IMAGE_IMPLEMENTATION is already defined in avatarmanager.cpp
#include "../../util/avatarmanager/stb_image.h"
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <stack>

static ConfigManager g_config_manager;
static ThemeManager g_theme_manager;


void c_gui::render()
{
	// Menu key handling is done in overlay.cpp to ensure it always works

	var->gui.menu_alpha = ImClamp(var->gui.menu_alpha + (gui->fixed_speed(8.f) * (var->gui.menu_opened ? 1.f : -1.f)), 0.f, 1.f);

	if (var->gui.menu_alpha <= 0.01f)
		return;

	// Only render topbar window if not hidden
	if (!globals::misc::hide_topbar) {
		gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2 - var->window.width / 2, 5));
		gui->set_next_window_size(ImVec2(var->window.width, elements->section.size.y + var->window.spacing.y * 2 - 1));
		gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.menu_alpha);
		gui->begin("Aimquette Control Panel", nullptr, var->window.main_flags);
		{
			const ImVec2 pos = GetWindowPos();
			const ImVec2 size = GetWindowSize();
			ImDrawList* draw_list = GetWindowDrawList();
			ImGuiStyle* style = &GetStyle();

			{
				style->WindowPadding = var->window.padding;
				style->PopupBorderSize = var->window.border_size;
				style->WindowBorderSize = var->window.border_size;
				style->ItemSpacing = var->window.spacing;
				style->WindowShadowSize = var->window.shadow_size;
				style->ScrollbarSize = var->window.scrollbar_size;
				style->Colors[ImGuiCol_WindowShadow] = { clr->accent.Value.x, clr->accent.Value.y, clr->accent.Value.z, var->window.shadow_alpha };
			}

			{
				draw->rect(GetBackgroundDrawList(), pos - ImVec2(1, 1), pos + size + ImVec2(1, 1), draw->get_clr({0, 0, 0, 0.5f}));
				draw->rect_filled(draw_list, pos, pos + size, draw->get_clr(clr->window.background_one));
				draw->line(draw_list, pos + ImVec2(1, 1), pos + ImVec2(size.x - 1, 1), draw->get_clr(clr->accent), 1);
				draw->line(draw_list, pos + ImVec2(1, 2), pos + ImVec2(size.x - 1, 2), draw->get_clr(clr->accent, 0.4f), 1);
				draw->rect(draw_list, pos, pos + size, draw->get_clr(clr->window.stroke));
			}

			{
				gui->set_cursor_pos(style->ItemSpacing);
				gui->begin_group();
				{
					for (int i = 0; i < IM_ARRAYSIZE(var->gui.current_section); i++)
						gui->section(var->gui.section_icons[i], &var->gui.current_section[i]);
				}
				gui->end_group();
			}

			var->window.width = GetCurrentWindow()->ContentSize.x + style->ItemSpacing.x;

			if (IsMouseHoveringRect(pos, pos + size))
				SetWindowFocus();
		}
		gui->end();
		gui->pop_style_var();
	} else {
		// Set default width when topbar is hidden so main menu windows can render properly
		if (var->window.width <= 0) {
			var->window.width = 600.0f; // Default width
		}
		// Always ensure first section is enabled when topbar is hidden so menu is accessible
		var->gui.current_section[0] = true;
	}

	// Apply alpha and style settings to main menu windows
	gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.menu_alpha);
	
	// Apply window style settings for main menu windows
	ImGuiStyle* style = &GetStyle();
	style->WindowPadding = var->window.padding;
	style->PopupBorderSize = var->window.border_size;
	style->WindowBorderSize = var->window.border_size;
	style->ItemSpacing = var->window.spacing;
	style->WindowShadowSize = var->window.shadow_size;
	style->ScrollbarSize = var->window.scrollbar_size;
	style->Colors[ImGuiCol_WindowShadow] = { clr->accent.Value.x, clr->accent.Value.y, clr->accent.Value.z, var->window.shadow_alpha };

		{
			if (var->gui.current_section[0])
			{
				// Always center the main menu window on injection/first open
				ImGuiWindow* existing_window = FindWindowByName("Aimquette l .gg/aimquette");
				bool window_exists = existing_window != nullptr;
				bool is_at_corner = window_exists && existing_window->Pos.x < 100.0f && existing_window->Pos.y < 100.0f;
				
				// Force center if window doesn't exist yet (first frame) or is at corner position
				if (!window_exists || is_at_corner) {
					gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2, GetIO().DisplaySize.y / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
				} else {
					gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2, GetIO().DisplaySize.y / 2), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
				}
				// Set fixed size to prevent size changes when tabbing
				gui->set_next_window_size(ImVec2(980, 600), ImGuiCond_FirstUseEver);
				gui->set_next_window_size_constraints(ImVec2(550, 500), GetIO().DisplaySize);
				gui->begin("Aimquette l .gg/aimquette", nullptr, var->window.flags);
				{
					// Force window size to be consistent - prevent size changes when tabbing
					ImGuiWindow* current_window = GetCurrentWindow();
					if (current_window && !current_window->Collapsed) {
						ImVec2 expected_size = ImVec2(980, 600);
						ImVec2 current_size = current_window->SizeFull;
						// If size differs significantly, force it back
						if (std::abs(current_size.x - expected_size.x) > 10.0f || std::abs(current_size.y - expected_size.y) > 10.0f) {
							current_window->Size = expected_size;
							current_window->SizeFull = expected_size;
						}
					}
					
					draw->window_decorations();

					{
						static int subtabs;
						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
						gui->begin_group();
						{
							gui->sub_section("Combat", 0, subtabs, 6);
							gui->sub_section("Visuals", 1, subtabs, 6);
							gui->sub_section("World", 2, subtabs, 6);
							gui->sub_section("Playerlist", 3, subtabs, 6);
							gui->sub_section("Workspace", 4, subtabs, 6);
							gui->sub_section("Settings", 5, subtabs, 6);
						}
						gui->end_group();

						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + elements->section.height - 1));
						gui->begin_content();
						{
							if (subtabs == 0) // Combat tab
							{
								// Disable scrolling on combat tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								static int combat_subtabs = 0;
								
								// Left box - changes content based on tab selection
								gui->begin_group();
								{
									gui->begin_child("Assist", 2, 1);
									{
										// Disable scrolling on Assist box
										ImGuiWindow* assist_window = GetCurrentWindow();
										if (assist_window)
										{
											assist_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											assist_window->Scroll = ImVec2(0, 0);
											assist_window->ScrollMax = ImVec2(0, 0);
										}
										
										// Tabs at the top of the child window - use sub_section for proper rendering
										gui->set_cursor_pos(elements->widgets.padding);
										gui->begin_group();
										{
											gui->sub_section("Assist", 0, combat_subtabs, 2);
											gui->sub_section("Silent Aim", 1, combat_subtabs, 2);
										}
										gui->end_group();

										// Add spacing below tabs
										ImGui::Spacing();

										if (combat_subtabs == 0) // Assist tab content
										{
											// Assist section
											gui->checkbox("Aimbot", &globals::combat::aimbot, &globals::combat::aimbotkeybind.key, reinterpret_cast<int*>(&globals::combat::aimbotkeybind.type));
											gui->checkbox("Sticky", &globals::combat::stickyaim);
											gui->checkbox("Unlock on death", &globals::combat::unlockondeath);
											
											gui->slider_float("Field Of View", &globals::combat::fovsize, 10, 1000);
											
											static int movement_type = 0;
											const char* movement_items[3] = { "Camera", "Mouse", "Memory" };
											gui->dropdown("Movement", &movement_type, movement_items, 3);
											globals::combat::aimbottype = movement_type;
											
											static int hitbox = 0;
											const char* hitbox_items[11] = { "Head", "Upper Torso", "Lower Torso", "HumanoidRootPart", "Left Hand", "Right Hand", "Left Foot", "Right Foot", "Closest Part", "Random Part", "Closest Point" };
											gui->dropdown("Hit Boxes", &hitbox, hitbox_items, 11);
											globals::combat::aimpart = hitbox;
											
											// Airpart feature
											gui->checkbox("Air Part", &globals::combat::airpart_enabled);
											if (globals::combat::airpart_enabled) {
												static int airpart = 0;
												const char* airpart_items[9] = { "Head", "Upper Torso", "Lower Torso", "HumanoidRootPart", "Left Hand", "Right Hand", "Left Foot", "Right Foot", "Closest Part" };
												gui->dropdown("Air Part", &airpart, airpart_items, 9);
												globals::combat::airpart = airpart;
											}
											
											// Ignore Parts dropdowns for Camera and Mouse aimbot types
											if (globals::combat::aimbottype == 0) {
												// Camera aimbot
												if (globals::combat::ignore_parts_camera == nullptr) {
													globals::combat::ignore_parts_camera = new std::vector<int>(8, 0);
												}
												const char* ignore_parts_items[] = { "Head", "Upper Torso", "Lower Torso", "HRP", "Left Hand", "Right Hand", "Left Foot", "Right Foot" };
												gui->multi_dropdown("Ignore Parts", *globals::combat::ignore_parts_camera, ignore_parts_items, 8);
											}
											else if (globals::combat::aimbottype == 1) {
												// Mouse aimbot
												if (globals::combat::ignore_parts_mouse == nullptr) {
													globals::combat::ignore_parts_mouse = new std::vector<int>(8, 0);
												}
												const char* ignore_parts_items[] = { "Head", "Upper Torso", "Lower Torso", "HRP", "Left Hand", "Right Hand", "Left Foot", "Right Foot" };
												gui->multi_dropdown("Ignore Parts", *globals::combat::ignore_parts_mouse, ignore_parts_items, 8);
											}
											
											gui->checkbox("Smoothness", &globals::combat::smoothing);
											if (globals::combat::smoothing)
											{
												gui->slider_float("Smoothness X", &globals::combat::smoothingx, 1, 100);
												gui->slider_float("Smoothness Y", &globals::combat::smoothingy, 1, 100);
												// Smoothing Styles: 0=None, 1=Linear, 2=EaseInQuad, 3=EaseOutQuad, 4=EaseInOutQuad, 5=EaseInCubic, 6=EaseOutCubic, 7=EaseInOutCubic, 8=EaseInSine, 9=EaseOutSine, 10=EaseInOutSine
												const char* smoothness_mode_items[11] = { 
													"None",              // 0 - No easing, raw smoothing
													"Linear",            // 1 - Constant speed
													"EaseInQuad",        // 2 - Slow start, fast end (acceleration)
													"EaseOutQuad",       // 3 - Fast start, slow end (deceleration)
													"EaseInOutQuad",    // 4 - Smooth start and end, fast middle
													"EaseInCubic",       // 5 - Very slow start, sharp speed-up
													"EaseOutCubic",      // 6 - Fast start, eases into stop
													"EaseInOutCubic",    // 7 - Smooth in and out with steep middle
													"EaseInSine",        // 8 - Ultra-smooth start
													"EaseOutSine",       // 9 - Ultra-smooth end
													"EaseInOutSine"      // 10 - Natural ease both ways
												};
												gui->dropdown("Smoothness Mode", &globals::combat::smoothingstyle, smoothness_mode_items, 11);
											}
											
											gui->checkbox("Prediction", &globals::combat::predictions);
											if (globals::combat::predictions)
											{
												gui->slider_float("Prediction X", &globals::combat::predictionsx, 0.1f, 50.0f);
												gui->slider_float("Prediction Y", &globals::combat::predictionsy, 0.1f, 50.0f);
											}
											
											// Shake controls for Camera and Memory aimbot types
											if (globals::combat::aimbottype == 0 || globals::combat::aimbottype == 2) {
												gui->checkbox("Shake", &globals::combat::camlock_shake);
												if (globals::combat::camlock_shake) {
													gui->slider_float("Shake X", &globals::combat::camlock_shake_x, 0.1f, 10.0f);
													gui->slider_float("Shake Y", &globals::combat::camlock_shake_y, 0.1f, 10.0f);
													gui->slider_float("Shake Z", &globals::combat::camlock_shake_z, 0.1f, 10.0f);
												}
											}
											
											gui->checkbox("Draw FOV", &globals::combat::drawfov);
											gui->sameline();
											gui->label_color_edit("Fov Color", globals::combat::fovcolor, false);
											
											
											
											gui->slider_float("Range", &globals::combat::range, 1, 25);
											gui->slider_float("Health", &globals::combat::healththreshhold, 0, 100);
										}
										else if (combat_subtabs == 1) // Silent Aim tab content
										{
											gui->checkbox("Silent", &globals::combat::silentaim, &globals::combat::silentaimkeybind.key, reinterpret_cast<int*>(&globals::combat::silentaimkeybind.type));
											gui->checkbox("Sticky Aim", &globals::combat::stickyaimsilent);
											gui->checkbox("Spoof Mouse", &globals::combat::spoofmouse);
											
											// Closest Part/Point dropdown for Silent Aim
											static int closestpart_mode = 0;
											const char* closestpart_items[3] = { "Off", "Closest Part", "Closest Point" };
											gui->dropdown("Closest Mode", &closestpart_mode, closestpart_items, 3);
											globals::combat::closestpartsilent = closestpart_mode;
											
											gui->slider_int("Hit Chance", &globals::combat::hitchance, 1, 100);
											
											gui->checkbox("Prediction", &globals::combat::silentpredictions);
											if (globals::combat::silentpredictions)
											{
												gui->slider_float("Prediction X", &globals::combat::silentpredictionsx, 0.1f, 50.0f);
												gui->slider_float("Prediction Y", &globals::combat::silentpredictionsy, 0.1f, 50.0f);
											}
											
											gui->checkbox("Show Fov", &globals::combat::drawsfov);
											gui->sameline();
											gui->label_color_edit("Fov Color", globals::combat::sfovcolor, false);
											gui->checkbox("Use Fov", &globals::combat::usesfov);
											gui->slider_float("Fov Radius", &globals::combat::sfovsize, 10, 1000);
											
											static int silentaimpart = 0;
											const char* silentaimpart_items[10] = { "Head", "Upper Torso", "Lower Torso", "HumanoidRootPart", "Left Hand", "Right Hand", "Left Foot", "Right Foot", "Closest Part", "Random Part" };
											gui->dropdown("Silent Aim Part", &silentaimpart, silentaimpart_items, IM_ARRAYSIZE(silentaimpart_items), true);
											globals::combat::silentaimpart = silentaimpart;
										}
									}
									gui->end_child();
							}
							gui->end_group();

							gui->sameline();

								// Right side - Checks, Triggerbot, and Movement/Misc
							gui->begin_group();
								{
									// Calculate half height for both Checks and Triggerbot boxes
									float parent_height = GetWindowHeight();
									float assist_box_height = parent_height - elements->content.padding.y * 2.f;
									float box_height = assist_box_height / 2.0f;
									
									// Checks box - top right, half height
									ImGuiWindow* parent_group = GetCurrentWindow();
									
									gui->begin_child("Checks", 2, 1, ImVec2(0, box_height));
									{
										// Flags checkboxes in Checks layout box
										if (globals::combat::flags == nullptr)
										{
											globals::combat::flags = new std::vector<int>(7, 0);
										}
										// Ensure vector has 7 elements
										if (globals::combat::flags->size() < 7)
										{
											globals::combat::flags->resize(7, 0);
										}
										
										// Use static variables to cache flag states and prevent rapid toggling
										static bool flag_team_state = false;
										static bool flag_knocked_state = false;
										static bool flag_range_state = false;
										static bool flag_health_state = false;
										static bool flag_wallcheck_state = false;
										static bool flag_grabbed_state = false;
										static bool flag_forcefield_state = false;
										static bool flags_initialized = false;
										
										// Initialize states from vector once
										if (!flags_initialized) {
											flag_team_state = (*globals::combat::flags)[0] != 0;
											flag_knocked_state = (*globals::combat::flags)[1] != 0;
											flag_range_state = (*globals::combat::flags)[2] != 0;
											flag_health_state = (*globals::combat::flags)[3] != 0;
											flag_wallcheck_state = (*globals::combat::flags)[4] != 0;
											flag_grabbed_state = (*globals::combat::flags)[5] != 0;
											flag_forcefield_state = (*globals::combat::flags)[6] != 0;
											flags_initialized = true;
										}
										
										// Sync states with vector (only read when vector changes externally)
										bool current_team = (*globals::combat::flags)[0] != 0;
										bool current_knocked = (*globals::combat::flags)[1] != 0;
										bool current_range = (*globals::combat::flags)[2] != 0;
										bool current_health = (*globals::combat::flags)[3] != 0;
										bool current_wallcheck = (*globals::combat::flags)[4] != 0;
										bool current_grabbed = (*globals::combat::flags)[5] != 0;
										bool current_forcefield = (*globals::combat::flags)[6] != 0;
										
										// Only update cached state if vector changed externally (e.g., from config load)
										if (current_team != flag_team_state) flag_team_state = current_team;
										if (current_knocked != flag_knocked_state) flag_knocked_state = current_knocked;
										if (current_range != flag_range_state) flag_range_state = current_range;
										if (current_health != flag_health_state) flag_health_state = current_health;
										if (current_wallcheck != flag_wallcheck_state) flag_wallcheck_state = current_wallcheck;
										if (current_grabbed != flag_grabbed_state) flag_grabbed_state = current_grabbed;
										if (current_forcefield != flag_forcefield_state) flag_forcefield_state = current_forcefield;
										
										// Flags checkboxes - use cached states
										if (gui->checkbox("Team", &flag_team_state)) {
											(*globals::combat::flags)[0] = flag_team_state ? 1 : 0;
										}
										if (gui->checkbox("Knocked", &flag_knocked_state)) {
											(*globals::combat::flags)[1] = flag_knocked_state ? 1 : 0;
										}
										if (gui->checkbox("Range", &flag_range_state)) {
											(*globals::combat::flags)[2] = flag_range_state ? 1 : 0;
										}
										if (flag_range_state) {
											static int range_preset = 4; // Default to 1000
											const char* range_items[] = { "100", "250", "500", "750", "1000", "1500", "2000", "2500", "3000", "5000" };
											float range_values[] = { 100.0f, 250.0f, 500.0f, 750.0f, 1000.0f, 1500.0f, 2000.0f, 2500.0f, 3000.0f, 5000.0f };
											// Sync dropdown with current value
											for (int i = 0; i < IM_ARRAYSIZE(range_values); i++) {
												if (std::abs(globals::combat::range - range_values[i]) < 1.0f) {
													range_preset = i;
													break;
												}
											}
											if (gui->dropdown("Range Value", &range_preset, range_items, IM_ARRAYSIZE(range_items), true)) {
												globals::combat::range = range_values[range_preset];
											}
										}
										if (gui->checkbox("Health", &flag_health_state)) {
											(*globals::combat::flags)[3] = flag_health_state ? 1 : 0;
										}
										if (flag_health_state) {
											static int health_preset = 1; // Default to 10
											const char* health_items[] = { "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100" };
											float health_values[] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f, 100.0f };
											// Sync dropdown with current value
											for (int i = 0; i < IM_ARRAYSIZE(health_values); i++) {
												if (std::abs(globals::combat::healththreshhold - health_values[i]) < 1.0f) {
													health_preset = i;
													break;
												}
											}
											if (gui->dropdown("Health Threshold", &health_preset, health_items, IM_ARRAYSIZE(health_items), true)) {
												globals::combat::healththreshhold = health_values[health_preset];
											}
										}
										if (gui->checkbox("WallCheck", &flag_wallcheck_state)) {
											(*globals::combat::flags)[4] = flag_wallcheck_state ? 1 : 0;
										}
										// Grabbed check removed per user request
										if (gui->checkbox("Forcefield", &flag_forcefield_state)) {
											(*globals::combat::flags)[6] = flag_forcefield_state ? 1 : 0;
										}
										gui->checkbox("Crew Check", &globals::combat::crew_check);
									}
									
									ImGuiWindow* checks_window = GetCurrentWindow();
									float checks_bottom = 0;
									if (checks_window && (checks_window->Flags & ImGuiWindowFlags_ChildWindow))
									{
										checks_bottom = checks_window->Pos.y + checks_window->SizeFull.y;
									}
									
									gui->end_child();

									// Position Triggerbot directly under Checks with proper spacing
									if (parent_group)
									{
										if (checks_bottom > 0)
										{
											// Add spacing between Checks and Triggerbot to prevent overlap
											parent_group->DC.CursorPos = ImVec2(parent_group->DC.CursorPos.x, checks_bottom + elements->content.spacing.y);
										}
										else
										{
											// Fallback: use normal spacing
											parent_group->DC.CursorPos.y += elements->content.spacing.y;
										}
										parent_group->DC.CursorPosPrevLine = parent_group->DC.CursorPos;
									}

									// Triggerbot box - bottom right, half height
									gui->begin_child("Triggerbot", 2, 1, ImVec2(0, box_height));
									{
										// Disable scrolling on Triggerbot box
										ImGuiWindow* triggerbot_window = GetCurrentWindow();
										if (triggerbot_window)
										{
											triggerbot_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											triggerbot_window->Scroll = ImVec2(0, 0);
											triggerbot_window->ScrollMax = ImVec2(0, 0);
										}
										
										gui->checkbox("Enable", &globals::combat::triggerbot, &globals::combat::triggerbotkeybind.key, reinterpret_cast<int*>(&globals::combat::triggerbotkeybind.type));
										gui->checkbox("Range", &globals::combat::triggerbotrange);
										if (globals::combat::triggerbotrange)
										{
											gui->slider_float("Range", &globals::combat::triggerbotrangevalue, 10, 500);
										}
										gui->checkbox("Knife Check", &globals::combat::knifecheck);
										gui->slider_float("Delay M/S", &globals::combat::delay, 0, 50);
										gui->slider_float("Release", &globals::combat::releasedelay, 0, 100);
									}
									gui->end_child();
									
									// Add Movement and Misc Combat features after Triggerbot
									ImGui::Spacing();
									
									gui->begin_child("Movement", 2, 1, ImVec2(0, box_height));
									{
										// Disable scrolling on Movement box
										ImGuiWindow* movement_window = GetCurrentWindow();
										if (movement_window)
										{
											movement_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											movement_window->Scroll = ImVec2(0, 0);
											movement_window->ScrollMax = ImVec2(0, 0);
										}
										
										gui->checkbox("Speed", &globals::misc::speed, &globals::misc::speedkeybind.key, (int*)&globals::misc::speedkeybind.type);
										if (globals::misc::speed) {
											static int speedtype = 0;
											const char* speedtype_items[3] = { "WalkSpeed", "Velocity", "CFrame" };
											gui->dropdown("Speed Type", &speedtype, speedtype_items, IM_ARRAYSIZE(speedtype_items), true);
											globals::misc::speedtype = speedtype;
											gui->slider_float("Speed Value", &globals::misc::speedvalue, 0, 200);
										}

										gui->checkbox("Flight", &globals::misc::flight, &globals::misc::flightkeybind.key, (int*)&globals::misc::flightkeybind.type);
										if (globals::misc::flight) {
											static int flighttype = 0;
											const char* flighttype_items[2] = { "CFrame", "Velocity" };
											gui->dropdown("Flight Type", &flighttype, flighttype_items, IM_ARRAYSIZE(flighttype_items), true);
											globals::misc::flighttype = flighttype;
											gui->slider_float("Flight Value", &globals::misc::flightvalue, 0, 200);
										}

										gui->checkbox("Jump Power", &globals::misc::jumppower, &globals::misc::jumppowerkeybind.key, (int*)&globals::misc::jumppowerkeybind.type);
										if (globals::misc::jumppower) {
											gui->slider_float("Jump Power Value", &globals::misc::jumpowervalue, 0, 200);
										}

										gui->checkbox("360 Spin", &globals::misc::spin360, &globals::misc::spin360keybind.key, (int*)&globals::misc::spin360keybind.type);
										if (globals::misc::spin360) {
											gui->slider_float("Spin Speed", &globals::misc::spin360speed, 1, 10);
										}

										gui->checkbox("Bike Fly", &globals::misc::bikefly);
									}
									gui->end_child();
									
									ImGui::Spacing();
									
									gui->begin_child("Misc Combat", 2, 1, ImVec2(0, box_height));
									{
										// Disable scrolling on Misc Combat box
										ImGuiWindow* misc_combat_window = GetCurrentWindow();
										if (misc_combat_window)
										{
											misc_combat_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											misc_combat_window->Scroll = ImVec2(0, 0);
											misc_combat_window->ScrollMax = ImVec2(0, 0);
										}
										
										gui->checkbox("Rapid Fire", &globals::misc::rapidfire);
										gui->checkbox("Auto Reload", &globals::misc::autoreload);
										gui->checkbox("Auto Armor", &globals::misc::autoarmor);
										gui->checkbox("Anti Stomp", &globals::misc::antistomp);
										gui->checkbox("Anti Aim", &globals::combat::antiaim);
										gui->checkbox("Underground Anti Aim", &globals::combat::underground_antiaim);
										
										gui->checkbox("Animation Changer", &globals::combat::animation_enable);
										if (globals::combat::animation_enable) {
											const char* animation_types[] = { "Default", "Ninja", "Robot", "Zombie", "Levitation", "Stylish", "Cartoony", "Super Hero", "Elder", "Toy", "Old School" };
											gui->dropdown("Walk Animation", &globals::combat::walk_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Run Animation", &globals::combat::run_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Jump Animation", &globals::combat::jump_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Fall Animation", &globals::combat::fall_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Idle Animation", &globals::combat::idle_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Climb Animation", &globals::combat::climb_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Swim Animation", &globals::combat::swim_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
										}
										
									}
									gui->end_child();
							}
							gui->end_group();
						}
							else if (subtabs == 1) // Visuals tab
							{
								// Disable scrolling on visuals tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								// Calculate height to match left side (ESP + Other boxes combined)
								float parent_height = GetWindowHeight();
								float content_padding = elements->content.padding.y * 2.f;
								float spacing = elements->content.spacing.y;
								float combined_height = parent_height - content_padding - spacing;
								
								gui->begin_group();
								{
									gui->begin_child("ESP", 2, 1, ImVec2(0, combined_height));
									{
										// Enable scrolling on ESP box - removed NoScrollbar and NoScrollWithMouse flags
										
										ImGui::Spacing();
										
										gui->checkbox("Enabled", &globals::visuals::visuals);
										gui->checkbox("Boxes", &globals::visuals::boxes);
										gui->sameline();
										gui->label_color_edit("Box Color", globals::visuals::boxcolors, false);
										if (globals::visuals::boxes) {
											static int boxtype = 0;
											const char* boxtype_items[3] = { "2D", "3D", "Corner" };
											gui->dropdown("Box Type", &boxtype, boxtype_items, IM_ARRAYSIZE(boxtype_items), true);
											globals::visuals::boxtype = boxtype;
											
											// Box overlay flags (Outline, Glow, Fill)
											if (globals::visuals::box_overlay_flags->size() != 3) {
												*globals::visuals::box_overlay_flags = { 0, 0, 0 };
											}
											const char* box_overlay_items[3] = { "Outline", "Glow", "Fill" };
											gui->multi_dropdown("Box Overlays", *globals::visuals::box_overlay_flags, box_overlay_items, 3);
											
											gui->label_color_edit("Box Fill", globals::visuals::boxfillcolor, false);
											gui->checkbox("Gradient", &globals::visuals::box_gradient);
											if (globals::visuals::box_gradient) {
												gui->label_color_edit("Gradient Color 1", globals::visuals::box_gradient_color1, false);
												gui->label_color_edit("Gradient Color 2", globals::visuals::box_gradient_color2, false);
												gui->checkbox("Rotate Gradient", &globals::visuals::box_gradient_rotation);
												if (globals::visuals::box_gradient_rotation) {
													gui->slider_float("Rotation Speed", &globals::visuals::box_gradient_rotation_speed, 0, 2);
												}
											}
										}

										gui->checkbox("Health Bar", &globals::visuals::healthbar);
										gui->sameline();
										gui->label_color_edit("Health Bar Color", globals::visuals::healthbarcolor, false);

										gui->checkbox("Armor Bar", &globals::visuals::armorbar);
										gui->checkbox("Name", &globals::visuals::name);
										gui->sameline();
										gui->label_color_edit("Name Color", globals::visuals::namecolor, false);

										gui->checkbox("Distance", &globals::visuals::distance);
										gui->sameline();
										gui->label_color_edit("Distance Color", globals::visuals::distancecolor, false);

										gui->checkbox("Tool ESP", &globals::visuals::toolesp);
										gui->sameline();
										gui->label_color_edit("Tool Color", globals::visuals::toolespcolor, false);

										gui->checkbox("Flags", &globals::visuals::flags);
										gui->sameline();
										gui->label_color_edit("Flags Color", globals::visuals::flagscolor, false);
										if (globals::visuals::flags) {
											gui->slider_float("Flags Font Size", &globals::visuals::flags_font_size, 8, 20);
											gui->slider_float("Flags Offset X", &globals::visuals::flags_offset_x, 0, 20);
										}

										gui->checkbox("Skeletons", &globals::visuals::skeletons);
										gui->sameline();
										gui->label_color_edit("Skeleton Color", globals::visuals::skeletonscolor, false);

										gui->checkbox("Chams", &globals::visuals::chams);
										gui->sameline();
										gui->label_color_edit("Chams Color", globals::visuals::chamscolor, false);
										if (globals::visuals::chams) {
											gui->checkbox("Ignore Z (Wallhack)", &globals::visuals::chams_ignorez);
											gui->sameline();
											gui->label_color_edit("Ignore Z Color", globals::visuals::chams_ignorez_color, false);
											// Chams overlay flags (Outline, Glow, Fill)
											if (globals::visuals::chams_overlay_flags->size() != 3) {
												*globals::visuals::chams_overlay_flags = { 0, 0, 0 };
											}
											const char* chams_overlay_items[3] = { "Outline", "Glow", "Fill" };
											gui->multi_dropdown("Chams Overlays", *globals::visuals::chams_overlay_flags, chams_overlay_items, 3);
										}

										gui->checkbox("Tracers", &globals::visuals::tracers);
										gui->sameline();
										gui->label_color_edit("Tracer Color", globals::visuals::tracerscolor, false);
										if (globals::visuals::tracers) {
											static int tracerstype = 0;
											const char* tracerstype_items[3] = { "Off", "Normal", "Spiderweb" };
											gui->dropdown("Tracer Type", &tracerstype, tracerstype_items, IM_ARRAYSIZE(tracerstype_items), true);
											globals::visuals::tracerstype = tracerstype;
											gui->checkbox("Tracer Outline", &globals::visuals::tracers_outline);
											gui->checkbox("Tracer Glow", &globals::visuals::tracers_glow);
										}

										gui->checkbox("China Hat", &globals::visuals::chinahat);
										gui->sameline();
										gui->label_color_edit("China Hat Color", globals::visuals::chinahat_color, false);
										if (globals::visuals::chinahat) {
											gui->checkbox("Target Only", &globals::visuals::chinahat_target_only);
										}

										gui->checkbox("Glow ESP", &globals::visuals::glowesp);
										gui->sameline();
										gui->label_color_edit("Glow Color", globals::visuals::glowcolor, false);

										gui->checkbox("Locked Indicator", &globals::visuals::lockedindicator);
										gui->sameline();
										gui->label_color_edit("Locked Color", globals::visuals::lockedcolor, false);

										gui->checkbox("Self ESP", &globals::visuals::selfesp);
										
										gui->checkbox("Crosshair", &globals::visuals::crosshair_enabled);
										gui->sameline();
										gui->label_color_edit("Crosshair Color", globals::visuals::crosshair_color, false);
										if (globals::visuals::crosshair_enabled) {
											gui->slider_float("Crosshair Size", &globals::visuals::crosshair_size, 5, 50);
											gui->slider_float("Crosshair Gap", &globals::visuals::crosshair_gap, 0, 20);
											gui->slider_float("Crosshair Thickness", &globals::visuals::crosshair_thickness, 1, 5);
											static int crosshair_style = 0;
											const char* crosshair_style_items[5] = { "Cross", "Circle", "Square", "Dot", "Custom" };
											gui->dropdown("Crosshair Style", &crosshair_style, crosshair_style_items, IM_ARRAYSIZE(crosshair_style_items), true);
											globals::visuals::crosshair_styleIdx = crosshair_style;
										}


										gui->checkbox("Sonar", &globals::visuals::sonar);
										gui->sameline();
										gui->label_color_edit("Sonar Color", globals::visuals::sonarcolor, false);
										if (globals::visuals::sonar) {
											gui->slider_float("Sonar Range", &globals::visuals::sonar_range, 10, 200);
											gui->slider_float("Sonar Thickness", &globals::visuals::sonar_thickness, 1, 5);
											gui->checkbox("Show Distance", &globals::visuals::sonar_show_distance);
											gui->label_color_edit("Sonar Dot Color", globals::visuals::sonar_dot_color, false);
										}

										gui->checkbox("Fog", &globals::visuals::fog);
										gui->sameline();
										gui->label_color_edit("Fog Color", globals::visuals::fog_color, false);
										if (globals::visuals::fog) {
											gui->slider_float("Fog Start", &globals::visuals::fog_start, 0, 1000);
											gui->slider_float("Fog End", &globals::visuals::fog_end, 0, 5000);
											gui->checkbox("Rainbow Fog", &globals::visuals::rainbow_fog);
											if (globals::visuals::rainbow_fog) {
												gui->slider_float("Rainbow Speed", &globals::visuals::rainbow_fog_speed, 0.1f, 10.0f);
											}
											gui->checkbox("Fog Glow", &globals::visuals::fog_glow);
											if (globals::visuals::fog_glow) {
												gui->slider_float("Glow Intensity", &globals::visuals::fog_glow_intensity, 0.1f, 5.0f);
												gui->label_color_edit("Glow Color", globals::visuals::fog_glow_color, false);
											}
										}

										gui->checkbox("Max Distance", &globals::visuals::maxdistance_enabled);
										if (globals::visuals::maxdistance_enabled) {
											gui->slider_float("Max Distance", &globals::visuals::maxdistance, 100, 5000);
										}
										
										ImGui::Separator();
										gui->checkbox("Lighting Modifications", &globals::visuals::lighting_modifications);
										if (globals::visuals::lighting_modifications) {
											gui->slider_float("Brightness", &globals::visuals::lighting_brightness, 0.0f, 10.0f);
											gui->slider_float("Contrast", &globals::visuals::lighting_contrast, 0.0f, 2.0f);
											gui->label_color_edit("Color Correction", globals::visuals::lighting_color_correction, false);
											gui->checkbox("Shadows", &globals::visuals::lighting_shadows);
											gui->label_color_edit("Ambient", globals::visuals::lighting_ambient, false);
										}
										
										gui->checkbox("Range Check", &globals::visuals::rangecheck);
										if (globals::visuals::rangecheck) {
											gui->slider_float("Range", &globals::visuals::range, 0, 5000);
										}

										gui->checkbox("Enemy Check", &globals::visuals::enemycheck);
										gui->checkbox("Friendly Check", &globals::visuals::friendlycheck);
										gui->checkbox("Team Check", &globals::visuals::teamcheck);
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Effects", 2, 1, ImVec2(0, combined_height));
									{
										// Disable scrolling on Effects box
										ImGuiWindow* effects_window = GetCurrentWindow();
										if (effects_window)
										{
											effects_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											effects_window->Scroll = ImVec2(0, 0);
											effects_window->ScrollMax = ImVec2(0, 0);
										}
										gui->checkbox("Hit Tracer", &globals::visuals::hittracer);
										gui->checkbox("Trail", &globals::visuals::trail);
										gui->sameline();
										gui->label_color_edit("Trail Color", globals::visuals::trail_color, false);
										if (globals::visuals::trail) {
											gui->slider_float("Trail Duration", &globals::visuals::trail_duration, 0.5f, 10.0f);
											gui->slider_float("Trail Thickness", &globals::visuals::trail_thickness, 1.0f, 10.0f);
										}

										gui->checkbox("Hit Bubble", &globals::visuals::hitbubble);
										gui->checkbox("Target Skeleton", &globals::visuals::targetskeleton);
										gui->checkbox("Prediction Dot", &globals::visuals::predictionsdot);
										gui->checkbox("Fortnite Indicator", &globals::visuals::fortniteindicator);
									}
									gui->end_child();
								}
								gui->end_group();
						}
							else if (subtabs == 2) // World tab
							{
								// Disable scrolling on world tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								gui->begin_group();
								{
									gui->begin_child("Movement", 2, 1);
									{
										// Disable scrolling on Movement box
										ImGuiWindow* movement_window = GetCurrentWindow();
										if (movement_window)
										{
											movement_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											movement_window->Scroll = ImVec2(0, 0);
											movement_window->ScrollMax = ImVec2(0, 0);
										}
										
										gui->checkbox("Speed", &globals::misc::speed, &globals::misc::speedkeybind.key, (int*)&globals::misc::speedkeybind.type);
										if (globals::misc::speed) {
											static int speedtype = 0;
											const char* speedtype_items[3] = { "WalkSpeed", "Velocity", "CFrame" };
											gui->dropdown("Speed Type", &speedtype, speedtype_items, IM_ARRAYSIZE(speedtype_items), true);
											globals::misc::speedtype = speedtype;
											gui->slider_float("Speed Value", &globals::misc::speedvalue, 0, 200);
										}

										gui->checkbox("Flight", &globals::misc::flight, &globals::misc::flightkeybind.key, (int*)&globals::misc::flightkeybind.type);
										if (globals::misc::flight) {
											static int flighttype = 0;
											const char* flighttype_items[2] = { "CFrame", "Velocity" };
											gui->dropdown("Flight Type", &flighttype, flighttype_items, IM_ARRAYSIZE(flighttype_items), true);
											globals::misc::flighttype = flighttype;
											gui->slider_float("Flight Value", &globals::misc::flightvalue, 0, 200);
										}

										gui->checkbox("Jump Power", &globals::misc::jumppower, &globals::misc::jumppowerkeybind.key, (int*)&globals::misc::jumppowerkeybind.type);
										if (globals::misc::jumppower) {
											gui->slider_float("Jump Power Value", &globals::misc::jumpowervalue, 0, 200);
										}

										gui->checkbox("360 Spin", &globals::misc::spin360, &globals::misc::spin360keybind.key, (int*)&globals::misc::spin360keybind.type);
										if (globals::misc::spin360) {
											gui->slider_float("Spin Speed", &globals::misc::spin360speed, 1, 10);
										}

										gui->checkbox("Bike Fly", &globals::misc::bikefly);
										
										gui->checkbox("Headless", &globals::misc::headless);
										
										gui->checkbox("Rapid Fire", &globals::misc::rapidfire);
										gui->checkbox("Auto Reload", &globals::misc::autoreload);
										gui->checkbox("Auto Armor", &globals::misc::autoarmor);
										gui->checkbox("Anti Stomp", &globals::misc::antistomp);
										gui->checkbox("Anti Aim", &globals::combat::antiaim);
										gui->checkbox("Underground Anti Aim", &globals::combat::underground_antiaim);
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Desync", 2, 1);
									{
										// Disable scrolling on Desync box
										ImGuiWindow* desync_window = GetCurrentWindow();
										if (desync_window)
										{
											desync_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											desync_window->Scroll = ImVec2(0, 0);
											desync_window->ScrollMax = ImVec2(0, 0);
										}
										
										
										gui->checkbox("Animation Changer", &globals::combat::animation_enable);
										if (globals::combat::animation_enable) {
											const char* animation_types[] = { "Default", "Ninja", "Robot", "Zombie", "Levitation", "Stylish", "Cartoony", "Super Hero", "Elder", "Toy", "Old School" };
											gui->dropdown("Walk Animation", &globals::combat::walk_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Run Animation", &globals::combat::run_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Jump Animation", &globals::combat::jump_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Fall Animation", &globals::combat::fall_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Idle Animation", &globals::combat::idle_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Climb Animation", &globals::combat::climb_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
											gui->dropdown("Swim Animation", &globals::combat::swim_animation, animation_types, IM_ARRAYSIZE(animation_types), false);
											gui->sameline();
											if (gui->button("+", 1)) {
												// Button clicked - can add functionality here if needed
											}
										}
										
										
									}
									gui->end_child();
								}
								gui->end_group();
							}
							else if (subtabs == 3) // Playerlist tab
							{
								// Disable scrolling on playerlist tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								// Get players from cache
								std::vector<roblox::player> players;
								if (globals::instances::cachedplayers.size() > 0) {
									players = globals::instances::cachedplayers;
								}
								
								static int selected_player = -1;
								static std::vector<int> player_status;
								static bool spectating = false;
								static char search_query[256] = "";
								
								// Colors
								ImU32 text_color = draw->get_clr(clr->widgets.text);
								ImU32 text_dim_color = draw->get_clr(clr->widgets.text_inactive);
								ImVec4 text_dim = clr->widgets.text_inactive.Value;
								
								// Playerlist - rrext style (spans 2 layout boxes)
								gui->begin_child("Playerlist", 1, 1);
								{
									// Disable scrolling on Playerlist box
									ImGuiWindow* playerlist_window = GetCurrentWindow();
									if (playerlist_window)
									{
										playerlist_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
										playerlist_window->Scroll = ImVec2(0, 0);
										playerlist_window->ScrollMax = ImVec2(0, 0);
									}
									
									// Theme colors
									ImU32 enemy_color = IM_COL32(255, 100, 100, 255); // Keep red for enemy
									ImU32 friendly_color = IM_COL32(100, 255, 100, 255); // Keep green for friendly
									ImU32 neutral_color = draw->get_clr(clr->widgets.text_inactive); // Use theme inactive text color
									ImU32 client_color = draw->get_clr(clr->accent); // Use theme accent color
									ImU32 bg_selected = draw->get_clr(clr->window.background_two); // Use theme background
									ImU32 accent_color = draw->get_clr(clr->accent); // For highlighting search matches
									
									// Search bar
									ImGui::InputTextWithHint("##PlayerlistSearch", "Search players...", search_query, sizeof(search_query));
									
									ImGui::Spacing();
									ImGui::Separator();
									ImGui::Spacing();
									
									// Filter players based on search
									std::vector<roblox::player> filtered_players;
									std::vector<size_t> original_indices; // Track original indices for player_status
									
									if (strlen(search_query) == 0) {
										filtered_players = players;
										for (size_t i = 0; i < players.size(); i++) {
											original_indices.push_back(i);
										}
									}
									else {
										std::string query_lower = search_query;
										std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
										
										for (size_t i = 0; i < players.size(); i++) {
											std::string name_lower = players[i].name;
											std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
											
											if (name_lower.find(query_lower) != std::string::npos) {
												filtered_players.push_back(players[i]);
												original_indices.push_back(i);
											}
										}
									}
									
									// Initialize player status
									if (player_status.size() != players.size()) {
										player_status.resize(players.size(), 2);
										
										for (size_t i = 0; i < players.size(); i++) {
											auto& player = players[i];
											
											if (player.name == globals::instances::lp.name) {
												player_status[i] = 3; // Client
											}
											else if (std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), player.name) != globals::instances::whitelist.end()) {
												player_status[i] = 1; // Friendly
											}
											else if (std::find(globals::instances::blacklist.begin(), globals::instances::blacklist.end(), player.name) != globals::instances::blacklist.end()) {
												player_status[i] = 0; // Enemy
											}
										}
									}
									
									for (size_t idx = 0; idx < filtered_players.size(); idx++) {
										size_t i = original_indices[idx];
										auto& player = filtered_players[idx];
										ImGui::PushID(static_cast<int>(i));
										
										bool is_selected = (selected_player == static_cast<int>(i));
										bool is_client = (player.name == globals::instances::lp.name);
										
										// Get status
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
										
										// Simple rrext-style item - clean list with border highlight
										ImVec2 item_min = ImGui::GetCursorScreenPos();
										ImVec2 item_size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeight() + 8.0f);
										ImVec2 item_max = ImVec2(item_min.x + item_size.x, item_min.y + item_size.y);
										
										ImDrawList* draw_list = ImGui::GetWindowDrawList();
										
										// Highlight selected player with accent color background and text
										if (is_selected) {
											// Subtle accent-colored background
											ImU32 accent_bg = IM_COL32(
												static_cast<int>(clr->accent.Value.x * 255),
												static_cast<int>(clr->accent.Value.y * 255),
												static_cast<int>(clr->accent.Value.z * 255),
												30 // Low alpha for subtle background
											);
											draw_list->AddRectFilled(item_min, item_max, accent_bg);
										}
										
										// Player name with search highlighting
										std::string display_name = player.name.length() > 40 ? player.name.substr(0, 37) + "..." : player.name;
										ImVec2 name_pos = ImVec2(item_min.x + 8.0f, item_min.y + item_size.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
										
										// Use accent color for selected player text
										ImU32 player_text_color = is_selected ? accent_color : text_color;
										
										// Highlight search matches in accent color
										if (strlen(search_query) > 0) {
											std::string query_lower = search_query;
											std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
											std::string name_lower = player.name;
											std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
											
											size_t pos = name_lower.find(query_lower);
											if (pos != std::string::npos) {
												// Draw text before match
												if (pos > 0) {
													std::string before = display_name.substr(0, pos);
													draw_list->AddText(name_pos, player_text_color, before.c_str());
													name_pos.x += ImGui::CalcTextSize(before.c_str()).x;
												}
												
												// Draw matched text in accent color (brighter if selected)
												ImU32 match_color = is_selected ? accent_color : accent_color;
												std::string match = display_name.substr(pos, query_lower.length());
												draw_list->AddText(name_pos, match_color, match.c_str());
												name_pos.x += ImGui::CalcTextSize(match.c_str()).x;
												
												// Draw text after match
												if (pos + query_lower.length() < display_name.length()) {
													std::string after = display_name.substr(pos + query_lower.length());
													draw_list->AddText(name_pos, player_text_color, after.c_str());
												}
											}
											else {
												// No match, draw normally
												draw_list->AddText(name_pos, player_text_color, display_name.c_str());
											}
										}
										else {
											// No search, draw normally
											draw_list->AddText(name_pos, player_text_color, display_name.c_str());
										}
										
										// Clickable area
										ImGui::SetCursorScreenPos(item_min);
										ImGui::InvisibleButton("##player_item", item_size);
										
										if (ImGui::IsItemClicked() && !is_client) {
											selected_player = (selected_player == static_cast<int>(i)) ? -1 : static_cast<int>(i);
										}
										
										ImGui::SetCursorScreenPos(ImVec2(item_min.x, item_max.y));
										
										// Show buttons directly under selected player (rrext style)
										if (is_selected && !is_client) {
											ImGui::Spacing();
											ImGui::Indent(8.0f);
											
											// Helper functions
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
											
											// All buttons in one row - use val parameter to divide width evenly
											// Calculate based on available content width, accounting for padding
											float available_width = ImGui::GetContentRegionAvail().x;
											float button_spacing = ImGui::GetStyle().ItemSpacing.x;
											
											// Use val=3 to divide width by 3 for 3 buttons
											int button_val = 3;
											
											// Teleport button
											if (gui->button("Teleport", button_val)) {
												if (is_valid_address(player.hrp.address) && is_valid_address(globals::instances::lp.hrp.address)) {
													Vector3 target_pos = player.hrp.get_pos();
													globals::instances::lp.hrp.write_position(target_pos);
												}
											}
											
											ImGui::SameLine();
											
											// Spectate button
											std::string spectate_text = spectating ? "Unspectate" : "Spectate";
											if (gui->button(spectate_text.c_str(), button_val)) {
												roblox::instance cam;
												if (!spectating) {
													spectating = true;
													cam.spectate(player.hrp.address);
												}
												else {
													spectating = false;
													cam.unspectate();
												}
											}
											
											ImGui::SameLine();
											
											// Whitelist button
											bool is_whitelisted = std::find(globals::instances::whitelist.begin(), globals::instances::whitelist.end(), player.name) != globals::instances::whitelist.end();
											if (is_whitelisted) {
												// Convert ImU32 to ImVec4 (RGBA format)
												ImVec4 friendly_col = ImVec4(0.39f, 1.0f, 0.39f, 0.5f); // Green from friendly_color
												ImVec4 friendly_col_hover = ImVec4(0.39f, 1.0f, 0.39f, 0.7f);
												ImGui::PushStyleColor(ImGuiCol_Button, friendly_col);
												ImGui::PushStyleColor(ImGuiCol_ButtonHovered, friendly_col_hover);
											}
											if (gui->button("Whitelist", button_val)) {
												if (is_whitelisted) {
													removeFromLists(player.name);
													if (i < static_cast<int>(player_status.size())) {
														player_status[i] = 2;
													}
												}
												else {
													addToWhitelist(player.name);
													if (i < static_cast<int>(player_status.size())) {
														player_status[i] = 1;
													}
												}
											}
											if (is_whitelisted) {
												ImGui::PopStyleColor(2);
											}
											
											ImGui::Unindent(8.0f);
											ImGui::Spacing();
										}
										
										ImGui::PopID();
										}
								}
								gui->end_child();
							}
							else if (subtabs == 4) // Workspace tab (Centrum-style)
							{
								// Disable scrolling on workspace tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								// Workspace - Centrum style (spans 2 layout boxes)
								gui->begin_child("Workspace", 1, 1);
								{
									// Enable scrolling on Workspace box (contains the tree view items)
									ImGuiWindow* workspace_window = GetCurrentWindow();
									if (workspace_window)
									{
										workspace_window->Flags &= ~(ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
									}
									
									// Colors
									ImU32 text_color = draw->get_clr(clr->widgets.text);
									ImU32 text_dim_color = draw->get_clr(clr->widgets.text_inactive);
									ImVec4 text_dim = clr->widgets.text_inactive.Value;
									
									// Centrum-style workspace with tabs and tree view
									static int workspace_tab = 0; // 0 = Workspace, 1 = Services, 2 = Players
									static roblox::instance selected_instance;
									static std::unordered_set<uint64_t> expanded_nodes;
									static std::unordered_map<uint64_t, std::vector<roblox::instance>> node_cache;
									static std::unordered_map<uint64_t, std::string> node_name_cache;
									static std::unordered_map<uint64_t, std::string> node_class_cache;
									static char search_query[256] = "";
									static std::vector<roblox::instance> search_results;
									static bool show_search_results = false;
									static auto last_cache_refresh = std::chrono::steady_clock::now();
									
									// Class prefixes like Centrum
									static std::unordered_map<std::string, std::string> class_prefixes = {
										{"Workspace", "[WS] "},
										{"Players", "[P] "},
										{"Folder", "[F] "},
										{"Part", "[PT] "},
										{"BasePart", "[BP] "},
										{"Script", "[S] "},
										{"LocalScript", "[LS] "},
										{"ModuleScript", "[MS] "},
										{"Model", "[M] "},
										{"Humanoid", "[H] "}
									};
									
									// Cache node function
									auto cache_node = [&](roblox::instance& node) {
										if (node_cache.find(node.address) == node_cache.end()) {
											try {
												node_cache[node.address] = node.get_children();
												node_name_cache[node.address] = node.get_name();
												node_class_cache[node.address] = node.get_class_name();
											}
											catch (...) {
												// Ignore errors
											}
										}
									};
									
									// Tabs - using same style as main tabs
									gui->begin_group();
									{
										gui->sub_section("Workspace", 0, workspace_tab, 3);
										gui->sub_section("Services", 1, workspace_tab, 3);
										gui->sub_section("Players", 2, workspace_tab, 3);
									}
									gui->end_group();
									
									ImGui::Spacing();
									
									// Search bar
									ImGui::InputTextWithHint("##WorkspaceSearch", "Search...", search_query, sizeof(search_query));
									
									ImGui::SameLine();
									if (gui->button("Refresh", 3)) {
										node_cache.clear();
										node_name_cache.clear();
										node_class_cache.clear();
										search_results.clear();
										show_search_results = false;
										expanded_nodes.clear();
										last_cache_refresh = std::chrono::steady_clock::now() - std::chrono::seconds(3);
									}
									
									ImGui::Spacing();
									ImGui::Separator();
									ImGui::Spacing();
									
									// Get root instance based on tab
									roblox::instance root_instance;
									try {
										auto& datamodel = globals::instances::datamodel;
										if (workspace_tab == 0) {
											// Workspace
											root_instance = globals::instances::workspace;
										}
										else if (workspace_tab == 1) {
											// Services - get DataModel
											root_instance = roblox::instance(datamodel.address);
										}
										else if (workspace_tab == 2) {
											// Players
											root_instance = globals::instances::players;
										}
									}
									catch (...) {
										// Ignore errors
									}
									
									// Refresh cache every 2 seconds
									auto now = std::chrono::steady_clock::now();
									if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cache_refresh).count() >= 2) {
										if (node_cache.size() > 5000) {
											node_cache.clear();
											node_name_cache.clear();
											node_class_cache.clear();
										}
										last_cache_refresh = now;
									}
									
									// Handle search
									if (strlen(search_query) > 0) {
										if (!show_search_results) {
											search_results.clear();
											std::string query = search_query;
											std::transform(query.begin(), query.end(), query.begin(), ::tolower);
											
											std::function<void(roblox::instance&)> search_instance = [&](roblox::instance& inst) {
												if (search_results.size() >= 100) return;
												
												cache_node(inst);
												
												try {
													std::string name = node_name_cache[inst.address];
													std::transform(name.begin(), name.end(), name.begin(), ::tolower);
													
													if (name.find(query) != std::string::npos) {
														search_results.push_back(inst);
													}
													
													for (auto& child : node_cache[inst.address]) {
														search_instance(child);
													}
												}
												catch (...) {
													// Ignore errors
												}
											};
											
											if (root_instance.address != 0) {
												cache_node(root_instance);
												search_instance(root_instance);
											}
											
											show_search_results = true;
										}
									}
									else {
										show_search_results = false;
									}
									
									// Render tree or search results
									if (show_search_results && strlen(search_query) > 0) {
										ImGui::Text("Search Results (%d):", static_cast<int>(search_results.size()));
										ImGui::Separator();
										
										for (auto& node : search_results) {
											if (node.address == 0) continue;
											
											cache_node(node);
											
											ImGui::PushID(static_cast<int>(node.address));
											
											std::string name = node_name_cache[node.address];
											std::string class_name = node_class_cache[node.address];
											std::string prefix = "";
											
											if (class_prefixes.find(class_name) != class_prefixes.end()) {
												prefix = class_prefixes[class_name];
											}
											
											std::string display_text = prefix + name + " [" + class_name + "]";
											bool is_selected = (selected_instance.address == node.address);
											
											if (ImGui::Selectable(display_text.c_str(), is_selected)) {
												selected_instance = node;
											}
											
											ImGui::PopID();
										}
									}
									else {
										// Render tree view
										if (root_instance.address != 0) {
											cache_node(root_instance);
											
											std::function<void(roblox::instance&, int)> render_node = [&](roblox::instance& node, int depth) {
												if (node.address == 0) return;
												
												cache_node(node);
												
												ImGui::PushID(static_cast<int>(node.address));
												
												std::string name = node_name_cache[node.address];
												std::string class_name = node_class_cache[node.address];
												std::string prefix = "";
												
												if (class_prefixes.find(class_name) != class_prefixes.end()) {
													prefix = class_prefixes[class_name];
												}
												
												std::string display_text = prefix + name + " [" + class_name + "]";
												bool is_selected = (selected_instance.address == node.address);
												bool has_children = !node_cache[node.address].empty();
												
												ImGuiTreeNodeFlags flags = has_children ? 0 : ImGuiTreeNodeFlags_Leaf;
												flags |= ImGuiTreeNodeFlags_OpenOnArrow;
												if (is_selected) {
													flags |= ImGuiTreeNodeFlags_Selected;
												}
												
												bool is_expanded = ImGui::TreeNodeEx(display_text.c_str(), flags);
												
												if (ImGui::IsItemClicked()) {
													selected_instance = node;
												}
												
												if (is_expanded) {
													if (has_children) {
														for (auto& child : node_cache[node.address]) {
															render_node(child, depth + 1);
														}
													}
													ImGui::TreePop();
												}
												
												ImGui::PopID();
											};
											
											// Render root and children
											std::string root_name = node_name_cache[root_instance.address];
											std::string root_class = node_class_cache[root_instance.address];
											std::string root_prefix = "";
											
											if (class_prefixes.find(root_class) != class_prefixes.end()) {
												root_prefix = class_prefixes[root_class];
											}
											
											std::string root_display = root_prefix + root_name + " [" + root_class + "]";
											bool root_selected = (selected_instance.address == root_instance.address);
											bool root_has_children = !node_cache[root_instance.address].empty();
											
											ImGuiTreeNodeFlags root_flags = root_has_children ? 0 : ImGuiTreeNodeFlags_Leaf;
											root_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
											if (root_selected) {
												root_flags |= ImGuiTreeNodeFlags_Selected;
											}
											
											if (ImGui::TreeNodeEx(root_display.c_str(), root_flags)) {
												if (ImGui::IsItemClicked()) {
													selected_instance = root_instance;
												}
												
												if (root_has_children) {
													for (auto& child : node_cache[root_instance.address]) {
														render_node(child, 1);
													}
												}
												ImGui::TreePop();
											}
										}
										else {
											ImGui::TextColored(text_dim, "No instances found");
										}
									}
									
									// Show action buttons for selected instance
									if (selected_instance.address != 0) {
										ImGui::Spacing();
										ImGui::Separator();
										ImGui::Spacing();
										
										std::string selected_name = node_name_cache[selected_instance.address];
										ImGui::Text("Selected: %s", selected_name.c_str());
										
										ImGui::Spacing();
										
										// Action buttons
										int button_val = 3;
										
										// Teleport To button
										if (gui->button("Teleport To", button_val)) {
											try {
												if (is_valid_address(selected_instance.address)) {
													Vector3 target_pos = selected_instance.get_pos();
													target_pos.y += 5.0f; // Offset above
													if (is_valid_address(globals::instances::lp.hrp.address)) {
														globals::instances::lp.hrp.write_position(target_pos);
													}
												}
											}
											catch (...) {
												// Ignore errors
											}
										}
										
										ImGui::SameLine();
										
										// Copy Path button
										if (gui->button("Copy Path", button_val)) {
											try {
												std::string path = selected_instance.get_name();
												roblox::instance parent = selected_instance.read_parent();
												while (parent.address != 0) {
													std::string parent_name = parent.get_name();
													if (!parent_name.empty()) {
														path = parent_name + "." + path;
													}
													parent = parent.read_parent();
												}
												ImGui::SetClipboardText(path.c_str());
											}
											catch (...) {
												// Ignore errors
											}
										}
										
										ImGui::SameLine();
										
										// Copy Address button
										if (gui->button("Copy Address", button_val)) {
											try {
												std::string addr_str = std::to_string(selected_instance.address);
												ImGui::SetClipboardText(addr_str.c_str());
											}
											catch (...) {
												// Ignore errors
											}
										}
									}
								}
								gui->end_child();
							}
							else if (subtabs == 5) // Settings tab
							{
								// Disable scrolling on settings tab content area
								ImGuiWindow* content_window = GetCurrentWindow();
								if (content_window)
								{
									content_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
									content_window->Scroll = ImVec2(0, 0);
									content_window->ScrollMax = ImVec2(0, 0);
								}
								
								gui->begin_group();
								{
									// Left: Main Tab
									gui->begin_child("Main Tab", 2, 1);
									{
										// Disable scrolling on Main Tab box
										ImGuiWindow* main_tab_window = GetCurrentWindow();
										if (main_tab_window)
										{
											main_tab_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											main_tab_window->Scroll = ImVec2(0, 0);
											main_tab_window->ScrollMax = ImVec2(0, 0);
										}
										
										gui->checkbox("VSync", &globals::misc::vsync);
										
										// Max FPS Limit
										gui->slider_int("Max FPS Limit", &globals::misc::max_fps, 1, 300);
										
										// Anti-Capture (using streamproof)
										gui->checkbox("Anti-Capture", &globals::misc::streamproof);
										
										// NPC Check
										gui->checkbox("NPC Check", &globals::misc::npc_check);
										
										// Keybinds toggle
										gui->checkbox("Keybinds", &globals::misc::keybinds);
										
										// Player List toggle
										gui->checkbox("Player List", &globals::misc::playerlist);
										
										// Target HUD toggle
										gui->checkbox("Target HUD", &globals::misc::targethud);
										
										// Watermark Options
										gui->checkbox("Watermark", &globals::misc::watermark);
										if (globals::misc::watermark) {
											if (globals::misc::watermarkstuff == nullptr) {
												globals::misc::watermarkstuff = new std::vector<int>{ 1, 1, 0 };
											}
											std::vector<const char*> stuff = {"FPS", "Username", "Date"};
											gui->multi_dropdown("Watermark Options", *globals::misc::watermarkstuff, stuff.data(), stuff.size());
										}
										
										// Menu Key - use keybind widget
										// 0 = Hold, 1 = Toggle
										gui->label_keybind("Menu Key", &var->gui.menu_key, &var->gui.menu_key_mode);
										
										ImGui::Separator();
										
										// Workspace Viewer
										gui->checkbox("Workspace Viewer", &globals::visuals::workspace_viewer);
										if (globals::visuals::workspace_viewer) {
											gui->checkbox("Show Position", &globals::visuals::workspace_show_position);
											gui->checkbox("Show Size", &globals::visuals::workspace_show_size);
											gui->checkbox("Show Velocity", &globals::visuals::workspace_show_velocity);
											gui->checkbox("Transparency Modifier", &globals::visuals::workspace_transparency_modifier);
											if (globals::visuals::workspace_transparency_modifier) {
												gui->slider_float("Transparency", &globals::visuals::workspace_transparency, 0.0f, 1.0f);
											}
											gui->checkbox("CanCollide Modifier", &globals::visuals::workspace_cancollide_modifier);
											if (globals::visuals::workspace_cancollide_modifier) {
												gui->checkbox("CanCollide Value", &globals::visuals::workspace_cancollide_value);
											}
											gui->checkbox("Anchored Modifier", &globals::visuals::workspace_anchored_modifier);
											if (globals::visuals::workspace_anchored_modifier) {
												gui->checkbox("Anchored Value", &globals::visuals::workspace_anchored_value);
											}
										}
										
										ImGui::Spacing();
										
										// Discord button - opens discord.gg/aimquette
										if (gui->button("Discord", 1))
										{
											ShellExecuteA(NULL, "open", "https://discord.gg/aimquette", NULL, NULL, SW_SHOWNORMAL);
										}
										
										// Force Rescan button - use framework button
										if (gui->button("Force Rescan", 1))
										{
											force_rescan();
										}
										
										// Exit button - use framework button
										if (gui->button("Exit", 1))
										{
											globals::unattach = true;
											exit(0);
										}
									}
									gui->end_child();
								}
								gui->end_group();
								
								gui->sameline();
								
								// Right: Configs and Theme
								gui->begin_group();
								{
									// Calculate half height for Configs and Theme boxes
									float parent_height = GetWindowHeight();
									float content_padding = elements->content.padding.y * 2.f;
									float spacing = elements->content.spacing.y;
									float box_height = (parent_height - content_padding - spacing) / 2.0f;
									
									// Config box - top right
									ImGuiWindow* parent_group_config = GetCurrentWindow();
									gui->begin_child("Configs", 2, 1, ImVec2(0, box_height));
									{
										// Disable scrolling on Configs box
										ImGuiWindow* config_window = GetCurrentWindow();
										if (config_window)
										{
											config_window->Flags |= ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
											config_window->Scroll = ImVec2(0, 0);
											config_window->ScrollMax = ImVec2(0, 0);
										}
										
										ImVec2 availableSize = ImGui::GetContentRegionAvail();
										// Store the main window width before Config UI renders to prevent it from affecting the top bar
										ImGuiContext& g = *GImGui;
										ImGuiWindow* main_window = FindWindowByName("Aimquette ︳ discord.gg/aimquette");
										float saved_main_width = 0;
										if (main_window)
										{
											saved_main_width = main_window->Size.x;
										}
										float saved_window_width_var = var->window.width;
										
										g_config_manager.render_config_ui(availableSize.x, availableSize.y);
										
										// Restore the window width variable if Config UI changed it
										// This prevents the top bar from extending
										var->window.width = saved_window_width_var;
										if (main_window && saved_main_width > 0)
										{
											main_window->Size.x = saved_main_width;
											main_window->SizeFull.x = saved_main_width;
										}
									}
									
									// Get Config window reference before ending it
									ImGuiWindow* config_window_ref = GetCurrentWindow();
									float config_bottom = 0;
									if (config_window_ref && (config_window_ref->Flags & ImGuiWindowFlags_ChildWindow))
									{
										config_bottom = config_window_ref->Pos.y + config_window_ref->SizeFull.y;
									}
									gui->end_child();
									
									// Position Theme box directly under Config (no spacing)
									if (parent_group_config && config_bottom > 0)
									{
										parent_group_config->DC.CursorPos = ImVec2(parent_group_config->DC.CursorPos.x, config_bottom);
										parent_group_config->DC.CursorPosPrevLine = parent_group_config->DC.CursorPos;
									}
									
									// Theme box - bottom right
									gui->begin_child("Theme", 2, 1, ImVec2(0, box_height));
									{
										// Scrolling is enabled via begin_child function for "Theme" box
										
										// Theme color pickers - convert ImColor to float arrays
										static float accent_col[4] = {clr->accent.Value.x, clr->accent.Value.y, clr->accent.Value.z, clr->accent.Value.w};
										static float glow_col[4] = {clr->window.stroke.Value.x, clr->window.stroke.Value.y, clr->window.stroke.Value.z, clr->window.stroke.Value.w};
										static float outline_col[4] = {clr->window.stroke.Value.x, clr->window.stroke.Value.y, clr->window.stroke.Value.z, clr->window.stroke.Value.w};
										static float inline_col[4] = {clr->window.background_one.Value.x, clr->window.background_one.Value.y, clr->window.background_one.Value.z, clr->window.background_one.Value.w};
										static float text_col[4] = {clr->widgets.text.Value.x, clr->widgets.text.Value.y, clr->widgets.text.Value.z, clr->widgets.text.Value.w};
										static float text_outline_col[4] = {clr->widgets.text_inactive.Value.x, clr->widgets.text_inactive.Value.y, clr->widgets.text_inactive.Value.z, clr->widgets.text_inactive.Value.w};
										static float first_contrast_col[4] = {clr->window.background_one.Value.x, clr->window.background_one.Value.y, clr->window.background_one.Value.z, clr->window.background_one.Value.w};
										static float second_contrast_col[4] = {clr->window.background_two.Value.x, clr->window.background_two.Value.y, clr->window.background_two.Value.z, clr->window.background_two.Value.w};
										
										if (gui->label_color_edit("Accent Color", accent_col, false))
										{
											clr->accent.Value.x = accent_col[0];
											clr->accent.Value.y = accent_col[1];
											clr->accent.Value.z = accent_col[2];
										}
										if (gui->label_color_edit("Glow Color", glow_col, false))
										{
											clr->window.stroke.Value.x = glow_col[0];
											clr->window.stroke.Value.y = glow_col[1];
											clr->window.stroke.Value.z = glow_col[2];
										}
										if (gui->label_color_edit("Outline Color", outline_col, false))
										{
											clr->window.stroke.Value.x = outline_col[0];
											clr->window.stroke.Value.y = outline_col[1];
											clr->window.stroke.Value.z = outline_col[2];
										}
										if (gui->label_color_edit("Inline Color", inline_col, false))
										{
											clr->window.background_one.Value.x = inline_col[0];
											clr->window.background_one.Value.y = inline_col[1];
											clr->window.background_one.Value.z = inline_col[2];
										}
										if (gui->label_color_edit("Text Color", text_col, false))
										{
											clr->widgets.text.Value.x = text_col[0];
											clr->widgets.text.Value.y = text_col[1];
											clr->widgets.text.Value.z = text_col[2];
										}
										if (gui->label_color_edit("Text Outline Color", text_outline_col, false))
										{
											clr->widgets.text_inactive.Value.x = text_outline_col[0];
											clr->widgets.text_inactive.Value.y = text_outline_col[1];
											clr->widgets.text_inactive.Value.z = text_outline_col[2];
										}
										if (gui->label_color_edit("First Contrast", first_contrast_col, false))
										{
											clr->window.background_one.Value.x = first_contrast_col[0];
											clr->window.background_one.Value.y = first_contrast_col[1];
											clr->window.background_one.Value.z = first_contrast_col[2];
										}
										if (gui->label_color_edit("Second Contrast", second_contrast_col, false))
										{
											clr->window.background_two.Value.x = second_contrast_col[0];
											clr->window.background_two.Value.y = second_contrast_col[1];
											clr->window.background_two.Value.z = second_contrast_col[2];
										}
										
										ImGui::Spacing();
										
										// Theme name input field
										static char theme_name_buffer[256] = "";
										ImGui::SetNextItemWidth(-1);
										gui->text_field("Theme Name", theme_name_buffer, sizeof(theme_name_buffer));
										
										ImGui::Spacing();
										
										// Theme list dropdown for import
										auto theme_list = g_theme_manager.get_theme_list();
										static int selected_theme_idx = -1;
										if (!theme_list.empty()) {
											const char* theme_items[100] = { nullptr };
											for (size_t i = 0; i < theme_list.size() && i < 100; i++) {
												theme_items[i] = theme_list[i].c_str();
											}
											
											if (gui->dropdown("Select Theme", &selected_theme_idx, theme_items, static_cast<int>(theme_list.size()))) {
												if (selected_theme_idx >= 0 && selected_theme_idx < static_cast<int>(theme_list.size())) {
													strcpy_s(theme_name_buffer, sizeof(theme_name_buffer), theme_list[selected_theme_idx].c_str());
												}
											}
											ImGui::Spacing();
										}
										
										// Import Theme button - use framework button
										if (gui->button("Import Theme", 1))
										{
											if (strlen(theme_name_buffer) > 0) {
												g_theme_manager.refresh_theme_list();
												if (g_theme_manager.import_theme(std::string(theme_name_buffer))) {
													// Update static color picker arrays after import
													accent_col[0] = clr->accent.Value.x;
													accent_col[1] = clr->accent.Value.y;
													accent_col[2] = clr->accent.Value.z;
													accent_col[3] = clr->accent.Value.w;
													glow_col[0] = clr->window.stroke.Value.x;
													glow_col[1] = clr->window.stroke.Value.y;
													glow_col[2] = clr->window.stroke.Value.z;
													glow_col[3] = clr->window.stroke.Value.w;
													outline_col[0] = clr->window.stroke.Value.x;
													outline_col[1] = clr->window.stroke.Value.y;
													outline_col[2] = clr->window.stroke.Value.z;
													outline_col[3] = clr->window.stroke.Value.w;
													inline_col[0] = clr->window.background_one.Value.x;
													inline_col[1] = clr->window.background_one.Value.y;
													inline_col[2] = clr->window.background_one.Value.z;
													inline_col[3] = clr->window.background_one.Value.w;
													text_col[0] = clr->widgets.text.Value.x;
													text_col[1] = clr->widgets.text.Value.y;
													text_col[2] = clr->widgets.text.Value.z;
													text_col[3] = clr->widgets.text.Value.w;
													text_outline_col[0] = clr->widgets.text_inactive.Value.x;
													text_outline_col[1] = clr->widgets.text_inactive.Value.y;
													text_outline_col[2] = clr->widgets.text_inactive.Value.z;
													text_outline_col[3] = clr->widgets.text_inactive.Value.w;
													first_contrast_col[0] = clr->window.background_one.Value.x;
													first_contrast_col[1] = clr->window.background_one.Value.y;
													first_contrast_col[2] = clr->window.background_one.Value.z;
													first_contrast_col[3] = clr->window.background_one.Value.w;
													second_contrast_col[0] = clr->window.background_two.Value.x;
													second_contrast_col[1] = clr->window.background_two.Value.y;
													second_contrast_col[2] = clr->window.background_two.Value.z;
													second_contrast_col[3] = clr->window.background_two.Value.w;
												}
											} else {
												Notifications::Warning("Please enter a theme name!");
											}
										}
										
										// Export Theme button - use framework button
										if (gui->button("Export Theme", 1))
										{
											if (strlen(theme_name_buffer) > 0) {
												g_theme_manager.refresh_theme_list();
												g_theme_manager.export_theme(std::string(theme_name_buffer));
											} else {
												Notifications::Warning("Please enter a theme name!");
											}
										}
									}
									gui->end_child();
								}
								gui->end_group();
							}
						}
						
						gui->end_content();
					}
					
					// Footer at bottom - add spacing and text to extend menu (outside content area)
					ImGui::Spacing();
					ImGui::Spacing();
					
					// "Aimquette l .gg/aimquette" text at bottom
					PushStyleColor(ImGuiCol_Text, clr->accent.Value);
					Text("Aimquette l .gg/aimquette");
					PopStyleColor();
					
					ImGui::Spacing();
					
					// Get current date
					auto now = std::chrono::system_clock::now();
					auto time_t = std::chrono::system_clock::to_time_t(now);
					struct tm local_time;
					localtime_s(&local_time, &time_t);
					
					char date_str[64];
					std::strftime(date_str, sizeof(date_str), "%A, %d %B %Y", &local_time);
					
					// Footer text - left side
					PushStyleColor(ImGuiCol_Text, clr->widgets.text_inactive.Value);
					Text("Aimquette | Custom Build");
					
					// Footer text - right side (aligned to right)
					SameLine(0, GetContentRegionAvail().x - CalcTextSize(date_str).x);
					Text(date_str);
					PopStyleColor();
					
					// Extended bottom section with TESTING text
					ImGui::Spacing();
					ImGui::Spacing();
					
					// Push content to bottom using Dummy
					ImVec2 avail = GetContentRegionAvail();
					if (avail.y > 0)
						Dummy(ImVec2(0, avail.y - GetFrameHeightWithSpacing() * 2));
					
					// Center the TESTING text at the bottom
					PushStyleColor(ImGuiCol_Text, clr->widgets.text.Value);
					ImVec2 text_size = CalcTextSize("TESTING");
					float available_width = GetContentRegionAvail().x;
					SetCursorPosX((available_width - text_size.x) * 0.5f);
					Text("TESTING");
					PopStyleColor();
					
					// Add extra spacing at the very bottom to extend the menu
					ImGui::Spacing();

				}
				gui->end();
			}

			if (var->gui.current_section[4])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("code", nullptr, var->window.flags);
				{
					draw->window_decorations();

				}
				gui->end();
			}

			if (var->gui.current_section[5])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("style", nullptr, var->window.flags);
				{
					draw->window_decorations();

					{
						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
						gui->begin_content();
						{
							gui->begin_child("Theme", 1, 2);
							{
								static float menu_accent[4] = { clr->accent.Value.x, clr->accent.Value.y, clr->accent.Value.z, 1.f };
								if (gui->label_color_edit("Menu Accent", menu_accent, false))
								{
									clr->accent.Value.x = menu_accent[0];
									clr->accent.Value.y = menu_accent[1];
									clr->accent.Value.z = menu_accent[2];
								}

								static float contrast_one[4] = { clr->window.background_one.Value.x, clr->window.background_one.Value.y, clr->window.background_one.Value.z, 1.f };
								if (gui->label_color_edit("Contrast One", contrast_one, false))
								{
									clr->window.background_one.Value.x = contrast_one[0];
									clr->window.background_one.Value.y = contrast_one[1];
									clr->window.background_one.Value.z = contrast_one[2];
								}

								static float contrast_two[4] = { clr->window.background_two.Value.x, clr->window.background_two.Value.y, clr->window.background_two.Value.z, 1.f };
								if (gui->label_color_edit("Contrast Two", contrast_two, false))
								{
									clr->window.background_two.Value.x = contrast_two[0];
									clr->window.background_two.Value.y = contrast_two[1];
									clr->window.background_two.Value.z = contrast_two[2];
								}

								static float inline_c[4] = { clr->window.stroke.Value.x, clr->window.stroke.Value.y, clr->window.stroke.Value.z, 1.f };
								if (gui->label_color_edit("Inline", inline_c, false))
								{
									clr->window.stroke.Value.x = inline_c[0];
									clr->window.stroke.Value.y = inline_c[1];
									clr->window.stroke.Value.z = inline_c[2];
								}

								static float outline_c[4] = { clr->widgets.stroke_two.Value.x, clr->widgets.stroke_two.Value.y, clr->widgets.stroke_two.Value.z, 1.f };
								if (gui->label_color_edit("Outline", outline_c, false))
								{
									clr->widgets.stroke_two.Value.x = outline_c[0];
									clr->widgets.stroke_two.Value.y = outline_c[1];
									clr->widgets.stroke_two.Value.z = outline_c[2];
								}

								static float text_active[4] = { clr->widgets.text.Value.x, clr->widgets.text.Value.y, clr->widgets.text.Value.z, 1.f };
								if (gui->label_color_edit("Text Active", text_active, false))
								{
									clr->widgets.text.Value.x = text_active[0];
									clr->widgets.text.Value.y = text_active[1];
									clr->widgets.text.Value.z = text_active[2];
								}

								static float text_inactive[4] = { clr->widgets.text_inactive.Value.x, clr->widgets.text_inactive.Value.y, clr->widgets.text_inactive.Value.z, 1.f };
								if (gui->label_color_edit("Text Inctive", text_inactive, false))
								{
									clr->widgets.text_inactive.Value.x = text_inactive[0];
									clr->widgets.text_inactive.Value.y = text_inactive[1];
									clr->widgets.text_inactive.Value.z = text_inactive[2];
								}
							}
							gui->end_child();

							ImGui::Spacing();
							ImGui::Spacing();

							gui->begin_child("Style", 1, 2);
							{
								gui->checkbox("Hover Highlight", &var->window.hover_hightlight);

								static bool window_glow = false;
								gui->checkbox("Window Glow", &window_glow);
								if (window_glow)
								{
									gui->slider_float("Glow Thickness", &var->window.shadow_size, 1, 100);
									gui->slider_float("Glow Alpha", &var->window.shadow_alpha, 0, 1);
								}
								else
									var->window.shadow_size = 0;
							}
							gui->end_child();
						}
						gui->end_content();
					}

				}
				gui->end();
			}

			if (var->gui.current_section[6])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("cloud", nullptr, var->window.flags);
				{
					draw->window_decorations();

				}
				gui->end();
			}
		}


	gui->pop_style_var();
}