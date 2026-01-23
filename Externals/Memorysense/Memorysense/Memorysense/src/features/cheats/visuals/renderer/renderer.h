#pragma once
#include <string>
#include <cmath>
#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_internal.h"
#include "../ext/framework/settings/variables.h"
#include <main.h>

constexpr int left = 0;
constexpr int middle = 1;
constexpr int right = 2;

struct visualize
{

	static void outlined(ImVec2& pos, ImVec2& size, ImU32 col, float rounding = 0.f, bool use_outline = true, ImU32 outline_col = IM_COL32(0, 0, 0, 255))
	{
		pos.x = std::round(pos.x); pos.y = std::round(pos.y);
		size.x = std::round(size.x); size.y = std::round(size.y);
		auto draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		ImRect rect_bb(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
		ImVec2 shadow_offset = { cosf(0.f) * 2.f, sinf(0.f) * 2.f };
		//draw->AddShadowRect(rect_bb.Min, rect_bb.Max, IM_COL32(0, 0, 0, 128), 4.f, shadow_offset, ImDrawFlags_ShadowCutOutShapeBackground, rounding);
		//draw->AddShadowRect(rect_bb.Min, rect_bb.Max, IM_COL32(255, 255, 255, 128), 8.f, { 0.f, 0.f }, ImDrawFlags_ShadowCutOutShapeBackground, 0.f);
		//draw->AddRectFilled(rect_bb.Min, rect_bb.Max, IM_COL32(50, 150, 255, 76));
		if (use_outline) {
			draw->AddRect(rect_bb.Min, rect_bb.Max, outline_col, rounding);
			draw->AddRect({ rect_bb.Min.x - 2.f, rect_bb.Min.y - 2.f }, { rect_bb.Max.x + 2.f, rect_bb.Max.y + 2.f }, outline_col, rounding);
			draw->AddRect({ rect_bb.Min.x - 1.f, rect_bb.Min.y - 1.f }, { rect_bb.Max.x + 1.f, rect_bb.Max.y + 1.f }, col, rounding);
		} else {
			draw->AddRect(rect_bb.Min, rect_bb.Max, col, rounding);
		}
	}

	static void healthbar(const ImVec2& box_pos, const ImVec2& box_size, float health, float max_health, ImU32 col, float padding = 1.0f, float size = 1.0f, bool use_outline = true, ImU32 outline_col = IM_COL32(0, 0, 0, 255))
	{
		auto draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		float ratio = (max_health > 0.f) ? health / max_health : 0.f;
		ratio = std::fmax(0.f, std::fmin(ratio, 1.f));
		float box_top = std::round(box_pos.y);
		float box_bottom = std::round(box_pos.y + box_size.y);
		float box_left = std::round(box_pos.x);
		float bar_x = box_left - padding - 3.f - size;
		ImVec2 outline_min(bar_x - 1.f, box_top - 2.f);
		ImVec2 outline_max(bar_x + size + 1.f, box_bottom + 2.f);
		if (use_outline) {
			draw->AddRectFilled(outline_min, outline_max, outline_col);
		}
		float bar_height = (box_bottom - box_top) * ratio;
		ImVec2 fill_min(bar_x, (box_bottom - bar_height) - 1.f);
		ImVec2 fill_max(bar_x + size, box_bottom + 1.f);
		draw->AddRectFilled(fill_min, fill_max, col);
	}

	static void outlined_text(const ImVec2& pos, const ImVec2& size, const char* text, ImU32 color, float y_offset = 0.f, int align = middle, bool use_outline = true, ImU32 outline_col = IM_COL32(0, 0, 0, 255))
	{
		ImDrawList* draw = ImGui::GetForegroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		ImGui::PushFont(var->font.tahoma);
		ImVec2 text_size = ImGui::CalcTextSize(text);
		float ref_width = size.x > 0.f ? size.x : text_size.x;
		float x = pos.x;
		switch (align) {
		case left:   x = pos.x; break;
		case middle: x = pos.x + (ref_width * 0.5f) - (text_size.x * 0.5f); break;
		case right:  x = pos.x + ref_width - text_size.x; break;
		}
		ImVec2 final_pos(std::round(x), std::round(pos.y + y_offset));
		if (use_outline) {
			const ImVec2 offsets[4] = { ImVec2(-1,0), ImVec2(1,0), ImVec2(0,-1), ImVec2(0,1) };
			for (auto& o : offsets)
				draw->AddText(var->font.tahoma, ImGui::GetFontSize(), ImVec2(final_pos.x + o.x, final_pos.y + o.y), outline_col, text);
		}
		draw->AddText(var->font.tahoma, ImGui::GetFontSize(), final_pos, color, text);
		ImGui::PopFont();
	}

	static void outlined_line(const ImVec2& from, const ImVec2& to, ImU32 color, float thickness = 1.0f, ImU32 outline_color = IM_COL32(0, 0, 0, 255), float outline_thickness = 2.0f, bool use_outline = true) {
		ImDrawList* draw = ImGui::GetForegroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		if (use_outline) {
			draw->AddLine(from, to, outline_color, thickness + outline_thickness);
		}
		draw->AddLine(from, to, color, thickness);
	}

	static void box_filled(ImVec2& pos, ImVec2& size, ImU32 col, float rounding = 0.f) {
		pos.x = std::round(pos.x); pos.y = std::round(pos.y);
		size.x = std::round(size.x); size.y = std::round(size.y);
		auto draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		ImRect rect_bb(pos.x, pos.y, pos.x + size.x, pos.y + size.y);
		draw->AddRectFilled(rect_bb.Min, rect_bb.Max, col, rounding);
	}

	static ImVec2 get_element_position(const ImVec2& box_pos, const ImVec2& box_size, memorysense::visuals::ESP_SIDE side, float current_offset, const ImVec2& text_size, float extra_offset = 0.0f) {
		float padding = 5.0f;
		
		switch (side) {
			case memorysense::visuals::ESP_SIDE::LEFT:
				return ImVec2(box_pos.x - padding - text_size.x - extra_offset, box_pos.y + current_offset);
			case memorysense::visuals::ESP_SIDE::RIGHT:
				return ImVec2(box_pos.x + box_size.x + padding + extra_offset, box_pos.y + current_offset);
			case memorysense::visuals::ESP_SIDE::TOP:
				return ImVec2(box_pos.x, box_pos.y - padding - current_offset - text_size.y - extra_offset);
			case memorysense::visuals::ESP_SIDE::BOTTOM:
				return ImVec2(box_pos.x, box_pos.y + box_size.y + padding + current_offset + extra_offset);
		}
		return box_pos;
	}

	static void healthbar_positioned(const ImVec2& box_pos, const ImVec2& box_size, float health, float max_health, 
		ImU32 col, memorysense::visuals::ESP_SIDE side, bool vertical, float padding = 1.0f, float bar_thickness = 1.0f, 
		bool use_outline = true, ImU32 outline_col = IM_COL32(0, 0, 0, 255)) {
		
		auto draw = ImGui::GetBackgroundDrawList();
		draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;
		float ratio = (max_health > 0.f) ? health / max_health : 0.f;
		ratio = std::fmax(0.f, std::fmin(ratio, 1.f));
		
		ImVec2 bar_pos, bar_size;
		float gap = 4.0f;
		
		if (vertical && (side == memorysense::visuals::ESP_SIDE::LEFT || side == memorysense::visuals::ESP_SIDE::RIGHT)) {
			bar_size = ImVec2(bar_thickness, box_size.y + 2.0f);
			if (side == memorysense::visuals::ESP_SIDE::LEFT) {
				bar_pos = ImVec2(box_pos.x - gap - bar_thickness, box_pos.y - 1.0f);
			} else {
				bar_pos = ImVec2(box_pos.x + box_size.x + gap, box_pos.y - 1.0f);
			}
			
			float bar_height = bar_size.y * ratio;
			ImVec2 fill_min(bar_pos.x, bar_pos.y + bar_size.y - bar_height);
			ImVec2 fill_max(bar_pos.x + bar_size.x, bar_pos.y + bar_size.y);
			
			if (use_outline) {
				draw->AddRect(ImVec2(bar_pos.x - 1, bar_pos.y - 1), ImVec2(bar_pos.x + bar_size.x + 1, bar_pos.y + bar_size.y + 1), outline_col);
			}
			draw->AddRectFilled(fill_min, fill_max, col);
		} else {
			bar_size = ImVec2(box_size.x + 2.0f, bar_thickness);
			if (side == memorysense::visuals::ESP_SIDE::TOP) {
				bar_pos = ImVec2(box_pos.x - 1.0f, box_pos.y - gap - bar_thickness);
			} else {
				bar_pos = ImVec2(box_pos.x - 1.0f, box_pos.y + box_size.y + gap);
			}
			
			float bar_width = bar_size.x * ratio;
			ImVec2 fill_min(bar_pos.x, bar_pos.y);
			ImVec2 fill_max(bar_pos.x + bar_width, bar_pos.y + bar_size.y);
			
			if (use_outline) {
				draw->AddRect(ImVec2(bar_pos.x - 1, bar_pos.y - 1), ImVec2(bar_pos.x + bar_size.x + 1, bar_pos.y + bar_size.y + 1), outline_col);
			}
			draw->AddRectFilled(fill_min, fill_max, col);
		}
	}

};
