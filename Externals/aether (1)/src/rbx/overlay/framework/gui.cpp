#include "settings/functions.h"
#include <Windows.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <unordered_map>
#include "../../../globals/globals.hpp"
#include "../../cheat/features/rage/variables.h"
std::string get_current_time_date()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_time = std::chrono::system_clock::to_time_t(now);
	char buf[64];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
	return std::string(buf);
}
bool IsFirstTime(const char* id) {
	static std::unordered_map<std::string, bool> seen;
	auto& flag = seen[id];
	if (!flag) {
		flag = true;
		return true;
	}
	return false;
}

auto checkbox_with_color = [&](const char* label, bool* toggle, float* color, const char* color_id) {
	ImVec2 pos = ImGui::GetCursorPos();
	gui->checkbox(label, toggle);
	if (color) {
		ImGui::SameLine(140.0f);
		ImGui::SetCursorPosY(pos.y);
		gui->color_edit(color_id, color);

	}
	};
auto checkbox_with_colordouble = [&](const char* label, bool* toggle, float* color, float* color2, const char* color_id, const char* color_id2) {
	ImVec2 pos = ImGui::GetCursorPos();
	gui->checkbox(label, toggle);
	if (color) {
		ImGui::SameLine(140.0f);
		ImGui::SetCursorPosY(pos.y);
		gui->color_edit(color_id, color);
		ImGui::SameLine(140.0f);
		ImGui::SetCursorPosY(pos.y);
		gui->color_edit(color_id, color2);
	}
	};
bool keybind(CKeybind* bind, ImVec2 size, bool text_label, ImGuiButtonFlags flags = 0)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;

	const ImGuiID id = window->GetID(bind->get_name().c_str());
	const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + size);

	ImGui::ItemSize(rect, style.FramePadding.y);
	if (!ImGui::ItemAdd(rect, id))
		return false;

	bool hovered = ImGui::ItemHoverable(rect, id, 0);
	bool active = g.ActiveId == id;
	bool value_changed = false;

	const char* display_name = bind->key == 0 ? "none" : bind->get_key_name().c_str();

	if (active)
		display_name = "-";

	draw->fade_rect_filled(window->DrawList, rect.Min + ImVec2(2, 2), rect.Max - ImVec2(2, 2),
		draw->get_clr(clr->window.background_two), draw->get_clr(clr->window.background_one), fade_direction::vertically);

	draw->rect(window->DrawList, rect.Min + ImVec2(1, 1), rect.Max - ImVec2(1, 1), draw->get_clr(clr->window.stroke));
	draw->text_clipped_outline(window->DrawList, var->font.tahoma, rect.Min - ImVec2(0, 1), rect.Max - ImVec2(0, 1),
		draw->get_clr(clr->widgets.text), display_name, NULL, NULL, ImVec2(0.5f, 0.5f));
	draw->rect(window->DrawList, rect.Min, rect.Max, draw->get_clr(var->window.hover_hightlight && hovered ? clr->accent : clr->widgets.stroke_two));

	if (hovered && io.MouseClicked[0]) {
		if (g.ActiveId != id) {
			memset(io.MouseDown, 0, sizeof(io.MouseDown));
			memset(io.KeysDown, 0, sizeof(io.KeysDown));
			bind->key = 0;
		}
		ImGui::SetActiveID(id, window);
		ImGui::FocusWindow(window);
	}
	else if (io.MouseClicked[0] && g.ActiveId == id) {
		ImGui::ClearActiveID();
	}

	if (active) {
		if (bind->set_key()) {
			value_changed = true;
			ImGui::ClearActiveID();
		}
	}

	// Right-click popup for keybind type
	const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
	bool popup_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
	if (hovered && g.IO.MouseClicked[1] && !popup_open) {
		OpenPopupEx(popup_id, ImGuiPopupFlags_None);
		popup_open = true;
	}

	if (popup_open) {
		gui->push_style_var(ImGuiStyleVar_PopupBorderSize, 1.f);
		gui->push_style_color(ImGuiCol_Border, draw->get_clr(clr->window.stroke));
		gui->push_style_color(ImGuiCol_PopupBg, draw->get_clr(clr->window.background_one));
		gui->push_style_var(ImGuiStyleVar_ItemSpacing, ImVec2(3, 3));
		gui->push_style_var(ImGuiStyleVar_WindowPadding, ImVec2(3, 3));
		gui->set_next_window_pos(ImVec2(rect.Min.x, rect.Max.y + 3));

		if (BeginPopupEx(popup_id, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding)) {

			if (gui->selectable_ex("Hold", bind->type == CKeybind::HOLD, ImVec2(60, 15)))
				bind->type = CKeybind::HOLD;
			if (gui->selectable_ex("Toggle", bind->type == CKeybind::TOGGLE, ImVec2(60, 15)))
				bind->type = CKeybind::TOGGLE;
			if (gui->selectable_ex("Always", bind->type == CKeybind::ALWAYS, ImVec2(60, 15)))
				bind->type = CKeybind::ALWAYS;

			EndPopup();
		}

		gui->pop_style_var(3);
		gui->pop_style_color(2);
	}

	return value_changed;
}
static bool enable_lighting_editor = true;

static float brightness = 2.0f;
static float fog_start = 0.0f;
static float fog_end = 100000.0f;
static float bloom_intensity = 0.0f;
static float bloom_size = 0.0f;
static float bloom_threshold = 0.0f;
static float sunrays_intensity = 0.0f;
static float sunrays_spread = 0.0f;

static RBX::Vector3 ambience = { 0.0f, 0.0f, 0.0f };
static RBX::Vector3 shift_top = { 0.0f, 0.0f, 0.0f };
static RBX::Vector3 shift_bottom = { 0.0f, 0.0f, 0.0f };
static RBX::Vector3 color3 = { 0.0f, 0.0f, 0.0f };
bool network_spoofing = false;
bool network_spoofingB = false;
bool network_spoofingA = false;
CKeybind network_spoofing_bind = CKeybind("yo");
bool Keybind(CKeybind* Keybind, const ImVec2& SizeArg = ImVec2(0, 0), bool Clicked = false, ImGuiButtonFlags Flags = 0)
{
	ImGui::PushFont(var->font.tahoma);
	ImGuiWindow* Window = ImGui::GetCurrentWindow();
	if (Window->SkipItems)
		return false;

	ImGuiContext& G = *GImGui;
	const ImGuiStyle& Style = G.Style;
	const ImGuiID Id = Window->GetID(Keybind->get_name().c_str());
	const ImVec2 LabelSize = ImGui::CalcTextSize(Keybind->get_name().c_str(), NULL, true);

	ImVec2 Pos = Window->DC.CursorPos;
	if ((Flags & ImGuiButtonFlags_AlignTextBaseLine) &&
		Style.FramePadding.y < Window->DC.CurrLineTextBaseOffset)
	{
		Pos.y += Window->DC.CurrLineTextBaseOffset - Style.FramePadding.y;
	}

	ImVec2 Size = ImGui::CalcItemSize(
		SizeArg, LabelSize.x + Style.FramePadding.x * 2.0f, LabelSize.y + Style.FramePadding.y * 2.0f);

	const ImRect Bb(Pos, Pos + Size);
	ImGui::ItemSize(Size, Style.FramePadding.y);
	if (!ImGui::ItemAdd(Bb, Id))
		return false;

	if (G.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
		Flags |= ImGuiButtonFlags_Repeat;

	bool Hovered, Held;
	bool Pressed = ImGui::ButtonBehavior(Bb, Id, &Hovered, &Held, Flags);

	bool ValueChanged = false;
	int Key = Keybind->key;

	std::string Name = Keybind->get_key_name();

	if (Keybind->waiting_for_input)
		Name = "waiting";

	ImGuiIO& IO = ImGui::GetIO();

	// Handle mouse clicks on the keybind button
	if (IO.MouseClicked[0] && Hovered)
	{
		if (G.ActiveId == Id)
		{
			Keybind->waiting_for_input = true;
		}
	}
	else if (IO.MouseClicked[1] && Hovered)
	{
		ImGui::OpenPopup(Keybind->get_name().c_str());
	}
	else if (IO.MouseClicked[0] && !Hovered)
	{
		if (G.ActiveId == Id)
			ImGui::ClearActiveID();
	}

	// Handle key input when waiting for input
	if (Keybind->waiting_for_input)
	{
		if (IO.MouseClicked[0] && !Hovered)
		{
			Keybind->key = VK_LBUTTON;
			ImGui::ClearActiveID();
			Keybind->waiting_for_input = false;
		}
		else
		{
			if (Keybind->set_key())
			{
				ImGui::ClearActiveID();
				Keybind->waiting_for_input = false;
			}
		}
	}

	ImVec4 TextColor = ImLerp(ImVec4(201 / 255.f, 204 / 255.f, 210 / 255.f, 1.f), ImVec4(1.f, 1.f, 1.f, 1.f), 1.f);

	Window->DrawList->AddRectFilled(Bb.Min, Bb.Max, ImColor(33 / 255.0f, 33 / 255.0f, 33 / 255.0f, 1.f), 2.f);

	Window->DrawList->AddText(
		Bb.Min +
		ImVec2(
			SizeArg.x / 2 - ImGui::CalcTextSize(Name.c_str()).x / 2,
			SizeArg.y / 2 - ImGui::CalcTextSize(Name.c_str()).y / 2),
		ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_Text)),
		Name.c_str());

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar;

	if (ImGui::BeginPopup(Keybind->get_name().c_str(), 0))
	{
		ImGui::BeginGroup();
		{
			if (ImGui::Selectable("Hold", Keybind->type == CKeybind::HOLD))
				Keybind->type = CKeybind::HOLD;
			if (ImGui::Selectable("Always", Keybind->type == CKeybind::ALWAYS))
				Keybind->type = CKeybind::ALWAYS;
			if (ImGui::Selectable("Toggle", Keybind->type == CKeybind::TOGGLE))
				Keybind->type = CKeybind::TOGGLE;
		}
		ImGui::EndGroup();
		ImGui::EndPopup();
	}

	ImGui::PopFont();
	return Pressed;
}



void c_gui::render()
{


	var->gui.menu_alpha = ImClamp(var->gui.menu_alpha + (gui->fixed_speed(8.f) * (var->gui.menu_opened ? 1.f : -1.f)), 0.f, 1.f);
	if (!var->gui.menu_opened && var->gui.menu_alpha <= 0.f) {
		globals::menufinishedfading = true;
	}

	else if (var->gui.menu_opened && var->gui.menu_alpha >= 1.f) {
		globals::menufinishedfading = false;
	}
	if (var->gui.menu_alpha <= 0.01f)
		return;


	time_t now = time(0);
	tm* ltm = localtime(&now);
	char DateStr[32];
	strftime(DateStr, sizeof(DateStr), "%A %d %b.", ltm);

	std::string WindowTitle = std::string("Aether.gg - ") + DateStr;

	static int subtabs;

	gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2 - var->window.width / 2, 20));
	gui->set_next_window_size(ImVec2(var->window.width, elements->section.size.y + var->window.spacing.y * 2 - 1));
	gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.menu_alpha);
	gui->begin("Atlanta", nullptr, var->window.main_flags);
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
			draw->rect(GetBackgroundDrawList(), pos - ImVec2(1, 1), pos + size + ImVec2(1, 1), draw->get_clr({ 0, 0, 0, 0.5f }));
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

		{
			if (var->gui.current_section[0])
			{
				if (IsFirstTime("main_menu_resize"))
					gui->set_next_window_size(ImVec2(500, 500));
				gui->begin(WindowTitle.c_str(), nullptr, var->window.flags);
				{
					draw->window_decorations();

					{

						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
						gui->begin_group();
						{
							gui->sub_section("Legit", 0, subtabs, 4);
							gui->sub_section("Players", 1, subtabs, 4);
							gui->sub_section("Visuals", 2, subtabs, 4);
							gui->sub_section("Misc", 3, subtabs, 4);
							//gui->sub_section("Settings", 4, subtabs, 5);
						}
						gui->end_group();


						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + elements->section.height - 1));
						if (subtabs == 0) {
							gui->begin_content();
							{
								gui->begin_group();
								{
									gui->begin_child("Aim Assist", 2, 1);
									{
										{
											static bool enabled = globals::aimbot;

											static int mode = 0;
											ImVec2 pos = ImGui::GetCursorPos();

											gui->checkbox("Enable", &enabled);
											gui->sameline();
											gui->label_keybind("wdwwd", &globals::aimbot_bind);




											static bool sticky = globals::aimbot_sticky;
											gui->checkbox("Sticky Aim", &sticky);
											globals::aimbot_sticky = sticky;

											static bool prediction = globals::camera_prediction;
											gui->checkbox("Prediction", &prediction);
											globals::camera_prediction = prediction;

											static bool closest = globals::closest_part;
											gui->checkbox("Closest Part Enumation", &closest);
											globals::closest_part = closest;

											static bool resolver = globals::resolver;
											gui->checkbox("AA Resolver", &resolver);
											globals::resolver = resolver;

											static int aimbot_type = globals::aimbot_type;
											const char* aim_options[] = { "Memory", "Mouse" };
											gui->dropdown("Aimbot Type", &aimbot_type, aim_options, IM_ARRAYSIZE(aim_options));
											checkbox_with_color("Enable Dynamic Fov", &globals::draw_triggerbot_fov, globals::FovColor, "##FovColor");

											globals::aimbot_type = aimbot_type;
											gui->checkbox("Use Shake", &globals::shake);
											gui->slider_float("Shake X", &globals::shake_x, 1.0f, 50.0f);
											gui->slider_float("Shake Y", &globals::shake_y, 1.0f, 50.0f);
											gui->checkbox("Use Deadzone", &globals::usedeadzone);
											gui->slider_float("Deadzone X", &globals::deadzoneX, 1.0f, 50.0f);
											gui->slider_float("Deadzone Y", &globals::deadzoneY, 1.0f, 50.0f);
											if (aimbot_type == 0)
											{
												static float pred_x = globals::camera_prediction_x;
												static float pred_y = globals::camera_prediction_y;
												static float pred_z = globals::camera_prediction_z;
												static float smooth = globals::smoothness_camera;

												gui->slider_float("Prediction X", &pred_x, 1.0f, 50.0f);
												gui->slider_float("Prediction Y", &pred_y, 1.0f, 50.0f);
												gui->slider_float("Prediction Z", &pred_z, 1.0f, 50.0f);
												gui->slider_float("Smoothing", &smooth, 0.0f, 150.0f);

												globals::camera_prediction_x = pred_x;
												globals::camera_prediction_y = pred_y;
												globals::camera_prediction_z = pred_z;
												globals::smoothness_camera = smooth;

												const char* easing_names[] = {
													"Linear", "Ease In Out (Cosine)", "Ease In (Quadratic)", "Ease In Cubic",
													"Ease In Quartic", "Ease In Quintic", "Ease Out Exponential",
													"Ease Out Circular", "Ease In Back", "Bounce"
												};

												static int easing = globals::aimbot_easing_style;
												gui->dropdown("Smoothing Type", &easing, easing_names, IM_ARRAYSIZE(easing_names));
												globals::aimbot_easing_style = easing;
											}
											else if (aimbot_type == 1)
											{
												static float pred_x = globals::free_aim_prediction_x;
												static float pred_y = globals::free_aim_prediction_y;
												static float smooth = globals::mouse_smoothness;
												static float sens = globals::mouse_sensitivity;

												gui->slider_float("Prediction Y", &pred_y, 1.0f, 50.0f);
												gui->slider_float("Prediction X", &pred_x, 1.0f, 50.0f);
												gui->slider_float("Prediction Y", &pred_y, 1.0f, 50.0f);
												gui->slider_float("Smoothing", &smooth, 1.0f, 150.0f);
												gui->slider_float("Sensitivity", &sens, 1.0f, 50.0f);

												globals::free_aim_prediction_x = pred_x;
												globals::free_aim_prediction_y = pred_y;
												globals::mouse_smoothness = smooth;
												globals::mouse_sensitivity = sens;
											}

											static int hitpart = globals::aimbot_part;
											const char* hit_parts[] = { "Head", "Neck", "UpperTorso", "LowerTorso" };
											gui->dropdown("Hitpart", &hitpart, hit_parts, IM_ARRAYSIZE(hit_parts));
											globals::aimbot_part = hitpart;

											std::vector<std::string> check_items = { "Knocked Check", "Dead Check", "Grabbed Check", "Team Check" };
											gui->multi_dropdown("Aimbot Checks", globals::aimbot_checks, check_items, false);
										}

									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Bullet Assist", 2, 2);
									{
										static bool enabled = globals::free_aim;
										ImVec2 pos = ImGui::GetCursorPos();

										static int mode = 0;
										gui->checkbox("Enable", &enabled);
										gui->sameline();
										gui->label_keybind("wdwwd", &globals::free_aim_bind);
										globals::free_aim = enabled;
										checkbox_with_color("Enable Dynamic Fov", &globals::free_aim_draw_fov, globals::FovColor, "##FovColor");

										gui->slider_float("Fov Size", &globals::free_aim_fov, 0.1f, 500.0f);


										static bool sticky = globals::free_aim_sticky;
										gui->checkbox("Sticky Aim", &sticky);
										globals::free_aim_sticky = sticky;

										static bool prediction = globals::free_aim_prediction;
										gui->checkbox("Prediction", &prediction);
										globals::free_aim_prediction = prediction;

										static bool closest = globals::free_aim_closest_part;
										gui->checkbox("Closest Part Enumation", &closest);
										globals::free_aim_closest_part = closest;

										static bool link_target = globals::linkTarget;
										gui->checkbox("Link To Aimbot Target", &link_target);
										globals::linkTarget = link_target;

										gui->checkbox("Draw Fov", &globals::free_aim_draw_fov);
										static bool in_fov = globals::free_aim_is_in_fov;
										gui->checkbox("Out Of Fov Check", &in_fov);
										globals::free_aim_is_in_fov = in_fov;

										static bool resolver = globals::free_aim_resolver;
										gui->checkbox("AA Resolver", &resolver);
										globals::free_aim_resolver = resolver;

										const char* closest_type_opts[] = { "Closest Part", "Closest Point" };
										static int closest_type = globals::font_type;
										gui->dropdown("Closest Type", &closest_type, closest_type_opts, IM_ARRAYSIZE(closest_type_opts));
										globals::font_type = closest_type;

										static float pred_x = globals::free_aim_prediction_x;
										static float pred_y = globals::free_aim_prediction_y;
										gui->slider_float("Hitchance", &globals::hit_chance, 0.05f, 100.0f);
										gui->slider_float("Prediction X", &pred_x, 1.0f, 50.0f);
										gui->slider_float("Prediction Y", &pred_y, 1.0f, 50.0f);
										globals::free_aim_prediction_x = pred_x;
										globals::free_aim_prediction_y = pred_y;

										const char* hitparts[] = { "Head", "Neck", "UpperTorso", "LowerTorso" };
										static int hit = globals::free_aim_part;
										gui->dropdown("Hitpart", &hit, hitparts, IM_ARRAYSIZE(hitparts));
										globals::free_aim_part = hit;

										std::vector<std::string> checks = { "Knocked Check", "Dead Check", "Grabbed Check", "Team Check" };

										gui->multi_dropdown("Redirection Checks", globals::silent_aim_checks, checks, false);
									}
									gui->end_child();

									gui->begin_child("Trigger Assist", 2, 2);
									{
										ImVec2 pos = ImGui::GetCursorPos();

										static bool enabled = globals::triggerbot_enabled;

										gui->checkbox("Enable", &enabled);
										gui->sameline();
										gui->label_keybind("", &globals::triggerBind);

										gui->slider_int("Trigger Delay", &globals::triggerbot_delay, 0, 100);

										static float pred_x = globals::trigger_bot_prediction_x;
										static float pred_y = globals::trigger_bot_prediction_y;
										gui->slider_float("Predict X", &pred_x, 0.0f, 1.5f);
										gui->slider_float("Predict Y", &pred_y, 0.0f, 1.5f);
										globals::trigger_bot_prediction_x = pred_x;
										globals::trigger_bot_prediction_y = pred_y;

										static bool knife_check = globals::knife_CHeck;
										gui->checkbox("Knife Check", &knife_check);
										globals::knife_CHeck = knife_check;

									}
									gui->end_child();
								}
								gui->end_group();
							}
							gui->end_content();
						}
						if (subtabs == 1) {
							gui->begin_content();
							{
								gui->begin_group();
								{
									gui->begin_child("Players", 2.0f, 1.6f);
									{
										static bool master_switch = globals::esp;
										gui->checkbox("Enabled", &master_switch);
										globals::esp = master_switch;

										checkbox_with_color("Bounding Box", &globals::box_esp, globals::BoxOutlineColor, "##BoxColor");
										checkbox_with_color("Box Glow", &globals::boxGlow, globals::color_5, "##BoxGlowColor");
										checkbox_with_color("Fill", &globals::fill_box, globals::color_5, "##FillColor");

										gui->checkbox("Box image", &globals::fill_box_render);

										const char* image_types[] = { "johnpork", "abyss", "Lebron", "Egirl", "Fulcrum", "OsamaSon", "Hunter" };
										gui->dropdown("Image types", &globals::image_type, image_types, IM_ARRAYSIZE(image_types));
										checkbox_with_color("Name", &globals::name_esp, globals::name_color, "##NameColor");

										checkbox_with_color("Tool", &globals::tool_esp, globals::color_2, "##ToolColor");
										checkbox_with_color("Distance", &globals::distance_esp, globals::distance_color, "##DistanceColor");

										gui->checkbox("Flags", &globals::flag_esp);
										checkbox_with_color("Team", &globals::TeamVisual, globals::teamNameColor, "##TeamColor");
										checkbox_with_color("Healthbar", &globals::health_bar, globals::healthBar_Color, "##HealthColor");

										gui->checkbox("Healthbar Glow", &globals::enable_health_glow);
										gui->checkbox("Health Bar Text", &globals::health_bar_text);
										gui->slider_float("HealthBar Text Y Offset", &globals::health_y, -50.0f, 50.0f);
										gui->slider_float("HealthBar Text X Offset", &globals::health_x, -50.0f, 50.0f);

										gui->checkbox("Armor Bar", &globals::shield_bar);
										gui->checkbox("Flame Bar", &globals::flame_bar);

										checkbox_with_color("HighLight", &globals::highlight, globals::HighLight_color, "##HighlightColor");
										gui->checkbox("HighLight Glow", &globals::chamsglow);
										gui->slider_float("HighLight Glow Size", &globals::chamsGlowSize, 0.0f, 150.0f);

										checkbox_with_color("Chams", &globals::chams, globals::color_4, "##ChamsColor");
										gui->checkbox("WireFrame", &globals::WireFrameChams);
										gui->checkbox("Frames", &globals::FrameChams);
										gui->checkbox("Player Glow", &globals::PlayerGlow);

										checkbox_with_color("Skeleton", &globals::skeleton_esp, globals::skeleton_Color, "##SkeletonColor");
										gui->slider_float("Skeleton Thickness", &globals::skeletonThickness, 0.1f, 50.0f);
										gui->checkbox("Head Dot", &globals::HeadDotEsp);
										gui->checkbox("Arrows", &globals::dropped_items);

										checkbox_with_color("Tracers", &globals::tracer_esp, globals::TracerColor, "##TracerColor");
										gui->slider_float("Tracer Thickness", &globals::tracer_thickness, 0.5f, 10.0f);
										const char* tracer_origins[] = { "Mouse", "Bottom Center", "Screen Center", "Top Left" };
										gui->dropdown("Tracer Origin", &globals::TracerOrigin, tracer_origins, IM_ARRAYSIZE(tracer_origins));
									}
									gui->end_child();

									gui->begin_child("Other", 2, 2); 
									{
										checkbox_with_color("Crosshair", &globals::crosshair, globals::crosshair_color, "##CrosshairColor");
										gui->checkbox("Crosshair Text", &globals::vehicle_esp);
										gui->slider_int("Crosshair Size", &globals::crosshair_size, 1, 130);
										gui->slider_int("Crosshair Lerp Speed", &globals::crosshair_speed, 1, 50);
										gui->slider_int("Crosshair Gap", &globals::crosshair_gap, 1, 10);
										gui->checkbox("Crosshair Rotatation", &globals::cashier_esp);
										gui->slider_float("Crosshair Rotation Speed", &globals::crosshair_spinspeed, 0.1f, 10.0f);
										gui->checkbox("Crosshair Pulse", &globals::pulse_effect);
										const char* crosshair_types[] = { "Mouse", "Bottom", "Center", "Target" };
										gui->dropdown("Crosshair Origin", &globals::crosshair_origin, crosshair_types, IM_ARRAYSIZE(crosshair_types));
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Options", 2, 1);
									{
										gui->checkbox("Anti Aliasing", &globals::autoparry);
										gui->checkbox("Allow Local Player", &globals::localplayercheck);
										gui->slider_float("Glow Size", &globals::glow_size, 0.0f, 150.0f);
										gui->slider_float("Alpha", &globals::alpha, 0.0f, 1.0f, "vs");
										gui->checkbox("Off Screen Check", &globals::offscreen_Check);
										gui->checkbox("Target Only", &globals::onlytarget);
										gui->checkbox("Distance Limit", &globals::distancecheck);
										gui->slider_float("Max Distance", &globals::max_render_distance, 0.0f, 10000.0f);

										std::vector<std::string> visuals_checks = {
											"Knocked Check", "Dead Check", "Grabbed Check", "Team Check"
										};
										gui->multi_dropdown("Visuals Checks", globals::esp_checks, visuals_checks, false);

										const char* font_type[] = { "Visitor", "ProggyClean", "Tomaha", "Verdana", "Arial" };
										gui->dropdown("Font", &globals::fonttype, font_type, IM_ARRAYSIZE(font_type));

										const char* box_types[] = { "2D Static Box", "3D Box", "2D Dynamic Box", "2d Corner Static" };
										gui->dropdown("Box Type", &globals::box_type, box_types, IM_ARRAYSIZE(box_types));

										const char* bar_sides[] = { "Left", "Right" };
										gui->dropdown("Bar Side", &globals::HealthBarSide, bar_sides, IM_ARRAYSIZE(bar_sides));
										gui->checkbox("Health Based Bar Color", &globals::HealthBasedColor);

										std::vector<bool> temp_vars = {
											globals::crosshair_outline,
											globals::soccer_ball_esp,
											globals::FrameOutline,
											globals::skeletonOutline,
											globals::tool_outline,
											globals::name_outline,
											globals::distance_outline,
											globals::jail_esp,
											globals::cash_esp
										};

										std::vector<std::string> visual_labels = {
											"Crosshair",
											"Tracer",
											"Frame",
											"Skeleton",
											"Tool",
											"Name",
											"Distance",
											"Box",
											"Healthbar"
										};

										gui->multi_dropdown("Render Outline", temp_vars, visual_labels, false);

										globals::crosshair_outline = temp_vars[0];
										globals::soccer_ball_esp = temp_vars[1];
										globals::FrameOutline = temp_vars[2];
										globals::skeletonOutline = temp_vars[3];
										globals::tool_outline = temp_vars[4];
										globals::name_outline = temp_vars[5];
										globals::distance_outline = temp_vars[6];
										globals::jail_esp = temp_vars[7];
										globals::cash_esp = temp_vars[8];
									}
									gui->end_child();
								}
								gui->end_group();
							}
							gui->end_content();
						}

			
						RBX::Instance lighting = globals::game.FindFirstChildOfClass("Lighting");
						if (subtabs == 3) {
							gui->begin_content();
							{
								gui->begin_group();
								{
									static bool auto_set_spawn = false;
									static bool tp_spawn_knock = false;
									gui->begin_child("Movment", 2, 1);
									{

										gui->checkbox("Anti Stomp", &globals::antistomp);
										gui->checkbox("Auto Reload", &globals::autoreload);
										gui->checkbox("Rapid Fire", &globals::rapidfire);
										gui->checkbox("Fly", &globals::fly);
										gui->sameline();
										gui->label_keybind("Fly Bind", &globals::fly_bind);
										gui->slider_int("Fly Speed", &globals::fly_speed, 0, 50);



										gui->checkbox("Noclip", &globals::noclip);
										gui->sameline();
										gui->label_keybind("Noclip Bind", &globals::noclipbind);

										gui->checkbox("hipheight", &globals::cashier_esp); // ..??
										gui->sameline();
										gui->label_keybind("hipheight", &globals::HipHeight_Bind);
										gui->slider_float("hipheightlength", &globals::HipHeight, 0.f, 50.f);

										gui->checkbox("walkspeed", &globals::debug_info);
										gui->sameline();
										gui->label_keybind("WalkSpeed Bind", &globals::WalkSpeed_Bind);
										gui->slider_float("WalkSpeed Amount", &globals::walkspeed_amount, 0.f, 50.f);
						
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									

									gui->begin_child("Manipulation", 2, 2);
									{
										gui->checkbox("Enable Network Spoofing", &RageGlobals::network_spoofing);
										gui->sameline();
										gui->label_keybind("Network Spoofing Bind", &RageGlobals::network_spoofing_bind);
										gui->checkbox("Enable Network Spoofing On Join", &RageGlobals::network_spoofing_on_join);
										gui->checkbox("Enable Network Spoofing On Leave", &RageGlobals::network_spoofing_on_leave);

									//	gui->separator_text("Desync");
										gui->checkbox("Enable Desync", &RageGlobals::desync_enabled);
										gui->sameline();
										gui->label_keybind("Toggle Desync", &RageGlobals::desync_key);
										gui->slider_float("Desync Strength", &RageGlobals::desync_strength, 0.1f, 10.0f);
										gui->slider_float("Desync Delay", &RageGlobals::desync_delay, 0.01f, 5.0f);
										gui->checkbox("Desync Jitter Mode", &RageGlobals::desync_jitter);
										gui->checkbox("Auto Disable On Knock", &RageGlobals::desync_disable_on_knock);
										gui->checkbox("Auto Disable On Grab", &RageGlobals::desync_disable_on_grab);
										gui->checkbox("Auto Disable On Rag", &RageGlobals::desync_disable_on_ragdoll);
										gui->checkbox("Loop Desync", &RageGlobals::desync_loop);
									}
									gui->end_child();
							

									gui->begin_child("Miscellaneous", 2, 2);
									{
										
										gui->checkbox("Enable Waypoint On Keybind", &globals::waypoint_enabled);
										gui->sameline();
										gui->label_keybindB("Set Waypoint", &globals::keybind_set);
										gui->label_keybindB("Teleport to Waypoint", &globals::keybind_teleport);
										gui->label_keybindB("Clear Waypoint", &globals::keybind_clear);
										gui->checkbox("Waypoint On Spawn", &globals::tp_on_respawn);
									
									}
									gui->end_child();
								}
								gui->end_group();
							}
							gui->end_content();
						}
						if (subtabs == 2) {
							gui->begin_content();
							{
								gui->begin_group();
								{
									gui->begin_child("Lighting", 2, 1);
									{


										if (gui->slider_float("Brightness", &brightness, 0.0f, 10.0f))
											lighting.Setbrightness((int)brightness);

										if (gui->slider_float("Fog Start", &fog_start, 0.0f, 1000.0f))
											lighting.SetFogStart((int)fog_start);

										if (gui->slider_float("Fog End", &fog_end, 0.0f, 1000.0f))
											lighting.SetFogEnd((int)fog_end);

										if (gui->slider_float("Bloom Intensity", &bloom_intensity, 0.0f, 10.0f))
											lighting.SetBloomIntensity((int)bloom_intensity);

										if (gui->slider_float("Bloom Size", &bloom_size, 0.0f, 500))
											lighting.SetBloomSize((int)bloom_size);

										if (gui->slider_float("Bloom Threshold", &bloom_threshold, 0.0f, 500))
											lighting.SetBloomThreshHold((int)bloom_threshold);

										if (gui->slider_float("Sunrays Intensity", &sunrays_intensity, 0.0f, 10.0f))
											lighting.SetSunRayIntensity((int)sunrays_intensity);

										if (gui->slider_float("Sunrays Spread", &sunrays_spread, 0.0f, 500))
											lighting.SetSunRaySpread((int)sunrays_spread);

										if (gui->label_color_edit("Ambience", &ambience.x))
											lighting.SetAmbience(ambience);

										if (gui->label_color_edit("ColorShift Top", &shift_top.x))
											lighting.SetColorShiftTop(shift_top);

										if (gui->label_color_edit("ColorShift Bottom", &shift_bottom.x))
											lighting.SetColorShiftBottom(shift_bottom);




									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Camera", 2, 2);
									{


									}
									gui->end_child();

									gui->begin_child("World", 2, 2);
									{





									}
									gui->end_child();
								}
								gui->end_group();


								gui->begin_group();
								{
									gui->begin_child("Game", 2, 2);
									{

									}
									gui->end_child();

								}
								gui->end_group();
							}
							gui->end_content();
						}
					}


				}
				gui->end();
			}

			//if (var->gui.current_section[1])
			//{
			//	gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
			//	gui->begin("other", nullptr, var->window.flags);
			//	{
			//		draw->window_decorations();

			//	}
			//	gui->end();
			//}
			if (var->gui.current_section[1])
			{
				static int selected_row = -1;

				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("Players", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->push_style_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
					gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->content.spacing);
					gui->begin_def_child("table test", ImVec2(GetWindowWidth() - elements->content.window_padding.x * 2, GetContentRegionAvail().y - elements->content.window_padding.y * 6), 0, ImGuiWindowFlags_NoMove);
					{
						gui->push_font(var->font.tahoma);

						gui->push_style_color(ImGuiCol_TableBorderLight, draw->get_clr(clr->window.stroke));
						gui->push_style_color(ImGuiCol_TableBorderStrong, draw->get_clr(clr->window.stroke));
						gui->push_style_color(ImGuiCol_TableRowBg, draw->get_clr(clr->window.background_one));
						gui->push_style_color(ImGuiCol_TableRowBgAlt, draw->get_clr(clr->window.background_one));
						gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->widgets.text_inactive));
						if (gui->begin_table("Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(GetContentRegionAvail().x - 1, 0)))
						{
							for (int row = 0; row < 10; row++)
							{
								gui->table_next_row();

								gui->table_set_column_index(0);
								{
									if (selected_row == row)
										gui->push_style_color(ImGuiCol_Text, draw->get_clr(clr->accent));

									draw->text_outline((std::stringstream{} << "Nigga " << row).str().c_str());

									if (selected_row == row)
										gui->pop_style_color();

									if (IsItemClicked())
										selected_row = row;
								}

								gui->table_set_column_index(1);
								{
									draw->text_outline((std::stringstream{} << "pos " << row).str().c_str());
								}

								gui->table_set_column_index(2);
								{
									draw->text_outline((std::stringstream{} << "info " << row).str().c_str());
								}
							}
							gui->end_table();

							Text((std::stringstream{} << "selected row - " << std::to_string(selected_row)).str().c_str());
							ImGui::Separator();
							auto draw_list = ImGui::GetWindowDrawList();

							std::string text1 = "Name: Player_" + std::to_string(selected_row);
							gui->text(draw_list, text1.c_str());

							std::string text2 = "Position: X=" + std::to_string(selected_row * 10) + " Y=" + std::to_string(selected_row * 20);
							gui->text(draw_list, text2.c_str());

							std::string text3 = "Additional Info: health " + std::to_string(selected_row);
							gui->text(draw_list, text3.c_str());
							gui->pop_font();
						}
						gui->pop_style_color(5);
					}
					gui->end_def_child();
					gui->pop_style_var(2);

				}
				gui->end();
			}



			if (var->gui.current_section[2])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("Style", nullptr, var->window.flags);
				{
					draw->window_decorations();

					gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
					gui->begin_content();
					{
						static float menu_accent[4];
						static float contrast_one[4];
						static float contrast_two[4];
						static float inline_c[4];
						static float outline_c[4];
						static float text_active[4];
						static float text_inactive[4];

						static bool initialized = false;
						if (!initialized)
						{
							menu_accent[0] = clr->accent.Value.x;
							menu_accent[1] = clr->accent.Value.y;
							menu_accent[2] = clr->accent.Value.z;
							menu_accent[3] = 1.f;

							contrast_one[0] = clr->window.background_one.Value.x;
							contrast_one[1] = clr->window.background_one.Value.y;
							contrast_one[2] = clr->window.background_one.Value.z;
							contrast_one[3] = 1.f;

							contrast_two[0] = clr->window.background_two.Value.x;
							contrast_two[1] = clr->window.background_two.Value.y;
							contrast_two[2] = clr->window.background_two.Value.z;
							contrast_two[3] = 1.f;

							inline_c[0] = clr->window.stroke.Value.x;
							inline_c[1] = clr->window.stroke.Value.y;
							inline_c[2] = clr->window.stroke.Value.z;
							inline_c[3] = 1.f;

							outline_c[0] = clr->widgets.stroke_two.Value.x;
							outline_c[1] = clr->widgets.stroke_two.Value.y;
							outline_c[2] = clr->widgets.stroke_two.Value.z;
							outline_c[3] = 1.f;

							text_active[0] = clr->widgets.text.Value.x;
							text_active[1] = clr->widgets.text.Value.y;
							text_active[2] = clr->widgets.text.Value.z;
							text_active[3] = 1.f;

							text_inactive[0] = clr->widgets.text_inactive.Value.x;
							text_inactive[1] = clr->widgets.text_inactive.Value.y;
							text_inactive[2] = clr->widgets.text_inactive.Value.z;
							text_inactive[3] = 1.f;

							initialized = true;
						}

						gui->begin_child("Theme", 1, 2);
						{
							if (gui->label_color_edit("Menu Accent", menu_accent, false))
							{
								clr->accent.Value.x = menu_accent[0];
								clr->accent.Value.y = menu_accent[1];
								clr->accent.Value.z = menu_accent[2];
							//	printf("Menu Accent: R=%.3f G=%.3f B=%.3f\n", menu_accent[0], menu_accent[1], menu_accent[2]);
							}

							if (gui->label_color_edit("Contrast One", contrast_one, false))
							{
								clr->window.background_one.Value.x = contrast_one[0];
								clr->window.background_one.Value.y = contrast_one[1];
								clr->window.background_one.Value.z = contrast_one[2];
							//	printf("Contrast One: R=%.3f G=%.3f B=%.3f\n", contrast_one[0], contrast_one[1], contrast_one[2]);
							}

							if (gui->label_color_edit("Contrast Two", contrast_two, false))
							{
								clr->window.background_two.Value.x = contrast_two[0];
								clr->window.background_two.Value.y = contrast_two[1];
								clr->window.background_two.Value.z = contrast_two[2];
							//	printf("Contrast Two: R=%.3f G=%.3f B=%.3f\n", contrast_two[0], contrast_two[1], contrast_two[2]);
							}

							if (gui->label_color_edit("Inline", inline_c, false))
							{
								clr->window.stroke.Value.x = inline_c[0];
								clr->window.stroke.Value.y = inline_c[1];
								clr->window.stroke.Value.z = inline_c[2];
							//	printf("Inline: R=%.3f G=%.3f B=%.3f\n", inline_c[0], inline_c[1], inline_c[2]);
							}

							if (gui->label_color_edit("Outline", outline_c, false))
							{
								clr->widgets.stroke_two.Value.x = outline_c[0];
								clr->widgets.stroke_two.Value.y = outline_c[1];
								clr->widgets.stroke_two.Value.z = outline_c[2];
								//printf("Outline: R=%.3f G=%.3f B=%.3f\n", outline_c[0], outline_c[1], outline_c[2]);
							}


							if (gui->label_color_edit("Text Active", text_active, false))
							{
								clr->widgets.text.Value.x = text_active[0];
								clr->widgets.text.Value.y = text_active[1];
								clr->widgets.text.Value.z = text_active[2];
							}

							if (gui->label_color_edit("Text Inctive", text_inactive, false))
							{
								clr->widgets.text_inactive.Value.x = text_inactive[0];
								clr->widgets.text_inactive.Value.y = text_inactive[1];
								clr->widgets.text_inactive.Value.z = text_inactive[2];
							}
						}
						gui->end_child();

						gui->begin_child("Style", 1, 2);
						{
							static int theme_index = 0;
							static int previous_theme = -1;
							const char* themes[] = { "Default", "Atlanta", "Blue", "Pink", "Purple", "White", "Red", "Green", "Yellow", "Gray" };
							gui->dropdown("Theme", &theme_index, themes, IM_ARRAYSIZE(themes));

							if (theme_index != previous_theme)
							{
								if (theme_index == 1)
								{
									clr->accent = ImColor(154, 127, 172);
									clr->window.background_one = ImColor(36, 36, 47);
								}
								if (theme_index == 2)
								{
									clr->accent = ImColor(62, 93, 241);
									clr->window.background_one = ImColor(12, 12, 12);
								}
								if (theme_index == 3)
								{
									clr->accent = ImColor(204, 115, 146);
									clr->window.background_one = ImColor(19, 19, 21);

									//clr->window.background_two = ImColor(20, 20, 20);
								}
								if (theme_index == 4)
								{
									clr->accent = ImColor(141, 0, 234);
									clr->window.background_one = ImColor(0, 0, 0);
								}
								if (theme_index == 5)
								{
									clr->accent = ImColor(255, 255, 255);
									clr->window.background_one = ImColor(12, 12, 12);
								}
								if (theme_index == 6)
								{
									clr->accent = ImColor(255, 50, 50);
									clr->window.background_one = ImColor(20, 0, 0);
								}
								if (theme_index == 7)
								{
									clr->accent = ImColor(50, 255, 120);
									clr->window.background_one = ImColor(0, 20, 10);
								}
								if (theme_index == 8)
								{
									clr->accent = ImColor(255, 255, 100);
									clr->window.background_one = ImColor(30, 30, 0);
								}
								if (theme_index == 9)
								{
									clr->accent = ImColor(160, 160, 160);
									clr->window.background_one = ImColor(25, 25, 25);
								}

								menu_accent[0] = clr->accent.Value.x;
								menu_accent[1] = clr->accent.Value.y;
								menu_accent[2] = clr->accent.Value.z;

								contrast_one[0] = clr->window.background_one.Value.x;
								contrast_one[1] = clr->window.background_one.Value.y;
								contrast_one[2] = clr->window.background_one.Value.z;

								contrast_two[0] = clr->window.background_two.Value.x;
								contrast_two[1] = clr->window.background_two.Value.y;
								contrast_two[2] = clr->window.background_two.Value.z;

								inline_c[0] = clr->window.stroke.Value.x;
								inline_c[1] = clr->window.stroke.Value.y;
								inline_c[2] = clr->window.stroke.Value.z;

								outline_c[0] = clr->widgets.stroke_two.Value.x;
								outline_c[1] = clr->widgets.stroke_two.Value.y;
								outline_c[2] = clr->widgets.stroke_two.Value.z;

								text_active[0] = clr->widgets.text.Value.x;
								text_active[1] = clr->widgets.text.Value.y;
								text_active[2] = clr->widgets.text.Value.z;

								text_inactive[0] = clr->widgets.text_inactive.Value.x;
								text_inactive[1] = clr->widgets.text_inactive.Value.y;
								text_inactive[2] = clr->widgets.text_inactive.Value.z;

								previous_theme = theme_index;
							}
							ImVec2 pos = ImGui::GetCursorPos();

							if (gui->button("Rescan")) {
								RBX::GetGlobalVariables();
							}

						

							if (gui->button("Exit Process")) {
								exit(0);
							}

							

							if (gui->button("Show Console")) {
								ShowWindow(GetConsoleWindow(), SW_SHOW);
							}

						

							if (gui->button("Hide Console")) {
								ShowWindow(GetConsoleWindow(), SW_HIDE);
							}


							gui->label_keybindA("Menu Hotkey", &var->gui.menu_key, 0);

							gui->checkbox("Hover Highlight", &var->window.hover_hightlight);
					
					
							gui->slider_float("Window Titlebar Height", &var->window.titlebar, 20, 50);
							gui->slider_float("Window Padding X", &var->window.padding.x, 0, 50);
							gui->slider_float("Window Padding Y", &var->window.padding.y, 0, 50);
							gui->slider_float("Window Border Size", &var->window.border_size, 0, 10);
							gui->slider_float("Window Spacing X", &var->window.spacing.x, 0, 50);
							gui->slider_float("Window Spacing Y", &var->window.spacing.y, 0, 50);
							gui->slider_float("Item Spacing X", &elements->content.spacing.x, 0, 10);
							gui->slider_float("Item Spacing Y", &elements->content.spacing.y, 0, 10);
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
						gui->checkbox("Cache Off Screen Check", &globals::cframe);
						gui->checkbox("Sleepy Player Cache", &globals::HeavyOptimize);

						gui->slider_int("Player Cache Speed", &globals::threadrestarttime, 0, 10000);
						gui->slider_int("Player Cache Thread Time", &globals::threadtime, 0, 10000);


						gui->checkbox("V-Sync", &globals::vsync);
						gui->checkbox("Allow Menu Tearing", &globals::allow_tearing);
						gui->checkbox("Prioritize Overlay", &globals::highcpuusageesp);


						gui->end_child();
					}
					gui->end_content();
				}
				gui->end();
			}



			//if (var->gui.current_section[6])
			//{
			//	gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
			//	gui->begin("cloud", nullptr, var->window.flags);
			//	{
			//		draw->window_decorations();

			//	}
			//	gui->end();
			//}
		}

		var->window.width = GetCurrentWindow()->ContentSize.x + style->ItemSpacing.x;

		if (IsMouseHoveringRect(pos, pos + size))
			SetWindowFocus();
	}
	gui->end();
	gui->pop_style_var();
}
