#include "settings/functions.h"
#include "../src/main.h"
#include <Windows.h>
#include "../src/features/cheats/visuals/renderer/renderer.h"
#include "../src/features/cheats/explorer/explorer.h"
#include "../src/features/cheats/console/console.h"
#include <vector>
#include <algorithm>
#include <string>

void c_gui::render()
{
	if (GetAsyncKeyState(var->gui.menu_key) & 0x1)
		var->gui.menu_opened = !var->gui.menu_opened;

	var->gui.menu_alpha = ImClamp(var->gui.menu_alpha + (gui->fixed_speed(8.f) * (var->gui.menu_opened ? 1.f : -1.f)), 0.f, 1.f);

	if (var->gui.menu_alpha <= 0.01f)
		return;

	gui->set_next_window_pos(ImVec2(GetIO().DisplaySize.x / 2 - var->window.width / 2, 20));
	gui->set_next_window_size(ImVec2(var->window.width, elements->section.size.y + var->window.spacing.y * 2 - 1));
	gui->push_style_var(ImGuiStyleVar_Alpha, var->gui.menu_alpha);
	gui->begin("men", nullptr, var->window.main_flags);
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

		{
			if (var->gui.current_section[0])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("Memorysense", nullptr, var->window.flags);
				{
					draw->window_decorations();

					{
						static int subtabs;
						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar));
						gui->begin_group();
						{
							gui->sub_section("Combat", 0, subtabs, 4);
							gui->sub_section("Visuals", 1, subtabs, 4);
							gui->sub_section("Movement", 2, subtabs, 4);
							gui->sub_section("Misc", 3, subtabs, 4);
						}
						gui->end_group();

						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + elements->section.height - 1));
						gui->begin_content();
						{
							if (subtabs == 0) {

								gui->begin_group();
								{
									gui->begin_child("Aimbot", 2, 1);
									{
										gui->checkbox("Enabled", &memorysense::aimbot::enabled, &memorysense::aimbot::key, &memorysense::aimbot::mode);
										
										gui->checkbox("Team Check", &memorysense::aimbot::team_check);
										gui->checkbox("Disable Out of FOV", &memorysense::aimbot::disable_out_of_fov);
										gui->checkbox("Prediction", &memorysense::aimbot::prediction);
										gui->checkbox("Spectate Target", &memorysense::aimbot::spectate_target);
										
										const char* aim_parts[] = { "Head", "HumanoidRootPart", "Closest Part" };
										gui->dropdown("Aim Part", &memorysense::aimbot::aim_part, aim_parts, IM_ARRAYSIZE(aim_parts), true);
										
										const char* aim_types[] = { "Mouse Lock", "Cam Lock", "Frame Lock" };
										gui->dropdown("Aim Type", &memorysense::aimbot::aim_type, aim_types, IM_ARRAYSIZE(aim_types), true);
										
										gui->slider_int("Smooth", &memorysense::aimbot::smooth, 1, 100, false, "%d");
										gui->slider_int("FOV", &memorysense::aimbot::fov, 10, 500, false, "%d");
										gui->slider_int("Min Distance", &memorysense::aimbot::min_distance, 0, 1000, false, "%d");
										gui->slider_int("Max Distance", &memorysense::aimbot::max_distance, 0, 1000, false, "%d");
										
										if (memorysense::aimbot::prediction) {
											gui->slider_float("Pred X", &memorysense::aimbot::pred_x, 0.1f, 5.0f, false, "%.1f");
											gui->slider_float("Pred Y", &memorysense::aimbot::pred_y, 0.1f, 5.0f, false, "%.1f");
											gui->slider_float("Pred Z", &memorysense::aimbot::pred_z, 0.1f, 5.0f, false, "%.1f");
										}
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Triggerbot", 2, 2);
									{
										gui->checkbox("Enabled", &memorysense::aimbot::triggerbot_enabled, &memorysense::aimbot::triggerbot_keybind_key, &memorysense::aimbot::triggerbot_keybind_type);
										
										gui->checkbox("Team Check", &memorysense::aimbot::triggerbot_team_check);
										gui->checkbox("Head Only", &memorysense::aimbot::triggerbot_head_only);
										gui->checkbox("Wall Check", &memorysense::aimbot::triggerbot_wall_check);
										
										gui->slider_int("FOV", &memorysense::aimbot::triggerbot_fov, 10, 200, false, "%d");
										gui->slider_int("Min Distance", &memorysense::aimbot::triggerbot_min_distance, 0, 1000, false, "%d");
										gui->slider_int("Max Distance", &memorysense::aimbot::triggerbot_max_distance, 0, 1000, false, "%d");
										gui->slider_int("Hit Chance", &memorysense::aimbot::triggerbot_hit_chance, 1, 100, false, "%d%%");
										gui->slider_int("Delay (ms)", &memorysense::aimbot::triggerbot_delay_ms, 1, 100, false, "%d");
									}
									gui->end_child();

									gui->begin_child("Extra", 2, 2);
									{

									}
									gui->end_child();
								}
								gui->end_group();

							}

							if (subtabs == 1) {

								ImVec2 checkbox_pos;

								gui->begin_group();
								{
									gui->begin_child("ESP", 2, 1);
									{
										ImColor box_color = ImColor(memorysense::visuals::colors::visible::box);
										float box_col[4] = { box_color.Value.x, box_color.Value.y, box_color.Value.z, box_color.Value.w };
										gui->checkbox("Box", &memorysense::visuals::box, box_col, true);
										memorysense::visuals::colors::visible::box = ImColor(box_col[0], box_col[1], box_col[2], box_col[3]);

										ImColor healthbar_color = ImColor(memorysense::visuals::colors::visible::healthbar);
										float healthbar_col[4] = { healthbar_color.Value.x, healthbar_color.Value.y, healthbar_color.Value.z, healthbar_color.Value.w };
										gui->checkbox("Healthbar", &memorysense::visuals::healthbar, healthbar_col, true);
										memorysense::visuals::colors::visible::healthbar = ImColor(healthbar_col[0], healthbar_col[1], healthbar_col[2], healthbar_col[3]);

										ImColor name_color = ImColor(memorysense::visuals::colors::visible::name);
										float name_col[4] = { name_color.Value.x, name_color.Value.y, name_color.Value.z, name_color.Value.w };
										gui->checkbox("Name", &memorysense::visuals::name, name_col, true);
										memorysense::visuals::colors::visible::name = ImColor(name_col[0], name_col[1], name_col[2], name_col[3]);

										ImColor distance_color = ImColor(memorysense::visuals::colors::visible::distance);
										float distance_col[4] = { distance_color.Value.x, distance_color.Value.y, distance_color.Value.z, distance_color.Value.w };
										gui->checkbox("Distance", &memorysense::visuals::distance, distance_col, true);
										memorysense::visuals::colors::visible::distance = ImColor(distance_col[0], distance_col[1], distance_col[2], distance_col[3]);

										ImColor flags_color = ImColor(memorysense::visuals::colors::visible::flags);
										float flags_col[4] = { flags_color.Value.x, flags_color.Value.y, flags_color.Value.z, flags_color.Value.w };
										gui->checkbox("Flags", &memorysense::visuals::flags, flags_col, true);
										memorysense::visuals::colors::visible::flags = ImColor(flags_col[0], flags_col[1], flags_col[2], flags_col[3]);

										ImColor tracer_color = ImColor(memorysense::visuals::colors::visible::tracer);
										float tracer_col[4] = { tracer_color.Value.x, tracer_color.Value.y, tracer_color.Value.z, tracer_color.Value.w };
										gui->checkbox("Tracer", &memorysense::visuals::tracer, tracer_col, true);
										memorysense::visuals::colors::visible::tracer = ImColor(tracer_col[0], tracer_col[1], tracer_col[2], tracer_col[3]);

										float y_pos = GetCursorPosY();
										gui->checkbox("Chams", &memorysense::visuals::cham);

										ImVec2 stored_pos = GetCursorPos();
										ImColor cham_fill_color = ImColor(memorysense::visuals::colors::visible::cham_fill);
										float cham_fill_col[4] = { cham_fill_color.Value.x, cham_fill_color.Value.y, cham_fill_color.Value.z, cham_fill_color.Value.w };
										gui->set_cursor_pos(ImVec2(GetWindowWidth() - elements->widgets.color_size.x * 2 - 5 - GetStyle().WindowPadding.x, y_pos));
										gui->color_edit("##ChamFill", cham_fill_col, true);
										memorysense::visuals::colors::visible::cham_fill = ImColor(cham_fill_col[0], cham_fill_col[1], cham_fill_col[2], cham_fill_col[3]);

										gui->set_cursor_pos(ImVec2(GetWindowWidth() - elements->widgets.color_size.x - GetStyle().WindowPadding.x, y_pos));
										ImColor cham_outline_color = ImColor(memorysense::visuals::colors::visible::cham_outline);
										float cham_outline_col[4] = { cham_outline_color.Value.x, cham_outline_color.Value.y, cham_outline_color.Value.z, cham_outline_color.Value.w };
										gui->color_edit("##ChamOutline", cham_outline_col, true);
										memorysense::visuals::colors::visible::cham_outline = ImColor(cham_outline_col[0], cham_outline_col[1], cham_outline_col[2], cham_outline_col[3]);
										
										gui->set_cursor_pos(stored_pos);
									}
									gui->end_child();
								}
								gui->end_group();

								gui->sameline();

								gui->begin_group();
								{
									gui->begin_child("Extra", 2, 2);
									{
										gui->checkbox("Clientcheck", &memorysense::visuals::clientcheck);
										gui->checkbox("Teamcheck", &memorysense::visuals::teamcheck);

										const char* cham_types[2] = { "Cube", "Highlight" };
										gui->dropdown("Chams Type", &memorysense::visuals::cham_type, cham_types, IM_ARRAYSIZE(cham_types), true);

										gui->slider_float("Healthbar Padding", &memorysense::visuals::healthbar_padding, 1.0f, 20.0f, false, "%.1fpx");
										gui->slider_float("Healthbar Size", &memorysense::visuals::healthbar_size, 1.0f, 10.0f, false, "%.1fpx");

										const char* outline_elements[] = { "Box", "Healthbar", "Name", "Distance", "Flags" };
										gui->multi_dropdown("Outline", memorysense::visuals::outline_elements, outline_elements, 5);

										static ImColor outline_color = ImColor(memorysense::visuals::outline_color);
										float outline_col[4] = { outline_color.Value.x, outline_color.Value.y, outline_color.Value.z, outline_color.Value.w };
										if (gui->label_color_edit("Outline Color", outline_col, true)) {
											memorysense::visuals::outline_color = ImColor(outline_col[0], outline_col[1], outline_col[2], outline_col[3]);
										}
									}
									gui->end_child();

									gui->begin_child("Positioning", 2, 2);
									{
										const char* side_names[] = { "Left", "Right", "Top", "Bottom" };

										gui->dropdown("Name Side", (int*)&memorysense::visuals::element_sides[0], side_names, 4);
										gui->dropdown("Distance Side", (int*)&memorysense::visuals::element_sides[1], side_names, 4);
										gui->dropdown("Flags Side", (int*)&memorysense::visuals::element_sides[2], side_names, 4);
										gui->dropdown("Healthbar Side", (int*)&memorysense::visuals::element_sides[4], side_names, 4);
									}
									gui->end_child();
								}
								gui->end_group();


							}

						}
						gui->end_content();
					}

				}
				gui->end();
			}

			if (var->gui.current_section[1])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("preview", nullptr, var->window.flags);
				{
					draw->window_decorations();

					{
						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
						gui->begin_content();
						{
							ImVec2 content_min = GetCursorScreenPos();
							ImVec2 content_max = ImVec2(content_min.x + GetContentRegionAvail().x, content_min.y + GetContentRegionAvail().y);
							ImVec2 center = ImVec2((content_min.x + content_max.x) * 0.5f, (content_min.y + content_max.y) * 0.5f);

							ImVec2 box_size(150.f, 250.f);
							ImVec2 box_pos(std::round(center.x - box_size.x * 0.5f), std::round(center.y - box_size.y * 0.5f));

							ImDrawList* draw_list = GetWindowDrawList();
							draw_list->Flags &= ~ImDrawListFlags_AntiAliasedLines;
							draw_list->Flags &= ~ImDrawListFlags_AntiAliasedFill;

							static bool is_dragging = false;
							static int dragged_element = -1;
							static ImVec2 drag_offset;
							ImVec2 mouse_pos = GetMousePos();

							ImVec2 left_zone_min(box_pos.x - 100, box_pos.y);
							ImVec2 left_zone_max(box_pos.x, box_pos.y + box_size.y);
							bool left_hovered = mouse_pos.x >= left_zone_min.x && mouse_pos.x <= left_zone_max.x &&
								mouse_pos.y >= left_zone_min.y && mouse_pos.y <= left_zone_max.y;

							ImVec2 right_zone_min(box_pos.x + box_size.x, box_pos.y);
							ImVec2 right_zone_max(box_pos.x + box_size.x + 100, box_pos.y + box_size.y);
							bool right_hovered = mouse_pos.x >= right_zone_min.x && mouse_pos.x <= right_zone_max.x &&
								mouse_pos.y >= right_zone_min.y && mouse_pos.y <= right_zone_max.y;

							ImVec2 top_zone_min(box_pos.x, box_pos.y - 100);
							ImVec2 top_zone_max(box_pos.x + box_size.x, box_pos.y);
							bool top_hovered = mouse_pos.x >= top_zone_min.x && mouse_pos.x <= top_zone_max.x &&
								mouse_pos.y >= top_zone_min.y && mouse_pos.y <= top_zone_max.y;

							ImVec2 bottom_zone_min(box_pos.x, box_pos.y + box_size.y);
							ImVec2 bottom_zone_max(box_pos.x + box_size.x, box_pos.y + box_size.y + 100);
							bool bottom_hovered = mouse_pos.x >= bottom_zone_min.x && mouse_pos.x <= bottom_zone_max.x &&
								mouse_pos.y >= bottom_zone_min.y && mouse_pos.y <= bottom_zone_max.y;

							bool mouse_clicked = IsMouseClicked(0);
							bool mouse_released = IsMouseReleased(0);

							struct DraggableElement {
								int id;
								std::string text;
								ImU32 color;
								bool outline;
								memorysense::visuals::ESP_SIDE side;
								ImVec2 position;
								bool visible;
							};

							static std::vector<DraggableElement> draggable_elements;
							draggable_elements.clear();

							if (memorysense::visuals::name) {
								draggable_elements.push_back({ 0, "Sue Heck", memorysense::visuals::colors::visible::name, memorysense::visuals::outline_elements[2] ? true : false, memorysense::visuals::element_sides[0], ImVec2(0, 0), true});
							}
							if (memorysense::visuals::distance) {
								draggable_elements.push_back({ 1, "[67.8m]", memorysense::visuals::colors::visible::distance, memorysense::visuals::outline_elements[3] ? true : false, memorysense::visuals::element_sides[1], ImVec2(0, 0), true });
							}
							if (memorysense::visuals::flags) {
								draggable_elements.push_back({ 2, "Idle", memorysense::visuals::colors::visible::flags, memorysense::visuals::outline_elements[4] ? true : false, memorysense::visuals::element_sides[2], ImVec2(0, 0), true });
							}

							if (mouse_clicked && !is_dragging) {
								for (auto& elem : draggable_elements) {
									if (!elem.visible) continue;

									ImGui::PushFont(var->font.tahoma);
									ImVec2 text_size = ImGui::CalcTextSize(elem.text.c_str());
									ImGui::PopFont();

									float padding = 3.0f;
									float current_offset = 0.0f;

									for (int i = 0; i < elem.id; i++) {
										if (draggable_elements[i].side == elem.side) {
											ImGui::PushFont(var->font.tahoma);
											ImVec2 other_text_size = ImGui::CalcTextSize(draggable_elements[i].text.c_str());
											ImGui::PopFont();
											current_offset += other_text_size.y + 2.0f;
										}
									}

									float healthbar_space = 0.0f;
									if (memorysense::visuals::healthbar) {
										bool healthbar_on_same_side = false;
										if (elem.side == memorysense::visuals::ESP_SIDE::LEFT && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::RIGHT && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::RIGHT) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::TOP && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::TOP) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::BOTTOM && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::BOTTOM) healthbar_on_same_side = true;

										if (healthbar_on_same_side) {
											healthbar_space = memorysense::visuals::healthbar_size + 3.0f + 3.0f;
										}
									}

									ImVec2 elem_pos;
									switch (elem.side) {
									case memorysense::visuals::ESP_SIDE::LEFT:
										elem_pos = ImVec2(box_pos.x - padding - text_size.x - healthbar_space, box_pos.y + current_offset);
										break;
									case memorysense::visuals::ESP_SIDE::RIGHT:
										elem_pos = ImVec2(box_pos.x + box_size.x + padding + healthbar_space, box_pos.y + current_offset);
										break;
									case memorysense::visuals::ESP_SIDE::TOP:
										elem_pos = ImVec2(box_pos.x + box_size.x * 0.5f - text_size.x * 0.5f, box_pos.y - padding - current_offset - text_size.y - healthbar_space);
										break;
									case memorysense::visuals::ESP_SIDE::BOTTOM:
										elem_pos = ImVec2(box_pos.x + box_size.x * 0.5f - text_size.x * 0.5f, box_pos.y + box_size.y + padding + current_offset + healthbar_space);
										break;
									}

									ImRect elem_rect(elem_pos, ImVec2(elem_pos.x + text_size.x, elem_pos.y + text_size.y));
									if (elem_rect.Contains(mouse_pos)) {
										is_dragging = true;
										dragged_element = elem.id;
										drag_offset = ImVec2(mouse_pos.x - elem_pos.x, mouse_pos.y - elem_pos.y);
									}
								}

								if (memorysense::visuals::healthbar) {
									bool is_vertical = (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT ||
										memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::RIGHT);
									float gap = 3.0f;
									float bar_thickness = memorysense::visuals::healthbar_size;

									ImVec2 bar_pos;
									ImVec2 bar_size;
									ImVec2 hitbox_pos;
									ImVec2 hitbox_size;

									if (is_vertical) {
										bar_size = ImVec2(bar_thickness, box_size.y);
										hitbox_size = ImVec2(bar_thickness + 8.0f, box_size.y);
										if (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT) {
											bar_pos = ImVec2(box_pos.x - gap - bar_thickness, box_pos.y);
											hitbox_pos = ImVec2(box_pos.x - gap - bar_thickness - 4.0f, box_pos.y);
										}
										else {
											bar_pos = ImVec2(box_pos.x + box_size.x + gap, box_pos.y);
											hitbox_pos = ImVec2(box_pos.x + box_size.x + gap - 4.0f, box_pos.y);
										}
									}
									else {
										bar_size = ImVec2(box_size.x, bar_thickness);
										hitbox_size = ImVec2(box_size.x, bar_thickness + 8.0f);
										if (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::TOP) {
											bar_pos = ImVec2(box_pos.x, box_pos.y - gap - bar_thickness);
											hitbox_pos = ImVec2(box_pos.x, box_pos.y - gap - bar_thickness - 4.0f);
										}
										else {
											bar_pos = ImVec2(box_pos.x, box_pos.y + box_size.y + gap);
											hitbox_pos = ImVec2(box_pos.x, box_pos.y + box_size.y + gap - 4.0f);
										}
									}

									ImRect healthbar_rect(hitbox_pos, ImVec2(hitbox_pos.x + hitbox_size.x, hitbox_pos.y + hitbox_size.y));
									if (healthbar_rect.Contains(mouse_pos)) {
										is_dragging = true;
										dragged_element = 3;
										drag_offset = ImVec2(mouse_pos.x - bar_pos.x, mouse_pos.y - bar_pos.y);
									}
								}
							}

							if (mouse_released && is_dragging) {
								memorysense::visuals::ESP_SIDE new_side;

								if (left_hovered) new_side = memorysense::visuals::ESP_SIDE::LEFT;
								else if (right_hovered) new_side = memorysense::visuals::ESP_SIDE::RIGHT;
								else if (top_hovered) new_side = memorysense::visuals::ESP_SIDE::TOP;
								else if (bottom_hovered) new_side = memorysense::visuals::ESP_SIDE::BOTTOM;
								else new_side = (dragged_element < 3) ? draggable_elements[dragged_element].side : memorysense::visuals::element_sides[4];

								switch (dragged_element) {
								case 0:
									memorysense::visuals::element_sides[0] = new_side;
									draggable_elements[0].side = new_side;
									break;
								case 1:
									memorysense::visuals::element_sides[1] = new_side;
									draggable_elements[1].side = new_side;
									break;
								case 2:
									memorysense::visuals::element_sides[2] = new_side;
									draggable_elements[2].side = new_side;
									break;
								case 3:
									memorysense::visuals::element_sides[4] = new_side;
									break;
								}

								is_dragging = false;
								dragged_element = -1;
							}

							if (memorysense::visuals::box) {
								if (memorysense::visuals::outline_elements[0]) {
									draw_list->AddRect(ImVec2(box_pos.x - 1, box_pos.y - 1),
										ImVec2(box_pos.x + box_size.x + 1, box_pos.y + box_size.y + 1),
										memorysense::visuals::outline_color, 0.f, 0, 1.f);
									draw_list->AddRect(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
										memorysense::visuals::colors::visible::box, 0.f, 0, 1.f);
									draw_list->AddRect(ImVec2(box_pos.x + 1, box_pos.y + 1),
										ImVec2(box_pos.x + box_size.x - 1, box_pos.y + box_size.y - 1),
										memorysense::visuals::outline_color, 0.f, 0, 1.f);
								}
								else {
									draw_list->AddRect(box_pos, ImVec2(box_pos.x + box_size.x, box_pos.y + box_size.y),
										memorysense::visuals::colors::visible::box, 0.f, 0, 1.f);
								}
							}

							for (auto& elem : draggable_elements) {
								if (!elem.visible) continue;

								ImGui::PushFont(var->font.tahoma);
								ImVec2 text_size = ImGui::CalcTextSize(elem.text.c_str());
								ImGui::PopFont();

								ImVec2 elem_pos;
								if (is_dragging && dragged_element == elem.id) {
									elem_pos = ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y);
								}
								else {
									float padding = 3.0f;
									float current_offset = 0.0f;

									for (int i = 0; i < elem.id; i++) {
										if (draggable_elements[i].side == elem.side) {
											ImGui::PushFont(var->font.tahoma);
											ImVec2 other_text_size = ImGui::CalcTextSize(draggable_elements[i].text.c_str());
											ImGui::PopFont();
											current_offset += other_text_size.y + 2.0f;
										}
									}

									float healthbar_space = 0.0f;
									if (memorysense::visuals::healthbar) {
										bool healthbar_on_same_side = false;
										if (elem.side == memorysense::visuals::ESP_SIDE::LEFT && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::RIGHT && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::RIGHT) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::TOP && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::TOP) healthbar_on_same_side = true;
										else if (elem.side == memorysense::visuals::ESP_SIDE::BOTTOM && memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::BOTTOM) healthbar_on_same_side = true;

										if (healthbar_on_same_side) {
											healthbar_space = memorysense::visuals::healthbar_size + 3.0f + 3.0f;
										}
									}

									switch (elem.side) {
									case memorysense::visuals::ESP_SIDE::LEFT:
										elem_pos = ImVec2(box_pos.x - padding - text_size.x - healthbar_space, box_pos.y + current_offset);
										break;
									case memorysense::visuals::ESP_SIDE::RIGHT:
										elem_pos = ImVec2(box_pos.x + box_size.x + padding + healthbar_space, box_pos.y + current_offset);
										break;
									case memorysense::visuals::ESP_SIDE::TOP:
										elem_pos = ImVec2(box_pos.x + box_size.x * 0.5f - text_size.x * 0.5f, box_pos.y - padding - current_offset - text_size.y - healthbar_space);
										break;
									case memorysense::visuals::ESP_SIDE::BOTTOM:
										elem_pos = ImVec2(box_pos.x + box_size.x * 0.5f - text_size.x * 0.5f, box_pos.y + box_size.y + padding + current_offset + healthbar_space);
										break;
									}
								}

								ImGui::PushFont(var->font.tahoma);
								if (elem.outline) {
									const ImVec2 offsets[4] = { ImVec2(-1,0), ImVec2(1,0), ImVec2(0,-1), ImVec2(0,1) };
									for (int i = 0; i < 4; i++) {
										draw_list->AddText(ImVec2(elem_pos.x + offsets[i].x, elem_pos.y + offsets[i].y), memorysense::visuals::outline_color, elem.text.c_str());
									}
								}
								draw_list->AddText(elem_pos, ImColor(elem.color), elem.text.c_str());
								ImGui::PopFont();

								ImRect drag_rect(elem_pos, ImVec2(elem_pos.x + text_size.x, elem_pos.y + text_size.y));
							}

							if (memorysense::visuals::healthbar) {
								bool is_vertical = (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT ||
									memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::RIGHT);
								float gap = 3.0f;
								float bar_thickness = memorysense::visuals::healthbar_size;

								static float animation_time = 0.0f;
								animation_time += ImGui::GetIO().DeltaTime;
								float base_health = 0.55f;
								float amplitude = 0.35f;
								float frequency = 0.8f;
								float raw_percent = base_health + amplitude * sin(animation_time * frequency);
								float ratio = 1.0f - pow(1.0f - raw_percent, 3.0f);

								ImVec2 bar_pos;
								ImVec2 bar_size;

								if (is_dragging && dragged_element == 3) {
									if (is_vertical) {
										bar_size = ImVec2(bar_thickness, box_size.y);
										bar_pos = ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y);
									}
									else {
										bar_size = ImVec2(box_size.x, bar_thickness);
										bar_pos = ImVec2(mouse_pos.x - drag_offset.x, mouse_pos.y - drag_offset.y);
									}
								}
								else {
									if (is_vertical) {
										bar_size = ImVec2(bar_thickness, box_size.y);
										if (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::LEFT) {
											bar_pos = ImVec2(std::round(box_pos.x - gap - bar_thickness), std::round(box_pos.y));
										}
										else {
											bar_pos = ImVec2(std::round(box_pos.x + box_size.x + gap), std::round(box_pos.y));
										}
									}
									else {
										bar_size = ImVec2(box_size.x, bar_thickness);
										if (memorysense::visuals::element_sides[4] == memorysense::visuals::ESP_SIDE::TOP) {
											bar_pos = ImVec2(std::round(box_pos.x), std::round(box_pos.y - gap - bar_thickness));
										}
										else {
											bar_pos = ImVec2(std::round(box_pos.x), std::round(box_pos.y + box_size.y + gap));
										}
									}
								}

								if (is_vertical) {
									float fill_height = bar_size.y * ratio;
									ImVec2 fill_min(std::round(bar_pos.x), std::round(bar_pos.y + bar_size.y - fill_height));
									ImVec2 fill_max(std::round(bar_pos.x + bar_size.x), std::round(bar_pos.y + bar_size.y));

									if (memorysense::visuals::outline_elements[1]) {
										draw_list->AddRect(ImVec2(bar_pos.x - 1, bar_pos.y - 1),
											ImVec2(bar_pos.x + bar_size.x + 1, bar_pos.y + bar_size.y + 1),
											memorysense::visuals::outline_color, 0.f, 0, 1.f);
									}
									draw_list->AddRectFilled(fill_min, fill_max, memorysense::visuals::colors::visible::healthbar);
								}
								else {
									float fill_width = bar_size.x * ratio;
									ImVec2 fill_min(std::round(bar_pos.x), std::round(bar_pos.y));
									ImVec2 fill_max(std::round(bar_pos.x + fill_width), std::round(bar_pos.y + bar_size.y));

									if (memorysense::visuals::outline_elements[1]) {
										draw_list->AddRect(ImVec2(bar_pos.x - 1, bar_pos.y - 1),
											ImVec2(bar_pos.x + bar_size.x + 1, bar_pos.y + bar_size.y + 1),
											memorysense::visuals::outline_color, 0.f, 0, 1.f);
									}
									draw_list->AddRectFilled(fill_min, fill_max, memorysense::visuals::colors::visible::healthbar);
								}
							}

							if (memorysense::visuals::tracer) {
								ImVec2 screen_center(center.x, content_max.y - 10.0f);
								ImVec2 player_feet(center.x, box_pos.y + box_size.y);
								if (memorysense::visuals::outline_elements[5]) {
									draw_list->AddLine(ImVec2(screen_center.x - 1, screen_center.y), ImVec2(player_feet.x - 1, player_feet.y), memorysense::visuals::outline_color, 1.0f);
									draw_list->AddLine(ImVec2(screen_center.x + 1, screen_center.y), ImVec2(player_feet.x + 1, player_feet.y), memorysense::visuals::outline_color, 1.0f);
								}
								draw_list->AddLine(screen_center, player_feet, memorysense::visuals::colors::visible::tracer, 1.0f);
							}
						}
						gui->end_content();
					}

				}
				gui->end();
			}

			if (var->gui.current_section[2])
			{
				gui->set_next_window_size_constraints(ImVec2(400, 400), GetIO().DisplaySize);
				gui->begin("players", nullptr, var->window.flags);
				{
					draw->window_decorations();

					{
						gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
						gui->push_style_var(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
						gui->push_style_var(ImGuiStyleVar_ItemSpacing, elements->content.spacing);
						gui->begin_def_child("table test", ImVec2(GetWindowWidth() - elements->content.window_padding.x * 2, GetContentRegionAvail().y - elements->content.window_padding.y * 2), 0, ImGuiWindowFlags_NoMove);
						{
							gui->push_font(var->font.tahoma);
							static int selected_row = -1;

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

										draw->text_outline((std::stringstream{} << "name " << row).str().c_str());

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
								gui->pop_font();
							}
							gui->pop_style_color(5);
						}
						gui->end_def_child();
						gui->pop_style_var(2);
					}

				}
				gui->end();

			}

			if (var->gui.current_section[3])
			{
				gui->set_next_window_size_constraints(ImVec2(600, 500), GetIO().DisplaySize);
				gui->begin("Dex Explorer", nullptr, var->window.flags);
				{
					draw->window_decorations();

					{
                gui->set_cursor_pos(elements->content.window_padding + ImVec2(0, var->window.titlebar + 1));
                gui->begin_content();
						{
                    memorysense::explorer::Explorer::render();
						}
						gui->end_content();
					}
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

		var->window.width = GetCurrentWindow()->ContentSize.x + style->ItemSpacing.x;

		if (IsMouseHoveringRect(pos, pos + size))
			SetWindowFocus();
	}
	gui->end();
	gui->pop_style_var();
}