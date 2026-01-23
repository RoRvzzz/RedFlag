#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
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
		ImVec2 spacing{ 5, 6 };
		float shadow_size{ 30 };
		float shadow_alpha{ 0.3f };
		float border_size{ 0 };
		float rounding{ 4 };
		float width{ 0 };
		float titlebar{ 20 };
		float scrollbar_size{ 2 };
		bool hover_hightlight{ true };
	} window;

	struct
	{
		bool current_section[7];
		const char* section_icons[IM_ARRAYSIZE(current_section)] = {"A", "B", "C", "D", "E", "F", "G"};

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