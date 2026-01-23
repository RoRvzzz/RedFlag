#pragma once
#include <string>
#include <vector>
#include "../../imgui/imgui.h"

class c_variables
{
public:
	struct
	{
		ImGuiWindowFlags main_flags{ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_Tooltip };
		ImGuiWindowFlags flags{ ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground };
		ImVec2 padding{ 0, 0 };
		ImVec2 spacing{ 4.6, 5.7f };
		float shadow_size{ 60 };
		float shadow_alpha{ 1.0f };
		float border_size{ 0 };
		float rounding{ 4 };
		float width{ 0 };
		float titlebar{ 20 };
		float scrollbar_size{ 2 };
		bool hover_hightlight{ true };
	} window;

	struct
	{
		bool current_section[3];
		const char* section_icons[IM_ARRAYSIZE(current_section)] = {"A", "C", "F"};

		float menu_alpha{ 0 };
		bool menu_opened{ true };
		int menu_key{ 45 };
	} gui;

	struct
	{
		ImFont* icons[2];
		ImFont* tahoma;
	} font;
};

inline c_variables* var = new c_variables();